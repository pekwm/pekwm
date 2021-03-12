#include <cstdlib>
#include <iostream>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
}

unsigned char*
get_propery(Display *dpy, Window root, Atom atom,
            unsigned long rread, unsigned long *nread, unsigned long *nleft)
{
    Atom real_type;
    int real_format;
    unsigned char *udata = 0;

    XGetWindowProperty(dpy, root, atom,
                       0L, rread, False, XA_WINDOW,
                       &real_type, &real_format, nread, nleft, &udata);
    if (rread == 0 && udata) {
        XFree(udata);
        udata = 0;
    }

    return udata;
}

void
print_net_client_list(Display *dpy, Window root, Atom net_client_list)
{
    unsigned char *data;
    unsigned long nread, nleft;

    data = get_propery(dpy, root, net_client_list, 0L, &nread, &nleft);
    nleft /= 4;

    data = get_propery(dpy, root, net_client_list, nleft, &nread, &nleft);
    Window *wins = reinterpret_cast<Window*>(data);
    std::cout << "BEGIN WINDOWS" << std::endl;
    for (unsigned long i = 0; i < nread; i++) {
        std::cout << "  Window " << wins[i] << std::endl;
    }
    std::cout << "END WINDOWS" << std::endl;
    XFree(data);
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

    // wait for property changes on the root window, will be sent
    // whenever atoms get updated
    XSelectInput(dpy, root, PropertyChangeMask);

    // print initial window list
    Atom net_client_list = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    print_net_client_list(dpy, root, net_client_list);

    // wait for property notify on window, then read the _NET_FRAME_EXTENTS
    std::cout << "PROGRESS: wait for PropertyNotify" << std::endl;
    XEvent ev;
    while (! XNextEvent(dpy, &ev)) {
        if (ev.type != PropertyNotify) {
            std::cout << "ERROR: got event " << ev.type << std::endl;
            exit(1);
        }

        XPropertyEvent *pev = &ev.xproperty;
        if (pev->atom == net_client_list) {
            std::cout << "PROGRESS: got event " << ev.type << std::endl;
            print_net_client_list(dpy, root, net_client_list);
        }
    }

    XCloseDisplay(dpy);

    return 0;
}
