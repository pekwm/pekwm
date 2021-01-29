//
// pekwm_ctrl.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "RegexString.hh"
#include "Util.hh"
#include "x11.hh"

extern "C" {
#include <getopt.h>
#include <unistd.h>
}

enum CtrlAction {
   ACTION_RUN,
   ACTION_FOCUS,
   ACTION_NO
};

static void usage(const char* name, int ret)
{
    std::cout << "usage: " << name << " [-dhsw]" << std::endl
              << "  -a --action [run|focus] Control action" << std::endl
              << "  -c --client pattern Client pattern" << std::endl
              << "  -d --display dpy    Display" << std::endl
              << "  -h --help           Display this information" << std::endl;
    exit(ret);
}

static CtrlAction getAction(const std::string& name)
{
    if (name == "focus") {
        return ACTION_FOCUS;
    } else if (name == "run") {
        return ACTION_RUN;
    } else {
        return ACTION_NO;
    }
}

static std::wstring readClientName(Window window)
{
    std::string name;
    if (X11::getUtf8String(window, NET_WM_NAME, name)) {
        return Util::from_utf8_str(name);
    }
    if (X11::getTextProperty(window, XA_WM_NAME, name)) {
        return Util::to_wide_str(name);
    }
    return L"";
}

static Window findClient(const RegexString& match)
{
    ulong actual;
    Window *windows;
    if (! X11::getProperty(X11::getRoot(), NET_CLIENT_LIST, XA_WINDOW,
                           0, reinterpret_cast<uchar**>(&windows), &actual)) {
        return None;
    }

    for (uint i = 0; i < actual; i++) {
        auto name = readClientName(windows[i]);
        if (match == name) {
            return windows[i];
        }
    }

    XFree(windows);

    return None;
}

static bool sendClientMessage(Window window, AtomName atom,
                              int format, const void* data, size_t size)
{
    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.serial = 0;
    ev.xclient.send_event = True;
    ev.xclient.message_type = X11::getAtom(atom);
    ev.xclient.window = window;
    ev.xclient.format = format;
    memset(ev.xclient.data.b, 0, sizeof(ev.xclient.data.b));
    if (size) {
        memcpy(ev.xclient.data.b, data,
               std::min(size, sizeof(ev.xclient.data.b)));
    }

    return XSendEvent(X11::getDpy(), X11::getRoot(), False,
                      SubstructureRedirectMask | SubstructureNotifyMask, &ev);
}

static bool focusClient(Window window)
{
    std::cout << "_NET_ACTIVE_WINDOW " << window << std::endl;
    return sendClientMessage(window, NET_ACTIVE_WINDOW,
                             32, nullptr, 0);
}

static bool sendCommand(const std::string& cmd)
{
    return sendClientMessage(X11::getRoot(), PEKWM_CMD,
                             8, cmd.c_str(), cmd.size());
}

int main(int argc, char* argv[])
{
    const char* display = NULL;

    static struct option opts[] = {
        {"action", required_argument, NULL, 'a'},
        {"client", required_argument, NULL, 'c'},
        {"display", required_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    try {
        std::locale::global(std::locale(""));
    } catch (const std::runtime_error &e) {
        setlocale(LC_ALL, "");
    }

    Util::iconv_init();

    int ch;
    CtrlAction action = ACTION_RUN;
    RegexString client_re;
    while ((ch = getopt_long(argc, argv, "a:c:d:hs:w:", opts, NULL)) != -1) {
        switch (ch) {
        case 'a':
            action = getAction(optarg);
            if (action == ACTION_NO) {
                usage(argv[0], 1);
            }
            break;
        case 'c':
            client_re.parse_match(Util::to_wide_str(optarg));
            break;
        case 'd':
            display = optarg;
            break;
        case 'h':
            usage(argv[0], 0);
            break;
        default:
            usage(argv[0], 1);
            break;
        }
    }

    std::string cmd;
    for (int i = optind; i < argc; i++) {
        if (i != optind) {
            cmd += " ";
        }
        cmd += argv[i];
    }

    auto dpy = XOpenDisplay(display);
    if (! dpy) {
        std::cerr << "Can not open display!" << std::endl
                  << "Your DISPLAY variable currently is set to: "
                  << Util::getEnv("DISPLAY") << std::endl;
        return 1;
    }

    X11::init(dpy, true);

    Window client = None;
    if (client_re.is_match_ok()) {
        client = findClient(client_re);
        if (client == None) {
            std::wcerr << "no client match "
                       << client_re.getPattern() << std::endl;
            return 1;
        }
    }

    switch (action) {
    case ACTION_RUN:
        sendCommand(cmd);
        break;
    case ACTION_FOCUS:
        focusClient(client);
        break;
    case ACTION_NO:
        break;
    }

    X11::destruct();
    Util::iconv_deinit();

    return 0;
}
