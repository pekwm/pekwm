//
// ScreenResources.hh for pekwm
// Copyright © 2009-2015 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifndef _SCREEN_RESOURCES_HH_
#define _SCREEN_RESOURCES_HH_

#include "pekwm.hh"

extern "C" {
#include <X11/Xmd.h>
}

#include <map>

class ScreenResources {
public:
    enum CursorType {
        CURSOR_TOP_LEFT = BORDER_TOP_LEFT,
        CURSOR_TOP = BORDER_TOP,
        CURSOR_TOP_RIGHT = BORDER_TOP_RIGHT,
        CURSOR_LEFT = BORDER_LEFT,
        CURSOR_RIGHT = BORDER_RIGHT,
        CURSOR_BOTTOM_LEFT = BORDER_BOTTOM_LEFT,
        CURSOR_BOTTOM = BORDER_BOTTOM,
        CURSOR_BOTTOM_RIGHT = BORDER_BOTTOM_RIGHT,
        CURSOR_ARROW = BORDER_NO_POS,
        CURSOR_MOVE,
        CURSOR_RESIZE
    };

    ScreenResources(void);
    ~ScreenResources(void);

    static inline ScreenResources *instance(void) { return _instance; }

    inline Cursor getCursor(ScreenResources::CursorType type) { return _cursor_map[type]; }

private:
    std::map<CursorType, Cursor> _cursor_map;

    static ScreenResources *_instance;
};

#endif // _SCREEN_RESOURCES_HH_
