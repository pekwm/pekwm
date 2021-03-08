/**
 * Test application for (bad) transient for usage.
 */

#include <iostream>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <string.h>
}

int
main(int argc, char *argv[])
{
    Display *dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        std::cerr << "ERROR: unable to open display" << std::endl;
        return 1;
    }

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    XSetWindowAttributes attrs = {0};
    attrs.event_mask = PropertyChangeMask;
    unsigned long attrs_mask = CWEventMask|CWBackPixel;

    attrs.background_pixel = WhitePixel(dpy, screen);
    auto win = XCreateWindow(dpy, root,
                             0, 0, 100, 100, 0,
                             CopyFromParent, //depth
                             InputOutput, // class
                             CopyFromParent, // visual
                             attrs_mask,
                             &attrs);

    attrs.background_pixel = BlackPixel(dpy, screen);
    auto t_win1 = XCreateWindow(dpy, root,
                               0, 0, 100, 100, 0,
                               CopyFromParent, //depth
                               InputOutput, // class
                               CopyFromParent, // visual
                               attrs_mask,
                               &attrs);

    Window t_win2 = None;

    if (argc == 2 && strcmp(argv[1], "transient-on-self") == 0) {
        // set transient for to self, does not make sense and should
        // not be done in real code (but could happen)
        XSetTransientForHint(dpy, t_win1, t_win1);
        std::cout << "PROGRESS: transient " << t_win1 << " set to self"
                  << std::endl;
    } else if (argc == 2 && strcmp(argv[1], "transient-loop") == 0) {
        attrs.background_pixel = BlackPixel(dpy, screen);
        t_win2 = XCreateWindow(dpy, root,
                               0, 0, 100, 100, 0,
                               CopyFromParent, //depth
                               InputOutput, // class
                               CopyFromParent, // visual
                               attrs_mask,
                               &attrs);

        XSetTransientForHint(dpy, win, t_win1);
        std::cout << "PROGRESS: transient " << win << " set to "
                  << t_win1 << std::endl;
        XSetTransientForHint(dpy, t_win1, t_win2);
        std::cout << "PROGRESS: transient " << t_win1 << " set to "
                  << t_win2 << std::endl;
        XSetTransientForHint(dpy, t_win2, win);
        std::cout << "PROGRESS: transient " << t_win2 << " set to "
                  << win << std::endl;
    } else {
        XSetTransientForHint(dpy, win, t_win1);
        std::cout << "PROGRESS: transient " << t_win1 << " set to "
                  << win << std::endl;
    }

    XMapWindow(dpy, win);
    XFlush(dpy);
    XMapWindow(dpy, t_win1);
    XFlush(dpy);
    if (t_win2 != None) {
        XMapWindow(dpy, t_win2);
        XFlush(dpy);
    }

    std::cout << "PROGRESS: windows mapped (press enter)" << std::endl;
    std::string line;
    std::getline(std::cin, line);

    Window last_win;
    if (argc == 2 && strcmp(argv[1], "destroy-main-first") == 0) {
        // test destroying the window that the transient window is
        // transient for before destroying the transient window.
        std::cout << "PROGRESS: destroy main window" << std::endl;
        XDestroyWindow(dpy, win);
        last_win = t_win1;
    } else {
        std::cout << "PROGRESS: destroy transient window 1" << std::endl;
        XDestroyWindow(dpy, t_win1);
        last_win = win;
    }

    if (t_win2 != None) {
        std::cout << "PROGRESS: destroy transient window 2" << std::endl;
        XDestroyWindow(dpy, t_win2);
    }

    std::cout << "PROGRESS: destroy last window" << std::endl;
    XDestroyWindow(dpy, last_win);
    XCloseDisplay(dpy);

    return 0;
}
