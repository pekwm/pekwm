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

#include <functional>

extern "C" {
#include <getopt.h>
#include <unistd.h>
}

enum CtrlAction {
   ACTION_RUN,
   ACTION_FOCUS,
   ACTION_LIST,
   ACTION_NO
};

#ifndef UNITTEST
static void usage(const char* name, int ret)
{
    std::cout << "usage: " << name << " [-acdhs] [command]" << std::endl
              << "  -a --action [run|focus|list] Control action" << std::endl
              << "  -c --client pattern Client pattern" << std::endl
              << "  -d --display dpy    Display" << std::endl
              << "  -h --help           Display this information" << std::endl
              << "  -w --window window  Client window" << std::endl;
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
    } else {
        return ACTION_NO;
    }
}

static std::wstring readClientName(Window win)
{
    std::string name;
    if (X11::getUtf8String(win, NET_WM_NAME, name)) {
        return Util::from_utf8_str(name);
    }
    if (X11::getTextProperty(win, XA_WM_NAME, name)) {
        return Util::to_wide_str(name);
    }
    return L"";
}

static std::wstring readPekwmTitle(Window win)
{
    std::string name;
    if (X11::getString(win, PEKWM_TITLE, name)) {
        return Util::from_utf8_str(name);
    }
    return L"";
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

    return XSendEvent(X11::getDpy(), X11::getRoot(), False,
                      SubstructureRedirectMask | SubstructureNotifyMask, &ev);
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
        std::wcout << windows[i] << L" " << name;
        auto pekwm_title = readPekwmTitle(windows[i]);
        if (! pekwm_title.empty()) {
            std::wcout << L" (" << pekwm_title << ")";
        }
        std::wcout << std::endl;
    }

    X11::free(windows);

    return true;
}

int main(int argc, char* argv[])
{
    const char* display = NULL;

    static struct option opts[] = {
        {"action", required_argument, NULL, 'a'},
        {"client", required_argument, NULL, 'c'},
        {"display", required_argument, NULL, 'd'},
        {"help", no_argument, NULL, 'h'},
        {"window", required_argument, NULL, 'w'},
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
    Window client = None;
    RegexString client_re;
    while ((ch = getopt_long(argc, argv, "a:c:d:hw:", opts, NULL)) != -1) {
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
            client_re.parse_match(Util::to_wide_str(optarg));
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
            } catch (std::invalid_argument& ex) {
                std::cerr << "invalid client id " << optarg
                          << " given, expect a number" << std::endl;
            }
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
            std::wcerr << "no client match "
                       << client_re.getPattern() << std::endl;
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
        res = false;
        break;
    }

    if (res) {
        std::cout << " OK" << std::endl;
    } else {
        std::cout << " ERROR" << std::endl;
    }

    X11::destruct();
    Util::iconv_deinit();

    return 0;
}

#endif // ! UNITTEST
