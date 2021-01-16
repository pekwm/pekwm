//
// pekwm_bg.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"

#include "ImageHandler.hh"
#include "TextureHandler.hh"
#include "Util.hh"
#include "x11.hh"

extern "C" {
#include <getopt.h>
#include <signal.h>
#include <unistd.h>
}

static bool _stop = false;
static ImageHandler* _image_handler = nullptr;
static TextureHandler* _texture_handler = nullptr;

namespace pekwm
{
    ImageHandler* imageHandler()
    {
        return _image_handler;
    }

    TextureHandler* textureHandler()
    {
        return _texture_handler;
    }
}

static void sighandler(int signal)
{
    if (signal == SIGINT) {
        _stop = true;
    }
}

static void init(Display* dpy)
{
    _image_handler = new ImageHandler();
    _texture_handler = new TextureHandler();
}

static void cleanup()
{
    delete _texture_handler;
    delete _image_handler;
}

static void usage(const char* name, int ret)
{
    std::cout << "usage: " << name << " [-hl] texture" << std::endl
              << "  -d --display dpy    Display" << std::endl
              << "  -D --daemon         Run in the background" << std::endl
              << "  -h --help           Display this information" << std::endl
              << "  -l --load-dir path  Search path for images" << std::endl
              << "  -s --stop           Stop running pekwm_bg" << std::endl;
    exit(ret);
}

static Pixmap setBackground(PTexture *tex)
{
    auto pix = X11::createPixmap(X11::getWidth(), X11::getHeight());

    // render background per-head, might not suite all but leave it at
    // that for now.
    for (int i = 0; i < X11::getNumHeads(); i++) {
        auto head = X11::getHeadGeometry(i);
        tex->render(pix, head.x, head.y, head.width, head.height);
    }

    X11::setLong(X11::getRoot(), XROOTPMAP_ID, pix, XA_PIXMAP);
    X11::setLong(X11::getRoot(), XSETROOT_ID, pix, XA_PIXMAP);
    X11::setWindowBackgroundPixmap(X11::getRoot(), pix);
    X11::clearWindow(X11::getRoot());

    return pix;
}

void modeBackground(const std::string& tex_str)
{
    auto tex = pekwm::textureHandler()->getTexture(tex_str);
    if (tex) {
        std::cout << "Setting background " << tex_str << std::endl;
        auto pix = setBackground(tex);
        pekwm::textureHandler()->returnTexture(tex);

        // used for stop actions
        X11::setLong(X11::getRoot(), PEKWM_BG_PID, getpid());

        XEvent ev;
        while (! _stop && X11::getNextEvent(ev)) {
            ;
        }

        X11::freePixmap(pix);
    } else {
        std::cerr << "Failed to load texture " << tex_str << std::endl;
    }
}

void modeStop()
{
    long pid;
    if (! X11::getLong(X11::getRoot(), PEKWM_BG_PID, pid)) {
        std::cerr << "Failed to get _PEKWM_BG_PID, unable to stop" << std::endl;
        return;
    }

    kill(pid, SIGINT);

    std::cerr << "Sent " << pid << " SIGINT" << std::endl;
}

int main(int argc, char* argv[])
{
    const char* display = NULL;
    bool do_daemon = false;
    bool stop = false;
    std::string load_dir(".");

    static struct option opts[] = {
        {"display", required_argument, NULL, 'd'},
        {"daemon", no_argument, NULL, 'D'},
        {"help", no_argument, NULL, 'h'},
        {"load-dir", required_argument, NULL, 'l'},
        {"stop", no_argument, NULL, 's'},
        {NULL, 0, NULL, 0}
    };

    int ch;
    while ((ch = getopt_long(argc, argv, "d:Dhl:s", opts, NULL)) != -1) {
        switch (ch) {
        case 'd':
            display = optarg;
            break;
        case 'D':
            do_daemon = true;
            break;
        case 'h':
            usage(argv[0], 0);
            break;
        case 'l':
            load_dir = optarg;
            if (load_dir.back() != '/') {
                load_dir += '/';
            }
            Util::expandFileName(load_dir);
            break;
        case 's':
            stop = true;
            break;
        default:
            usage(argv[0], 1);
            break;
        }
    }

    std::string tex_str;
    for (int i = optind; i < argc; i++) {
        if (i > optind) {
            tex_str += " ";
        }
        tex_str += argv[i];
    }

    if (! stop && tex_str.empty()) {
        usage(argv[0], 1);
        return 1;
    }

    auto dpy = XOpenDisplay(display);
    if (! dpy) {
        std::cerr << "Can not open display!" << std::endl
                  << "Your DISPLAY variable currently is set to: "
                  << Util::getEnv("DISPLAY") << std::endl;
        return 1;
    }

    // setup signal handler after connecting to the display
    signal(SIGINT, &sighandler);

    X11::init(dpy, true);
    init(dpy);

    std::cout << "Load dir " << load_dir << std::endl;
    _image_handler->path_push_back(load_dir);

    modeStop();
    if (! stop) {
        if (do_daemon) {
            if (daemon(0, 0) == -1) {
                std::cerr << "Failed to daemonize: " << strerror(errno)
                          << std::endl;
            }
        }
        modeBackground(tex_str);
    }

    cleanup();
    X11::destruct();

    return 0;
}
