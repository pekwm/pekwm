//
// pekwm.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
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

#include "config.h"

#include <vector>

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
        return ((x == gm.x) && (y == gm.y) &&
                (width == gm.width) && (height == gm.height));
    }
    inline bool operator != (const Geometry& gm) {
        return (x != gm.x) || (y != gm.y) ||
                (width != gm.width) || (height != gm.height);
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
enum AtomName {
    // Ewmh Atom Names
    NET_SUPPORTED,
    NET_CLIENT_LIST, NET_CLIENT_LIST_STACKING,
    NET_NUMBER_OF_DESKTOPS,
    NET_DESKTOP_GEOMETRY, NET_DESKTOP_VIEWPORT,
    NET_CURRENT_DESKTOP, NET_DESKTOP_NAMES,
    NET_ACTIVE_WINDOW, NET_WORKAREA,
    NET_DESKTOP_LAYOUT, NET_SUPPORTING_WM_CHECK,
    NET_CLOSE_WINDOW,
    NET_WM_MOVERESIZE,
    NET_WM_NAME, NET_WM_VISIBLE_NAME,
    NET_WM_ICON_NAME, NET_WM_VISIBLE_ICON_NAME,
    NET_WM_ICON, NET_WM_DESKTOP,
    NET_WM_STRUT, NET_WM_PID,
    NET_WM_WINDOW_OPACITY,

    WINDOW_TYPE,
    WINDOW_TYPE_DESKTOP, WINDOW_TYPE_DOCK,
    WINDOW_TYPE_TOOLBAR, WINDOW_TYPE_MENU,
    WINDOW_TYPE_UTILITY, WINDOW_TYPE_SPLASH,
    WINDOW_TYPE_DIALOG, WINDOW_TYPE_NORMAL,

    STATE,
    STATE_MODAL, STATE_STICKY,
    STATE_MAXIMIZED_VERT, STATE_MAXIMIZED_HORZ,
    STATE_SHADED,
    STATE_SKIP_TASKBAR, STATE_SKIP_PAGER,
    STATE_HIDDEN, STATE_FULLSCREEN,
    STATE_ABOVE, STATE_BELOW,
    STATE_DEMANDS_ATTENTION,

    EWMH_ALLOWED_ACTIONS,
    EWMH_ACTION_MOVE, EWMH_ACTION_RESIZE,
    EWMH_ACTION_MINIMIZE, EWMH_ACTION_SHADE,
    EWMH_ACTION_STICK,
    EWHM_ACTION_MAXIMIZE_VERT, EWMH_ACTION_MAXIMIZE_HORZ,
    EWMH_ACTION_FULLSCREEN, ACTION_CHANGE_DESKTOP,
    EWMH_ACTION_CLOSE,

    UTF8_STRING,  // When adding an ewmh atom after this,
                  // fix setEwmhAtomsSupport(Window)
    STRING, MANAGER,

    // pekwm atom names
    PEKWM_FRAME_ID,
    PEKWM_FRAME_ORDER,
    PEKWM_FRAME_ACTIVE,
    PEKWM_FRAME_DECOR,
    PEKWM_FRAME_SKIP,
    PEKWM_TITLE,

    // ICCCM Atom Names
    WM_NAME,
    WM_ICON_NAME,
    WM_HINTS,
    WM_CLASS,
    WM_STATE,
    WM_CHANGE_STATE,
    WM_PROTOCOLS,
    WM_DELETE_WINDOW,
    WM_COLORMAP_WINDOWS,
    WM_TAKE_FOCUS,
    WM_WINDOW_ROLE,
    WM_CLIENT_MACHINE,

    // List of non PEKWM, ICCCM and EWMH atoms.
    MOTIF_WM_HINTS,

    MAX_NR_ATOMS
};

/**
 * _NET_WM_DESKTOP_LAYOUT orientation.
 */
enum NetWmOrientation {
    NET_WM_ORIENTATION_HORZ = 0,
    NET_WM_ORIENTATION_VERT = 1
};

/**
 * _NET_WM_DESKTOP_LAYOUT starting order.
 */
enum NetWmStartingOrder {
    NET_WM_TOPLEFT = 0,
    NET_WM_TOPRIGHT = 1,
    NET_WM_BOTTOMRIGHT = 2,
    NET_WM_BOTTOMLEFT = 3
};

/**
 * _NET_WM_MOVERESIZE direction
 */
enum NetWmMoveResize {
    NET_WM_MOVERESIZE_SIZE_TOPLEFT    = 0,
    NET_WM_MOVERESIZE_SIZE_TOP        = 1,
    NET_WM_MOVERESIZE_SIZE_TOPRIGHT   = 2,
    NET_WM_MOVERESIZE_SIZE_RIGHT      = 3,
    NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT= 4,
    NET_WM_MOVERESIZE_SIZE_BOTTOM     = 5,
    NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT = 6,
    NET_WM_MOVERESIZE_SIZE_LEFT       = 7,
    NET_WM_MOVERESIZE_MOVE            = 8, /* movement only */
    NET_WM_MOVERESIZE_SIZE_KEYBOARD   = 9, /* size via keyboard */
    NET_WM_MOVERESIZE_MOVE_KEYBOARD   =10, /* move via keyboard */
    NET_WM_MOVERESIZE_CANCEL          =11  /* cancel operation */
};

enum Layer {
    LAYER_DESKTOP,
    LAYER_BELOW,
    LAYER_NORMAL,
    LAYER_ONTOP,
    LAYER_DOCK,
    LAYER_ABOVE_DOCK,
    LAYER_MENU,
    LAYER_NONE
};

enum ApplyOn {
    APPLY_ON_ALWAYS = 0,
    APPLY_ON_START = (1<<1),
    APPLY_ON_NEW = (1<<2),
    APPLY_ON_RELOAD = (1<<3),
    APPLY_ON_WORKSPACE = (1<<4),
    APPLY_ON_TRANSIENT = (1<<5),
    APPLY_ON_TRANSIENT_ONLY = (1<<6)
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
    CURSOR_RESIZE,
    CURSOR_NONE,
    MAX_NR_CURSOR
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
    WORKSPACE_NO = (uint) ~0, WORKSPACE_LEFT = WORKSPACE_NO - 13,
    WORKSPACE_PREV, WORKSPACE_NEXT, WORKSPACE_RIGHT,
    WORKSPACE_PREV_V, WORKSPACE_UP, WORKSPACE_NEXT_V, WORKSPACE_DOWN,
    WORKSPACE_LAST,
    WORKSPACE_PREV_N, WORKSPACE_NEXT_N, WORKSPACE_LEFT_N, WORKSPACE_RIGHT_N
};
enum OrientationType {
    TOP_LEFT, TOP_EDGE, TOP_CENTER_EDGE, TOP_RIGHT,
    BOTTOM_RIGHT, BOTTOM_EDGE, BOTTOM_CENTER_EDGE, BOTTOM_LEFT,
    LEFT_EDGE, LEFT_CENTER_EDGE, RIGHT_EDGE, RIGHT_CENTER_EDGE,
    CENTER, NO_EDGE
};
enum Raise {
    ALWAYS_RAISE, END_RAISE, NEVER_RAISE, TEMP_RAISE, NO_RAISE
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
    DYNAMIC_MENU_TYPE, GOTOCLIENTMENU_TYPE, NO_MENU_TYPE
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

namespace pekwm
{
    bool isStartup();
    void setIsStartup(bool is_startup);
}

#endif // _PEKWM_HH_
