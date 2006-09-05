//
// pekwm.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// aewm.hh for aewm++
// Copyright (C) 2002 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef _PEKWM_HH_
#define _PEKWM_HH_

#include <string>

#include <X11/Xlib.h>
#include <X11/Xmd.h>

// MOTIF hints
//const long MWM_HINTS_FUNCTIONS = (1l << 0);
const long MWM_HINTS_DECORATIONS = (1l << 1);

// These aren't used, should we classify them as obsolotet
// or maybe implement support for them someday?
// enum MwmwFunc {
//	MWM_FUNC_ALL = (1l << 0),
//	MWM_FUNC_RESIZE = (1l << 1),
//	MWM_FUNC_MOVE = (1l << 2),
//	MWM_FUNC_ICONIFY = (1l << 3),
//	MWM_FUNC_MAXIMIZE = (1l << 4),
//	MWM_FUNC_CLOSE = (1l << 5)
// };

enum MwmDecor {
	MWM_DECOR_ALL = (1l << 0),
	MWM_DECOR_BORDER = (1l << 1),
	MWM_DECOR_HANDLE = (1l << 2),
	MWM_DECOR_TITLE = (1l << 3),
	MWM_DECOR_MENU = (1l << 4),
	MWM_DECOR_ICONIFY = (1l << 5),
	MWM_DECOR_MAXIMIZE = (1l << 6)
};

#define MWM_HINT_ELEMENTS 3
struct MwmHints {
  unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
};

enum WindowLayer {
	WIN_LAYER_DESKTOP =	0,
	WIN_LAYER_BELOW = 2,
	WIN_LAYER_NORMAL = 4,
	WIN_LAYER_ONTOP	= 6,
	WIN_LAYER_DOCK = 8,
	WIN_LAYER_ABOVE_DOCK = 10,
	WIN_LAYER_MENU = 12
};

// Extended Net Hints stuff
class NetWMStates {
public:
	NetWMStates() : modal(false), sticky(false), max_vertical(false),
									max_horizontal(false), shaded(false),
									skip_taskbar(false), skip_pager(false),
									hidden(false), fullscreen(false) { }
	~NetWMStates() { }
	bool modal;
	bool sticky;
	bool max_vertical;
	bool max_horizontal;
	bool shaded;
	bool skip_taskbar;
	bool skip_pager; 
	bool hidden;
	bool fullscreen;
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

class Strut {
public:
	Strut() : east(0), west(0), north(0), south(0) { };
	~Strut() { };
public: // member variables
	CARD32 east;
	CARD32 west;
	CARD32 north;
	CARD32 south;
};

#define NET_WM_MAX_STATES 9
#define NET_WM_STICKY_WINDOW	0xffffffff

// Text Justify for window titlebars
enum TextJustify { LEFT_JUSTIFY, CENTER_JUSTIFY, RIGHT_JUSTIFY, NO_JUSTIFY };
enum FocusModel { FOCUS_FOLLOW, FOCUS_SLOPPY, FOCUS_CLICK, NO_FOCUS };
enum PlacementModel { SMART, MOUSE_CENTERED, MOUSE_TOP_LEFT, NO_PLACEMENT };
enum HarbourPlacement { TOP, LEFT, RIGHT, BOTTOM, NO_HARBOUR_PLACEMENT };
enum Corner { TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, NO_CORNER };
enum Orientation { TOP_TO_BOTTOM = 1, LEFT_TO_RIGHT = 1,
									 BOTTOM_TO_TOP = 2, RIGHT_TO_LEFT = 2, NO_ORIENTATION = 0 };

// Action related stuff
enum Actions {
	MAXIMIZE = 1,
	MAXIMIZE_VERTICAL,
	MAXIMIZE_HORIZONTAL,
	RESIZE,
	SHADE,
	STICK,
	RAISE,
	LOWER,
	CLOSE,
	KILL,
	ALWAYS_ON_TOP,
	ALWAYS_BELOW,
	TOGGLE_BORDER,
	TOGGLE_TITLEBAR,
	TOGGLE_DECOR,
	NUDGE_HORIZONTAL,
	NUDGE_VERTICAL,
	RESIZE_HORIZONTAL,
	RESIZE_VERTICAL,
	MOVE_TO_CORNER,
	NEXT_FRAME,
	NEXT_IN_FRAME,
	PREV_IN_FRAME,
	MOVE_CLIENT_NEXT,
	MOVE_CLIENT_PREV,
	ACTIVATE_CLIENT,
	ACTIVATE_CLIENT_NUM,
	ICONIFY,
	ICONIFY_GROUP,
	UNICONIFY,
	NEXT_WORKSPACE,
	RIGHT_WORKSPACE,
	PREV_WORKSPACE,
	LEFT_WORKSPACE,
	SEND_TO_WORKSPACE,
	GO_TO_WORKSPACE,
	SHOW_WINDOWMENU,
	SHOW_ROOTMENU,
	SHOW_ICONMENU,
	HIDE_ALL_MENUS,
	EXEC,
	RELOAD,
	RESTART,
	RESTART_OTHER,
	EXIT,
#ifdef MENUS
	SUBMENU,
#endif // MENUS
	MOVE,
	GROUPING_DRAG,
	NO_ACTION = 0
};

// About the default values for these, i_param _needs_ to be -1 for
// keygrabber parser to work ok.
class Action {
public:
	Action() : s_param(""), i_param(-1) { }
	~Action() { }

	Actions action;
	std::string s_param;
	int i_param;
};

enum MouseButtonType {
	BUTTON_SINGLE,
	BUTTON_DOUBLE,
	BUTTON_MOTION
};

class MouseButtonAction : public Action {
public:
	MouseButtonAction() : button(0), mod(0) { }
	~MouseButtonAction() { }
	unsigned int button;
	unsigned int mod;
	MouseButtonType type; // single, double click or motion?
};

#endif // _PEKWM_HH_
