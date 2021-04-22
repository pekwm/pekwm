/**
 * Test application for (bad) transient for usage.
 */

#include "../../src/X11App.hh"
#include "../../src/Charset.hh"

#include <iostream>

static ObserverMapping* _observer_mapping;

namespace pekwm
{
    ObserverMapping* observerMapping(void) { return _observer_mapping; }
}

class TransientTest : public X11App {
public:
    TransientTest(int argc, char *argv[]);


    virtual void handleFd(int fd)
    {
        if (fd != 1) {
            return;
        }

        std::string line;
        std::getline(std::cin, line);

        if (_loop) {
            XSetTransientForHint(X11::getDpy(), _t_win2, _t_win1);
            std::cout << "PROGRESS: transient " << _t_win2 << " set to "
                      << _t_win1 << std::endl;
            XSetTransientForHint(X11::getDpy(), _window, _t_win2);
            std::cout << "PROGRESS: transient " << _t_win2 << " set to "
                      << _window << std::endl;
            _loop = false;
        } else if (_main_first && _window != None) {
            // test destroying the window that the transient window is
            // transient for before destroying the transient window.
            std::cout << "PROGRESS: destroy main window" << std::endl;
            XDestroyWindow(X11::getDpy(), _window);
            _window = None;
        } else if (_t_win1 != None) {
            std::cout << "PROGRESS: destroy transient window 1" << std::endl;
            XDestroyWindow(X11::getDpy(), _t_win1);
            _t_win1 = None;
        } else if (_t_win2 != None) {
            std::cout << "PROGRESS: destroy transient window 2" << std::endl;
            XDestroyWindow(X11::getDpy(), _t_win2);
            _t_win2 = None;
        } else if (_window != None) {
            std::cout << "PROGRESS: destroy main window" << std::endl;
            XDestroyWindow(X11::getDpy(), _window);
            _window = None;
        } else {
            std::cout << "PROGRESS: done" << std::endl;
            stop(0);
        }
    }

    void show(void)
    {
        mapWindow();
        X11::flush();
        if (_t_win1 != None) {
            X11::mapWindow(_t_win1);
            X11::flush();
        }
        if (_t_win2 != None) {
            X11::mapWindow(_t_win2);
            X11::flush();
        }
    }

private:
    Window _t_win1;
    Window _t_win2;
    bool _main_first;
    bool _loop;
};

TransientTest::TransientTest(int argc, char *argv[])
    : X11App(Geometry(0, 0, 100, 100), "transient test",
             "main", "TransientTest", WINDOW_TYPE_NORMAL),
      _t_win1(None),
      _t_win2(None)
{
    _main_first = argc == 2 && strcmp(argv[1], "destroy-main-first") == 0;
    _loop = argc == 2 && strcmp(argv[1], "transient-loop") == 0;

    XSetWindowAttributes attrs = {0};
    attrs.event_mask = PropertyChangeMask;
    unsigned long attrs_mask = CWEventMask|CWBackPixel;

    attrs.background_pixel = X11::getBlackPixel();
    _t_win1 = X11::createWindow(X11::getRoot(),
                                0, 0, 100, 100, 0,
                                CopyFromParent, //depth
                                InputOutput, // class
                                CopyFromParent, // visual
                                attrs_mask,
                                &attrs);

    if (argc == 2 && strcmp(argv[1], "transient-on-self") == 0) {
        // set transient for to self, does not make sense and should
        // not be done in real code (but could happen)
        XSetTransientForHint(X11::getDpy(), _t_win1, _t_win1);
        std::cout << "PROGRESS: transient " << _t_win1 << " set to self"
                  << std::endl;
    } else if (_loop) {
        attrs.background_pixel = X11::getBlackPixel();
        _t_win2 = XCreateWindow(X11::getDpy(), X11::getRoot(),
                                0, 0, 100, 100, 0,
                                CopyFromParent, //depth
                                InputOutput, // class
                                CopyFromParent, // visual
                                attrs_mask,
                                &attrs);
        XSetTransientForHint(X11::getDpy(), _t_win1, _window);
        std::cout << "PROGRESS: transient " << _window << " set to "
                  << _t_win1 << std::endl;
    } else {
        XSetTransientForHint(X11::getDpy(), _t_win1, _window);
        std::cout << "PROGRESS: transient " << _t_win1 << " set to "
                  << _window << std::endl;
    }
}

int
main(int argc, char *argv[])
{
    Display *dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        std::cerr << "ERROR: unable to open display" << std::endl;
        return 1;
    }

    _observer_mapping = new ObserverMapping();
    X11::init(dpy);
    Charset::init();

    {
        TransientTest test(argc, argv);
        test.addFd(1);
        test.show();
        test.main(1);
    }

    Charset::destruct();
    X11::destruct();
    delete _observer_mapping;

    return 0;
}
