//
// X11App.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "X11App.hh"
#include "X11Util.hh"

extern "C" {
#include <sys/wait.h>
#include <unistd.h>
}

static bool is_signal = false;
static bool is_signal_alrm = false;
static bool is_signal_hup = false;
static bool is_signal_int_term = false;
static bool is_signal_chld = false;

static void sigHandler(int signal)
{
    is_signal = true;
    switch (signal) {
    case SIGHUP:
        is_signal_hup = true;
        break;
    case SIGINT:
    case SIGTERM:
        is_signal_int_term = true;
        break;
    case SIGCHLD:
        is_signal_chld = true;
        break;
    case SIGALRM:
        // Do nothing, just used to break out of waiting
        is_signal_alrm = true;
        break;
    }
}

/**
 * Base for X11 applications
 */
X11App::X11App(Geometry gm, const std::string &title,
               const char *wm_name, const char *wm_class,
               AtomName window_type, XSizeHints *normal_hints)
    : PWinObj(true),
      _wm_name(wm_name),
      _wm_class(wm_class),
      _stop(-1),
      _max_fd(-1)
{
    _dpy_fd = ConnectionNumber(X11::getDpy());
    addFd(_dpy_fd);

    initSignalHandler();

    _gm = gm;
    _window =
        X11::createSimpleWindow(X11::getRoot(),
                                _gm.x, _gm.y, _gm.width, _gm.height, 0,
                                X11::getBlackPixel(), X11::getWhitePixel());
    X11::selectInput(_window, StructureNotifyMask);
    X11::selectXRandrInput();

    XSizeHints default_normal_hints = {0};
    if (normal_hints == nullptr) {
        normal_hints = &default_normal_hints;
    }
    XWMHints wm_hints = {0};
    wm_hints.flags = StateHint|InputHint;
    wm_hints.initial_state = NormalState;
    wm_hints.input = True;

    XClassHint class_hint = {strdup(wm_name), strdup(wm_class)};
    Xutf8SetWMProperties(X11::getDpy(), _window,
                         title.c_str(), title.c_str(), 0, 0,
                         normal_hints, &wm_hints, &class_hint);
    free(class_hint.res_name);
    free(class_hint.res_class);

    X11::setAtom(_window, WINDOW_TYPE, window_type);

    // setting of the WM properties ensure that the
    // WM_CLIENT_MACHINE is set which is a requirement for NET_WM_PID
    X11::setCardinal(_window, NET_WM_PID, static_cast<long>(getpid()));
}

X11App::~X11App(void)
{
    X11::destroyWindow(_window);
}

/**
 * Set return code, will cause the main loop to stop at the next
 * timeout/event.
 */
void
X11App::stop(uint code) { _stop = code; }

void
X11App::addFd(int fd)
{
    if (fd > _max_fd) {
        _max_fd = fd;
    }
    _fds.push_back(fd);
}

void
X11App::removeFd(int fd)
{
    _max_fd = 0;
    auto it = _fds.begin();
    for (; it != _fds.end(); ) {
        if (*it == fd) {
            it = _fds.erase(it);
        } else {
            if (*it > _max_fd) {
                _max_fd = *it;
            }
            ++it;
        }
    }
}

int
X11App::main(uint timeout_s)
{
    bool timed_out = false;

    TRACE(_wm_name << ", " << _wm_class << ": entering main loop");
    while (_stop == -1) {
        if (is_signal) {
            handleSignal();
        } else {
            refresh(timed_out);

            if (X11::pending()) {
                processEvent();
                timed_out = false;
            } else {
                timed_out = waitForData(timeout_s);
            }
        }
    }

    return _stop;
}

/**
 * X11 event callback.
 */
void
X11App::handleEvent(XEvent*)
{
}

/**
 * File-descriptor callback, called whenever data is available on fd.
 */
void
X11App::handleFd(int)
{
}

/**
 * Refresh function, called at every timeout interval.
 */
void
X11App::refresh(bool)
{
}

/**
 * Called whenever a child process finish
 */
void
X11App::handleChildDone(pid_t, int)
{
}

/**
 * Called whenever the screen size has changed (XRandr)
 */
void
X11App::screenChanged(const ScreenChangeNotification &scn)
{
}

void
X11App::initSignalHandler(void)
{
    struct sigaction act;

    // Set up the signal handlers.
    act.sa_handler = sigHandler;
    act.sa_mask = sigset_t();
    act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;

    sigaction(SIGTERM, &act, 0);
    sigaction(SIGINT, &act, 0);
    sigaction(SIGHUP, &act, 0);
    sigaction(SIGCHLD, &act, 0);
    sigaction(SIGALRM, &act, 0);
}

void
X11App::handleSignal(void)
{
    if (is_signal_chld) {
        pid_t pid;
        do {
            int status;
            pid = waitpid(WAIT_ANY, &status, WNOHANG);
            if (pid == -1) {
                if (errno == EINTR) {
                    TRACE("waitpid interrupted, retrying");
                }
            } else if (pid == 0) {
                TRACE("no more finished child processes");
            } else {
                TRACE("child process " << pid << " finished");
                handleChildDone(pid, WEXITSTATUS(status));
            }
        } while (pid > 0 || (pid == -1 && errno == EINTR));

        is_signal_chld = false;
    }
    if (is_signal_int_term) {
        stop(1);
        is_signal_int_term = false;
    }
    is_signal = false;
}

bool
X11App::waitForData(int timeout_s)
{
    // flush before selecting input ensuring any outstanding
    // output is sent before waiting on a reply.
    X11::flush();

    fd_set rfds;
    FD_ZERO(&rfds);
    for (int fd : _fds) {
        FD_SET(fd, &rfds);
    }

    struct timeval timeout = { timeout_s, 0 };
    int ret = select(_max_fd + 1, &rfds, nullptr, nullptr, &timeout);
    if (ret > 0) {
        for (int fd : _fds) {
            if (! FD_ISSET(fd, &rfds)) {
                continue;
            }

            if (fd == _dpy_fd) {
                processEvent();
            } else {
                handleFd(fd);
            }
        }
    }

    return ret < 1;
}

void
X11App::processEvent(void)
{
    static ScreenChangeNotification scn;

    XEvent ev;
    if (X11::getNextEvent(ev)) {
        if (X11::getScreenChangeNotification(&ev, scn)) {
            X11::updateGeometry(scn.width, scn.height);
            screenChanged(scn);
        } else {
            handleEvent(&ev);
        }
    }
}
