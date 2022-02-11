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

#ifndef _PEKWM_PEKWM_HH_
#define _PEKWM_PEKWM_HH_

#include "config.h"

#include <string>
#include <vector>

#include "Compat.hh"
#include "AppCtrl.hh"
#include "Types.hh"
#include "Exception.hh"

class EventLoop;

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xmd.h>
}

// data types

#define NET_WM_STICKY_WINDOW 0xffffffff
#define EWMH_OPAQUE_WINDOW 0xffffffff

// enums


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

/**
 * Keep in sync with CursorType in X11.hh. Border order is
 * significant, reordering can casue theme rendering break.
 */
enum BorderPosition {
	BORDER_TOP_LEFT = 0,
	BORDER_TOP_RIGHT,
	BORDER_BOTTOM_LEFT,
	BORDER_BOTTOM_RIGHT,
	BORDER_TOP,
	BORDER_LEFT,
	BORDER_RIGHT,
	BORDER_BOTTOM,
	BORDER_NO_POS
};

enum ButtonState {
	BUTTON_STATE_FOCUSED,
	BUTTON_STATE_UNFOCUSED,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_HOVER,
	BUTTON_STATE_NO
};
enum FocusedState {
	FOCUSED_STATE_FOCUSED,
	FOCUSED_STATE_UNFOCUSED,
	FOCUSED_STATE_FOCUSED_SELECTED,
	FOCUSED_STATE_UNFOCUSED_SELECTED,
	FOCUSED_STATE_NO
};
enum DecorState {
	DECOR_TITLEBAR = (1<<1),
	DECOR_BORDER = (1<<2)
};

enum ImageType {
	IMAGE_TYPE_TILED = 1,
	IMAGE_TYPE_SCALED,
	IMAGE_TYPE_FIXED,
	IMAGE_TYPE_NO = 0
};

enum FontJustify {
	FONT_JUSTIFY_LEFT,
	FONT_JUSTIFY_CENTER,
	FONT_JUSTIFY_RIGHT,
	FONT_JUSTIFY_NO
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
	DIRECTION_NO, DIRECTION_IGNORED
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
	TOP_TO_BOTTOM = 1,
	LEFT_TO_RIGHT = 1,
	BOTTOM_TO_TOP = 2,
	RIGHT_TO_LEFT = 2,
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
	/** Motion event with button pressed. */
	MOUSE_EVENT_MOTION_PRESSED = (1<<8),
	MOUSE_EVENT_NO = 0
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
	MOUSE_ACTION_LIST_TITLE_FRAME,
	MOUSE_ACTION_LIST_TITLE_OTHER,
	MOUSE_ACTION_LIST_CHILD_FRAME,
	MOUSE_ACTION_LIST_CHILD_OTHER,
	MOUSE_ACTION_LIST_ROOT,
	MOUSE_ACTION_LIST_MENU,
	MOUSE_ACTION_LIST_OTHER,

	MOUSE_ACTION_LIST_EDGE_T,
	MOUSE_ACTION_LIST_EDGE_B,
	MOUSE_ACTION_LIST_EDGE_L,
	MOUSE_ACTION_LIST_EDGE_R,

	MOUSE_ACTION_LIST_BORDER_TL,
	MOUSE_ACTION_LIST_BORDER_T,
	MOUSE_ACTION_LIST_BORDER_TR,
	MOUSE_ACTION_LIST_BORDER_L,
	MOUSE_ACTION_LIST_BORDER_R,
	MOUSE_ACTION_LIST_BORDER_BL,
	MOUSE_ACTION_LIST_BORDER_B,
	MOUSE_ACTION_LIST_BORDER_BR,

	MOUSE_ACTION_LIST_NO = MOUSE_ACTION_LIST_BORDER_BR
};
enum CfgDeny {
	CFG_DENY_POSITION = (1L << 0), //!< ConfigureRequest position deny.
	CFG_DENY_SIZE = (1L << 1), //!< ConfigureRequest size deny.
	CFG_DENY_STACKING = (1L << 2), //!< ConfigureRequest stacking deny.

	CFG_DENY_ACTIVE_WINDOW = (1L << 3), //!< _NET_ACTIVE_WINDOW deny.

	/** EWMH state maximized vert deny. */
	CFG_DENY_STATE_MAXIMIZED_VERT = (1L << 4),
	/** EWMH state maximized horz deny. */
	CFG_DENY_STATE_MAXIMIZED_HORZ = (1L << 5),
	CFG_DENY_STATE_HIDDEN = (1L << 6), //!< EWMH state hidden deny.
	CFG_DENY_STATE_FULLSCREEN = (1L << 7), //!< EWMH state fullscreen deny.
	CFG_DENY_STATE_ABOVE = (1L << 8), //! EWMH state above deny.
	CFG_DENY_STATE_BELOW = (1L << 9), //! EWMH state below deny.

	CFG_DENY_STRUT = (1L << 10), //! _NET_WM_STRUT_HINT registration.
	/** Ignore the ResizeInc from the SizeHints */
	CFG_DENY_RESIZE_INC = (1L << 11),

	CFG_DENY_NO = 0 //! No deny.
};

enum CurrHeadSelector {
	CURR_HEAD_SELECTOR_CURSOR,
	CURR_HEAD_SELECTOR_FOCUSED_WINDOW,
	CURR_HEAD_SELECTOR_NO
};

enum FocusSelector {
	/** select window under pointer. */
	FOCUS_SELECTOR_POINTER,
	/** select previously foused window (on last workspace change) */
	FOCUS_SELECTOR_WORKSPACE_LAST_FOCUSED,
	/** select top-most window. */
	FOCUS_SELECTOR_TOP,
	/** select root window */
	FOCUS_SELECTOR_ROOT,
	/** not a valid focus selector */
	FOCUS_SELECTOR_NO
};

namespace pekwm
{
	void initNoDisplay(void);
	void cleanupNoDisplay(void);

	bool init(AppCtrl* app_ctrl, EventLoop* event_loop,
		  Display* dpy, const std::string& config_file,
		  bool replace, bool synchronous);
	void cleanup(void);

	bool isStarting(void);
	void setStarted(void);
}

#endif // _PEKWM_PEKWM_HH_
