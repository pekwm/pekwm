//
// pekwm.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// aewm.hh for aewm++
// Copyright (C) 2002 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _PEKWM_HH_
#define _PEKWM_HH_

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xmd.h>
}

class Geometry {
public:
	Geometry() : x(0), y(0), width(1), height(1) { }
	Geometry(int _x, int _y, unsigned int _width, unsigned int _height) :
		x(_x), y(_y), width(_width), height(_height) { }
	~Geometry() { }

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

enum Layer {
	LAYER_DESKTOP =	0,
	LAYER_BELOW = 2,
	LAYER_NORMAL = 4,
	LAYER_ONTOP	= 6,
	LAYER_DOCK = 8,
	LAYER_ABOVE_DOCK = 10,
	LAYER_MENU = 12,
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
	SKIP_FRAME_SNAP = (1<<3),
	SKIP_NONE = 0
};

// If adding, make sure it "syncs" with the windowmanager cursor enum
// BAD HABIT
enum BorderPosition {
	BORDER_TOP_LEFT, BORDER_TOP, BORDER_TOP_RIGHT,
	BORDER_LEFT, BORDER_RIGHT,
	BORDER_BOTTOM_LEFT, BORDER_BOTTOM, BORDER_BOTTOM_RIGHT,
	BORDER_NO_POS
};

enum ButtonState {
	BUTTON_FOCUSED = 0, BUTTON_UNFOCUSED, BUTTON_PRESSED, BUTTON_NO_STATE
};

enum ImageType {
	IMAGE_TILED = 1, IMAGE_SCALED, IMAGE_TRANSPARENT, NO_IMAGETYPE = 0
};

// Extended Net Hints stuff
class NetWMStates {
public:
	NetWMStates() : modal(false), sticky(false),
									max_vert(false), max_horz(false), shaded(false),
									skip_taskbar(false), skip_pager(false),
									hidden(false), fullscreen(false),
									above(false), below(false) { }
	~NetWMStates() { }

	bool modal;
	bool sticky;
	bool max_vert, max_horz;
	bool shaded;
	bool skip_taskbar, skip_pager;
	bool hidden;
	bool fullscreen;
	bool above, below;
};

// Someday maybe we will have support for this net spec hint. =)
// enum NetWmMoveResize {
//	_NET_WM_MOVERESIZE_SIZE_TOPLEFT = 0,
//	_NET_WM_MOVERESIZE_SIZE_TOP = 1,
//	_NET_WM_MOVERESIZE_SIZE_TOPRIGHT = 2,
//	_NET_WM_MOVERESIZE_SIZE_RIGHT = 3,
//	_NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT = 4,
//	_NET_WM_MOVERESIZE_SIZE_BOTTOM = 5,
//	_NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT = 6,
//	_NET_WM_MOVERESIZE_SIZE_LEFT = 7,
//	_NET_WM_MOVERESIZE_MOVE = 8   // Movement only
// };

#define NET_WM_MAX_STATES 11
#define NET_WM_STICKY_WINDOW	0xffffffff

// Text Justify for window titlebars
enum TextJustify { LEFT_JUSTIFY, CENTER_JUSTIFY, RIGHT_JUSTIFY, NO_JUSTIFY };
enum PlacementModel { SMART, MOUSE_CENTERED, MOUSE_TOP_LEFT, NO_PLACEMENT };
enum HarbourPlacement { TOP, LEFT, RIGHT, BOTTOM, NO_HARBOUR_PLACEMENT };
enum ScreenEdgeType { SE_TOP, SE_LEFT, SE_RIGHT, SE_BOTTOM, SE_NO };
enum Edge { TOP_LEFT, TOP_EDGE, TOP_RIGHT, RIGHT_EDGE, BOTTOM_RIGHT, BOTTOM_EDGE, BOTTOM_LEFT, LEFT_EDGE, CENTER, NO_EDGE };
enum Raise { ALWAYS_RAISE, END_RAISE, NEVER_RAISE, NO_RAISE };
enum Orientation { TOP_TO_BOTTOM = 1, LEFT_TO_RIGHT = 1,
									 BOTTOM_TO_TOP = 2, RIGHT_TO_LEFT = 2, NO_ORIENTATION = 0 };
enum MouseButtonType {
	BUTTON_PRESS, BUTTON_RELEASE,
	BUTTON_DOUBLE, BUTTON_MOTION
};
enum ButtonNum {
	BUTTON1 = Button1, BUTTON2, BUTTON3, BUTTON4, BUTTON5,
	BUTTON6, BUTTON7, NUM_BUTTONS = BUTTON7
};
enum Focus {
	FOCUS_NEW = (1<<1),
	FOCUS_ENTER = (1<<2),
	FOCUS_LEAVE = (1<<3),
	FOCUS_CLICK = (1<<4),
	NO_FOCUS = 0
};

enum MenuType {
	WINDOWMENU_TYPE, ROOTMENU_TYPE,
	GOTOMENU_TYPE, ICONMENU_TYPE,
	ATTACH_CLIENT_TYPE, ATTACH_FRAME_TYPE,
	ATTACH_CLIENT_IN_FRAME_TYPE, ATTACH_FRAME_IN_FRAME_TYPE,
	DYNAMIC_MENU_TYPE, NO_MENU_TYPE
};

#endif // _PEKWM_HH_
