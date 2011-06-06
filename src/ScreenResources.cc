//
// ScreenResources.hh for pekwm
// Copyright (C) 2009 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "PWinObj.hh"
#include "ScreenResources.hh"
#include "PScreen.hh"
#include "Config.hh"
#include "PixmapHandler.hh"

extern "C" {
#include <X11/cursorfont.h>
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::map;

ScreenResources *ScreenResources::_instance = 0;

//! @brief ScreenResources constructor
ScreenResources::ScreenResources(void)
    : _pixmap_handler(0)
{
    if (_instance) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "ScreenResources(" << this << ")::ScreenResources()" << endl
             << " *** _instance already set: " << _instance << endl;
#endif // DEBUG
    }
    _instance = this;

    Display *dpy = PScreen::getDpy();

    // create resize cursors
    _cursor_map[CURSOR_TOP_LEFT] = XCreateFontCursor(dpy, XC_top_left_corner);
    _cursor_map[CURSOR_TOP] = XCreateFontCursor(dpy, XC_top_side);
    _cursor_map[CURSOR_TOP_RIGHT] = XCreateFontCursor(dpy, XC_top_right_corner);
    _cursor_map[CURSOR_LEFT] = XCreateFontCursor(dpy, XC_left_side);
    _cursor_map[CURSOR_RIGHT] = XCreateFontCursor(dpy, XC_right_side);
    _cursor_map[CURSOR_BOTTOM_LEFT] =
        XCreateFontCursor(dpy, XC_bottom_left_corner);
    _cursor_map[CURSOR_BOTTOM] = XCreateFontCursor(dpy, XC_bottom_side);
    _cursor_map[CURSOR_BOTTOM_RIGHT] =
        XCreateFontCursor(dpy, XC_bottom_right_corner);
    // create other cursors
    _cursor_map[CURSOR_ARROW] = XCreateFontCursor(dpy, XC_left_ptr);
    _cursor_map[CURSOR_MOVE] =  XCreateFontCursor(dpy, XC_fleur);
    _cursor_map[CURSOR_RESIZE] = XCreateFontCursor(dpy, XC_plus);

    _pixmap_handler =
        new PixmapHandler(Config::instance()->getScreenPixmapCacheSize());
}

//! @brief ScreenResources destructor
ScreenResources::~ScreenResources(void)
{
    map<CursorType, Cursor>::iterator c_it(_cursor_map.begin());
    for (; c_it != _cursor_map.end(); ++c_it) {
        XFreeCursor(PScreen::getDpy(), c_it->second);
    }

    if (_pixmap_handler) {
        delete _pixmap_handler;
    }

    _instance = 0;
}
