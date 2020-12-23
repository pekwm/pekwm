/**
 * Client used for testing pekwm.
 */

#include <iostream>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
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
    unsigned long attrs_mask = CWEventMask;

    Window win = XCreateWindow(dpy, root,
                               0, 0, 100, 100, 0,
                               CopyFromParent, //depth
                               InputOutput, // class
                               CopyFromParent, // visual
                               attrs_mask,
                               &attrs);
    XMapWindow(dpy, win);

    std::cout << "Window " << win << std::endl;

    XEvent ev;
    while (! XNextEvent(dpy, &ev)) {
    }

    XCloseDisplay(dpy);

    return 0;
}
