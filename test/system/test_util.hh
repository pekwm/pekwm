//
// test_util.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once



#include <cstdio>
#include <cstring>

extern "C" {
#include <sys/time.h>
#include <sys/select.h>
#include <X11/Xlib.h>
#include <errno.h>
}

bool
next_event(Display *dpy, XEvent *ev)
{
    if (XPending(dpy)) {
        return ! XNextEvent(dpy, ev);
    }

    XFlush(dpy);

    int xfd = ConnectionNumber(dpy);
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);
    FD_SET(xfd, &rfds);

    int ret;
    do {
        ret = select(xfd + 1, &rfds, 0, 0, 0);
    } while (ret == -1 && errno == EINTR);

    if (ret > 0) {
        if (FD_ISSET(xfd, &rfds)) {
            return ! XNextEvent(dpy, ev);
        }
    }
    return false;
}
