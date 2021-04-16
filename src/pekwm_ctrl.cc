//
// pekwm_ctrl.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "Compat.hh"
#include "Charset.hh"
#include "Debug.hh"
#include "RegexString.hh"
#include "Util.hh"
#include "X11.hh"

#include <functional>

extern "C" {
#include <getopt.h>
#include <time.h>
#include <unistd.h>
}

enum CtrlAction {
   ACTION_RUN,
   ACTION_FOCUS,
   ACTION_LIST,
   ACTION_UTIL,
   ACTION_NO
};

#ifndef UNITTEST
static void usage(const char* name, int ret)
{
    std::cout << "usage: " << name << " [-acdhs] [command]" << std::endl;
    std::cout << "  -a --action [run|focus|list|util] Control action" << std::endl;
    std::cout << "  -c --client pattern Client pattern" << std::endl;
    std::cout << "  -d --display dpy    Display" << std::endl;
    std::cout << "  -h --help           Display this information" << std::endl;
    std::cout << "  -w --window window  Client window" << std::endl;
    exit(ret);
}

static CtrlAction getAction(const std::string& name)
{
    if (name == "focus") {
        return ACTION_FOCUS;
    } else if (name == "list") {
        return ACTION_LIST;
    } else if (name == "run") {
        return ACTION_RUN;
    } else if (name == "util") {
        return ACTION_UTIL;
    } else {
        return ACTION_NO;
    }
}

static std::string readClientName(Window win)
{
    std::string name;
    if (X11::getUtf8String(win, NET_WM_NAME, name)) {
        return name;
    }
    if (X11::getTextProperty(win, XA_WM_NAME, name)) {
        return name;
    }
    return "";
}

static std::string readPekwmTitle(Window win)
{
    std::string name;
    if (X11::getUtf8String(win, PEKWM_TITLE, name)) {
        return name;
    }
    return "";
}

static Window findClient(const RegexString& match)
{
    ulong actual;
    Window *windows;
    if (! X11::getProperty(X11::getRoot(), X11::getAtom(NET_CLIENT_LIST),
                           XA_WINDOW, 0,
                           reinterpret_cast<uchar**>(&windows), &actual)) {
        return None;
    }

    for (uint i = 0; i < actual; i++) {
        auto name = readClientName(windows[i]);
        if (match == name) {
            return windows[i];
        }
    }

    X11::free(windows);

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

    return X11::sendEvent(X11::getRoot(), False,
                          SubstructureRedirectMask | SubstructureNotifyMask,
                          &ev);
}

static bool focusClient(Window win)
{
    return sendClientMessage(win, NET_ACTIVE_WINDOW, 32, nullptr, 0);
}

#endif // ! UNITTEST

static bool sendCommand(const std::string& cmd, Window win,
                        std::function<bool (Window, AtomName, int,
                                            const void*, size_t)> send_message)
{
    XClientMessageEvent ev;
    char buf[sizeof(ev.data.b)] = {0};
    int chunk_size = sizeof(buf) - 1;

    const char *src = cmd.c_str();
    int left = cmd.size();
    buf[chunk_size] = static_cast<int>(cmd.size()) <= chunk_size ? 0 : 1;
    memcpy(buf, src, std::min(left, chunk_size));
    bool res = send_message(win, PEKWM_CMD, 8, buf, sizeof(buf));
    src += chunk_size;
    left -= chunk_size;
    while (res && left > 0) {
        memset(buf, 0, sizeof(buf));
        buf[chunk_size] = left <= chunk_size ? 3 : 2;
        memcpy(buf, src, std::min(left, chunk_size));
        src += chunk_size;
        left -= chunk_size;
        res = send_message(win, PEKWM_CMD, 8, buf, sizeof(buf));
    }

    return res;
}

#ifndef UNITTEST

static bool listClients(void)
{
    ulong actual;
    Window *windows;
    if (! X11::getProperty(X11::getRoot(), X11::getAtom(NET_CLIENT_LIST),
                           XA_WINDOW, 0,
                           reinterpret_cast<uchar**>(&windows), &actual)) {
        return false;
    }

    for (uint i = 0; i < actual; i++) {
        auto name = readClientName(windows[i]);
        auto mb_name = Charset::toSystem(name);
        auto pekwm_title = readPekwmTitle(windows[i]);
        auto mb_pekwm_title = Charset::toSystem(pekwm_title);

        std::cout << windows[i] << " " << mb_name;
        if (! mb_pekwm_title.empty()) {
            std::cout << " (" << mb_pekwm_title << ")";
        }
        std::cout << std::endl;
    }

    X11::free(windows);

    return true;
}

int actionUtil(int argc, char* argv[])
{
    if (argc == 0) {
        return 1;
    }

    std::string cmd(argv[0]);
    if (cmd == "timestamp") {
        time_t ts = time(nullptr);
        std::cout << std::to_string(ts) << std::endl;
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char* argv[])
{
    const char* display = NULL;

    static struct option opts[] = {
        {const_cast<char*>("action"), required_argument, nullptr, 'a'},
        {const_cast<char*>("client"), required_argument, nullptr, 'c'},
        {const_cast<char*>("display"), required_argument, nullptr, 'd'},
        {const_cast<char*>("help"), no_argument, nullptr, 'h'},
        {const_cast<char*>("window"), required_argument, nullptr, 'w'},
        {nullptr, 0, nullptr, 0}
    };

    Charset::init();

    int ch;
    CtrlAction action = ACTION_RUN;
    Window client = None;
    RegexString client_re;
    while ((ch = getopt_long(argc, argv, "a:c:d:hw:", opts, nullptr)) != -1) {
        switch (ch) {
        case 'a':
            action = getAction(optarg);
            if (action == ACTION_NO) {
                usage(argv[0], 1);
            }
            break;
        case 'c':
            if (client != None) {
                std::cerr << "-c and -w are mutually exclusive" << std::endl;
                usage(argv[0], 1);
            }
            client_re.parse_match(optarg);
            break;
        case 'd':
            display = optarg;
            break;
        case 'h':
            usage(argv[0], 0);
            break;
        case 'w':
            if (client_re.is_match_ok()) {
                std::cerr << "-c and -w are mutually exclusive" << std::endl;
                usage(argv[0], 1);
            }
            try {
                client = std::stoi(optarg);
            } catch (std::invalid_argument&) {
                std::cerr << "invalid client id " << optarg
                          << " given, expect a number" << std::endl;
            }
            break;
        default:
            usage(argv[0], 1);
            break;
        }
    }

    if (action == ACTION_UTIL) {
        return actionUtil(argc - optind, argv + optind);
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
        auto actual_display = display ? display : Util::getEnv("DISPLAY");
        std::cerr << "Can not open display!" << std::endl
                  << "Your DISPLAY variable currently is set to: "
                  << actual_display << std::endl;
        return 1;
    }

    X11::init(dpy, true);

    if (client_re.is_match_ok()) {
        client = findClient(client_re);
        if (client == None) {
            std::cerr << "no client match ";
            std::cerr << Charset::toSystem(client_re.getPattern()) << std::endl;
            return 1;
        }
    }

    bool res;
    switch (action) {
    case ACTION_RUN:
        if (client == None) {
            client = X11::getRoot();
        }
        if (cmd.empty()) {
            std::cerr << "empty command string" << std::endl;
            usage(argv[0], 1);
        }
        std::cout << "_PEKWM_CMD " << client << " " << cmd;
        res = sendCommand(cmd, client, sendClientMessage);
        break;
    case ACTION_FOCUS:
        std::cout << "_NET_ACTIVE_WINDOW " << client;
        res = focusClient(client);
        break;
    case ACTION_LIST:
        std::cout << "_NET_CLIENT_LIST" << std::endl;
        res = listClients();
        break;
    case ACTION_NO:
    case ACTION_UTIL:
        res = false;
        break;
    }

    if (res) {
        std::cout << " OK" << std::endl;
    } else {
        std::cout << " ERROR" << std::endl;
    }

    X11::destruct();
    Charset::destruct();

    return 0;
}

#endif // ! UNITTEST
