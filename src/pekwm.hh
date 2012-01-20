//
// pekwm.hh for pekwm
// Copyright (C) 2003-2009 Claes Nasten <pekdon{@}pekdon{.}net>
//
// aewm.hh for aewm++
// Copyright (C) 2002 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_HH_
#define _PEKWM_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Compat.hh"
#include "Types.hh"
#include "Exception.hh"

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xmd.h>
}

// data types

class Geometry {
public:
  Geometry(void)
    : x(0), y(0), width(1), height(1) { }
  Geometry(int _x, int _y, unsigned int _width, unsigned int _height) :
    x(_x), y(_y), width(_width), height(_height) { }
  Geometry(const Geometry &gm)
    : x(gm.x), y(gm.y), width(gm.width), height(gm.height) { }
  ~Geometry(void) { }

  int x, y;
  unsigned int width, height;

    inline Geometry& operator = (const Geometry& gm) {
        x = gm.x;
        y = gm.y;
        width = gm.width;
        height = gm.height;
        return *this;
    }
    inline bool operator == (const Geometry& gm) {
        if ((x == gm.x) && (y == gm.y) &&
                (width == gm.width) && (height == gm.height))
            return true;
        return false;
    }
    inline bool operator != (const Geometry& gm) {
        if ((x != gm.x) || (y != gm.y) ||
                (width != gm.width) || (height != gm.height))
            return true;
        return false;
    }
};

// Extended Net Hints stuff
class NetWMStates {
public:
    NetWMStates(void)
        : modal(false), sticky(false),
          max_vert(false), max_horz(false), shaded(false),
          skip_taskbar(false), skip_pager(false),
          hidden(false), fullscreen(false),
          above(false), below(false), demands_attention(false) { }
    ~NetWMStates(void) { }

    bool modal;
    bool sticky;
    bool max_vert, max_horz;
    bool shaded;
    bool skip_taskbar, skip_pager;
    bool hidden;
    bool fullscreen;
    bool above, below;
    bool demands_attention;
};

#define NET_WM_STICKY_WINDOW	0xffffffff
#define EWMH_OPAQUE_WINDOW	0xffffffff

// enums
enum Layer {
    LAYER_DESKTOP = 0,
    LAYER_DESKTOP_TRANSIENT = 1,
    LAYER_BELOW = 2,
    LAYER_BELOW_TRANSIENT = 3,
    LAYER_NORMAL = 4,
    LAYER_NORMAL_TRANSIENT = 5,
    LAYER_ONTOP = 6,
    LAYER_ONTOP_TRANSIENT = 7,
    LAYER_DOCK = 8,
    LAYER_DOCK_TRANSIENT = 9,
    LAYER_ABOVE_DOCK = 10,
    LAYER_ABOVE_DOCK_TRANSIENT = 11,
    LAYER_MENU = 12,
    LAYER_MENU_TRANSIENT = 13,
    LAYER_NONE = 14
};

enum ApplyOn {
    APPLY_ON_START = (1<<1),
    APPLY_ON_NEW = (1<<2),
    APPLY_ON_RELOAD = (1<<3),
    APPLY_ON_WORKSPACE = (1<<4),
    APPLY_ON_TRANSIENT = (1<<5),
    APPLY_ON_TRANSIENT_ONLY = (1<<6),
    APPLY_ON_NONE = 0
};

enum Skip {
    SKIP_MENUS = (1<<1),
    SKIP_FOCUS_TOGGLE = (1<<2),
    SKIP_SNAP = (1<<3),
    SKIP_PAGER = (1<<4),
    SKIP_TASKBAR = (1<<5),
    SKIP_NONE = 0
};

enum BorderPosition {
    BORDER_TOP_LEFT = 0, BORDER_TOP_RIGHT,
    BORDER_BOTTOM_LEFT, BORDER_BOTTOM_RIGHT,
    BORDER_TOP, BORDER_LEFT, BORDER_RIGHT, BORDER_BOTTOM,
    BORDER_NO_POS
};

enum ButtonState {
    BUTTON_STATE_FOCUSED, BUTTON_STATE_UNFOCUSED, BUTTON_STATE_PRESSED,
    BUTTON_STATE_HOVER, BUTTON_STATE_NO
};
enum FocusedState {
    FOCUSED_STATE_FOCUSED, FOCUSED_STATE_UNFOCUSED,
    FOCUSED_STATE_FOCUSED_SELECTED, FOCUSED_STATE_UNFOCUSED_SELECTED,
    FOCUSED_STATE_NO
};
enum DecorState {
    DECOR_TITLEBAR = (1<<1),
    DECOR_BORDER = (1<<2)
};

enum ImageType {
    IMAGE_TYPE_TILED = 1, IMAGE_TYPE_SCALED, IMAGE_TYPE_FIXED, IMAGE_TYPE_NO = 0
};

enum FontJustify {
    FONT_JUSTIFY_LEFT, FONT_JUSTIFY_CENTER, FONT_JUSTIFY_RIGHT, FONT_JUSTIFY_NO
};
enum PlacementModel {
    PLACE_SMART, PLACE_MOUSE_NOT_UNDER, PLACE_MOUSE_CENTERED,
    PLACE_MOUSE_TOP_LEFT, PLACE_CENTERED_ON_PARENT, PLACE_NO
};
enum HarbourPlacement {
    TOP, LEFT, RIGHT, BOTTOM, NO_HARBOUR_PLACEMENT
};
enum EdgeType {
    SCREEN_EDGE_TOP, SCREEN_EDGE_BOTTOM,
    SCREEN_EDGE_LEFT, SCREEN_EDGE_RIGHT, SCREEN_EDGE_NO
};
enum PadType {
    PAD_UP, PAD_DOWN, PAD_LEFT, PAD_RIGHT, PAD_NO
};
enum DirectionType {
    DIRECTION_UP, DIRECTION_DOWN,
    DIRECTION_LEFT, DIRECTION_RIGHT,
    DIRECTION_NO
};
enum WorkspaceChangeType {
    WORKSPACE_NO = (uint) ~0, WORKSPACE_LEFT = WORKSPACE_NO - 9,
    WORKSPACE_PREV, WORKSPACE_NEXT, WORKSPACE_RIGHT,
    WORKSPACE_PREV_V, WORKSPACE_UP, WORKSPACE_NEXT_V, WORKSPACE_DOWN,
    WORKSPACE_LAST
};
enum OrientationType {
    TOP_LEFT, TOP_EDGE, TOP_CENTER_EDGE, TOP_RIGHT,
    BOTTOM_RIGHT, BOTTOM_EDGE, BOTTOM_CENTER_EDGE, BOTTOM_LEFT,
    LEFT_EDGE, LEFT_CENTER_EDGE, RIGHT_EDGE, RIGHT_CENTER_EDGE,
    CENTER, NO_EDGE
};
enum Raise {
    ALWAYS_RAISE, END_RAISE, NEVER_RAISE, NO_RAISE
};
enum Orientation {
    TOP_TO_BOTTOM = 1, LEFT_TO_RIGHT = 1,	BOTTOM_TO_TOP = 2, RIGHT_TO_LEFT = 2,
    NO_ORIENTATION = 0
};
enum MouseEventType {
    MOUSE_EVENT_PRESS = (1<<1),
    MOUSE_EVENT_RELEASE = (1<<2),
    MOUSE_EVENT_DOUBLE = (1<<3),
    MOUSE_EVENT_MOTION = (1<<4),
    MOUSE_EVENT_ENTER = (1<<5),
    MOUSE_EVENT_LEAVE = (1<<6),
    MOUSE_EVENT_ENTER_MOVING = (1<<7),
    MOUSE_EVENT_MOTION_PRESSED = (1<<8), /**< Motion event with button pressed. */
    MOUSE_EVENT_NO = 0
};
enum ButtonNum {
    BUTTON_ANY = 0,
    BUTTON1 = Button1, BUTTON2, BUTTON3, BUTTON4, BUTTON5,
    BUTTON6, BUTTON7, BUTTON8, BUTTON9, BUTTON10, BUTTON11,
    BUTTON12, BUTTON_NO
};
enum Mod {
    MOD_ANY = (uint) ~0
};
enum Focus {
    FOCUS_NEW = (1<<1),
    FOCUS_ENTER = (1<<2),
    FOCUS_LEAVE = (1<<3),
    FOCUS_CLICK = (1<<4),
    NO_FOCUS = 0
};
enum MenuType {
    WINDOWMENU_TYPE, ROOTMENU_TYPE, ROOTMENU_STANDALONE_TYPE,
    GOTOMENU_TYPE, ICONMENU_TYPE,
    ATTACH_CLIENT_TYPE, ATTACH_FRAME_TYPE,
    ATTACH_CLIENT_IN_FRAME_TYPE, ATTACH_FRAME_IN_FRAME_TYPE,
    DYNAMIC_MENU_TYPE, DECORMENU_TYPE, GOTOCLIENTMENU_TYPE, NO_MENU_TYPE
};
enum ObjectState {
    OBJECT_STATE_FOCUSED = 0, OBJECT_STATE_UNFOCUSED = 1,
    OBJECT_STATE_SELECTED = 2, OBJECT_STATE_NO = 2
};
enum MouseActionListName {
    MOUSE_ACTION_LIST_TITLE_FRAME, MOUSE_ACTION_LIST_TITLE_OTHER,
    MOUSE_ACTION_LIST_CHILD_FRAME, MOUSE_ACTION_LIST_CHILD_OTHER,
    MOUSE_ACTION_LIST_ROOT, MOUSE_ACTION_LIST_MENU,
    MOUSE_ACTION_LIST_OTHER,

    MOUSE_ACTION_LIST_EDGE_T, MOUSE_ACTION_LIST_EDGE_B,
    MOUSE_ACTION_LIST_EDGE_L, MOUSE_ACTION_LIST_EDGE_R,

    MOUSE_ACTION_LIST_BORDER_TL, MOUSE_ACTION_LIST_BORDER_T,
    MOUSE_ACTION_LIST_BORDER_TR, MOUSE_ACTION_LIST_BORDER_L,
    MOUSE_ACTION_LIST_BORDER_R, MOUSE_ACTION_LIST_BORDER_BL,
    MOUSE_ACTION_LIST_BORDER_B, MOUSE_ACTION_LIST_BORDER_BR
};
enum CfgDeny {
    CFG_DENY_POSITION = (1L << 0), //!< ConfigureRequest position deny.
    CFG_DENY_SIZE = (1L << 1), //!< ConfigureRequest size deny.
    CFG_DENY_STACKING = (1L << 2), //!< ConfigureRequest stacking deny.

    CFG_DENY_ACTIVE_WINDOW = (1L << 3), //!< _NET_ACTIVE_WINDOW deny.

    CFG_DENY_STATE_MAXIMIZED_VERT = (1L << 4), //!< EWMH state maximized vert deny.
    CFG_DENY_STATE_MAXIMIZED_HORZ = (1L << 5), //!< EWMH state maximized horz deny.
    CFG_DENY_STATE_HIDDEN = (1L << 6), //!< EWMH state hidden deny.
    CFG_DENY_STATE_FULLSCREEN = (1L << 7), //!< EWMH state fullscreen deny.
    CFG_DENY_STATE_ABOVE = (1L << 8), //! EWMH state above deny.
    CFG_DENY_STATE_BELOW = (1L << 9), //! EWMH state below deny.

    CFG_DENY_STRUT = (1L << 10), //! _NET_WM_STRUT_HINT registration.

    CFG_DENY_NO = 0 //! No deny.
};


#endif // _PEKWM_HH_
