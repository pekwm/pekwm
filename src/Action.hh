//
// Action.hh for pekwm
// Copyright (C) 2003 Claes Nasten <pekdon at pekdon dot net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _ACTION_HH_
#define _ACTION_HH_

#include <list>
#include <string>

class Client;

// Enums
enum ActionOk {
	KEYGRABBER_OK = (1<<1),
	FRAME_OK = (1<<2),
	CLIENT_OK = (1<<3),
	ROOTCLICK_OK = (1<<4),
	BUTTONCLICK_OK = (1<<5),
	WINDOWMENU_OK = (1<<6),
	ROOTMENU_OK = (1<<7)
};

enum ActionType {
	MAXIMIZE = 1, MAXIMIZE_VERTICAL, MAXIMIZE_HORIZONTAL,
	RESIZE, MOVE_RESIZE,
	SHADE,
	STICK,
	RAISE, LOWER,
	ACTIVATE_OR_RAISE,
	CLOSE, KILL,
	ALWAYS_ON_TOP, ALWAYS_BELOW,
	TOGGLE_BORDER, TOGGLE_TITLEBAR, TOGGLE_DECOR,
	MOVE_TO_EDGE,
	NEXT_FRAME, 	PREV_FRAME,
	NEXT_IN_FRAME, PREV_IN_FRAME,
	MOVE_CLIENT_NEXT, MOVE_CLIENT_PREV,
	ACTIVATE_CLIENT, ACTIVATE_CLIENT_NUM,
	ICONIFY, UNICONIFY,
	NEXT_WORKSPACE, RIGHT_WORKSPACE,
	PREV_WORKSPACE, LEFT_WORKSPACE,
	SEND_TO_WORKSPACE,
	GO_TO_WORKSPACE,
	SHOW_MENU, HIDE_ALL_MENUS,
	DETACH,
	TOGGLE_TAG, TOGGLE_TAG_BEHIND, UNTAG,
	MARK_CLIENT, ATTACH_MARKED,
	ATTACH_CLIENT_IN_NEXT_FRAME, ATTACH_CLIENT_IN_PREV_FRAME,
	ATTACH_FRAME_IN_NEXT_FRAME, ATTACH_FRAME_IN_PREV_FRAME,
	EXEC,
	RELOAD,
	RESTART, RESTART_OTHER,
	EXIT,
#ifdef MENUS
	SUBMENU,
	DYNAMIC_MENU,
#endif // MENUS
	MOVE, GROUPING_DRAG,

	NO_ACTION = 0
};

enum MoveResizeActionType {
	MOVE_HORIZONTAL = 1, MOVE_VERTICAL,
	RESIZE_HORIZONTAL, RESIZE_VERTICAL,
	MOVE_SNAP,
	MOVE_CANCEL, MOVE_END,
	NO_MOVERESIZE_ACTION = 0
};

#ifdef MENUS
enum MenuActionType {
	MENU_NEXT = 1, MENU_PREV,
	MENU_SELECT,
	MENU_ENTER_SUBMENU, MENU_LEAVE_SUBMENU,
	MENU_CLOSE,
	NO_MENU_ACTION = 0
};
#endif // MENUS

// Structs and Classes

struct Action {
	unsigned int action;

	int param_i;
	std::string param_s;
};

class ActionEvent {
public:
	ActionEvent() { }
	~ActionEvent() { }

	inline bool isOnlyAction(unsigned int action) {
		if ((action_list.size() == 1) && (action_list.front().action == action))
			return true;
		return false;
	}

public:	
	unsigned int mod, sym; // event matching
	unsigned int type, threshold; // more matching, press, release etc

	std::list<Action> action_list;
};

class ActionPerformed {
public:
	ActionPerformed(ActionEvent* a, Client* c) : ae(a), client(c), type(0) { }
	~ActionPerformed() { }

	ActionEvent *ae;
	Client *client;

	int type;
	union _event {
		XButtonEvent *button;
		XKeyEvent *key;
		XMotionEvent *motion;
	} event;
};

#endif // _ACTION_HH_
