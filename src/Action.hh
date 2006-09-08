//
// Action.hh for pekwm
// Copyright (C) 2003-2006 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#include "../config.h"

#ifndef _ACTION_HH_
#define _ACTION_HH_

#include "Types.hh"

#include <list>
#include <string>

class PWinObj;

//! @brief Masks used to set Action context validity.
enum ActionOk {
    KEYGRABBER_OK = (1<<1), //!< Keygrabber ok.
    FRAME_OK = (1<<2), //!< Frame title ok.
    CLIENT_OK = (1<<3), //!< Client click ok.
    ROOTCLICK_OK = (1<<4), //!< Root window click ok.
    BUTTONCLICK_OK = (1<<5), //!< Button{Press,Release} ok.
    WINDOWMENU_OK = (1<<6), //!< Ok from WindowMenu.
    ROOTMENU_OK = (1<<7), //!< Ok from RootMenu.
    FRAME_BORDER_OK  = (1<<8), //!< Frame border ok.
    SCREEN_EDGE_OK = (1<<9) //!< ScreenEdge ok.
};

enum ActionType {
    ACTION_UNSET = 0, ACTION_SET = 1, // to conform with bool values
    ACTION_TOGGLE,

    ACTION_FOCUS, ACTION_UNFOCUS,

    ACTION_GROW_DIRECTION, ACTION_MAXFILL,
    ACTION_RESIZE, ACTION_MOVE_RESIZE,
    ACTION_RAISE, ACTION_LOWER,
    ACTION_ACTIVATE_OR_RAISE,
    ACTION_CLOSE, ACTION_CLOSE_FRAME, ACTION_KILL,
    ACTION_MOVE_TO_EDGE,
    ACTION_NEXT_FRAME, ACTION_NEXT_FRAME_MRU,
    ACTION_PREV_FRAME, ACTION_PREV_FRAME_MRU,
    ACTION_FOCUS_DIRECTIONAL,
    ACTION_ACTIVATE_CLIENT_REL, ACTION_MOVE_CLIENT_REL,
    ACTION_ACTIVATE_CLIENT, ACTION_ACTIVATE_CLIENT_NUM,
    ACTION_SEND_TO_WORKSPACE, ACTION_GOTO_WORKSPACE,
    ACTION_WARP_TO_WORKSPACE, ACTION_WARP_TO_VIEWPORT,
    ACTION_SEND_TO_VIEWPORT,
    ACTION_SHOW_MENU, ACTION_HIDE_ALL_MENUS,
    ACTION_DETACH,	ACTION_ATTACH_MARKED,
    ACTION_ATTACH_CLIENT_IN_NEXT_FRAME, ACTION_ATTACH_CLIENT_IN_PREV_FRAME,
    ACTION_ATTACH_FRAME_IN_NEXT_FRAME, ACTION_ATTACH_FRAME_IN_PREV_FRAME,

    ACTION_VIEWPORT_MOVE_XY, ACTION_VIEWPORT_MOVE_DRAG,
    ACTION_VIEWPORT_MOVE_DIRECTION, ACTION_VIEWPORT_SCROLL,
    ACTION_VIEWPORT_GOTO,
    ACTION_GOTO_CLIENT_ID, ACTION_FIND_CLIENT,

    ACTION_EXEC, ACTION_RELOAD, ACTION_RESTART,
    ACTION_RESTART_OTHER, ACTION_EXIT,
#ifdef MENUS
    ACTION_MENU_NEXT, ACTION_MENU_PREV, ACTION_MENU_SELECT,
    ACTION_MENU_ENTER_SUBMENU, ACTION_MENU_LEAVE_SUBMENU,

    ACTION_MENU_SUB,
    ACTION_MENU_DYN,
#endif // MENUS
    ACTION_MOVE, ACTION_GROUPING_DRAG,
    ACTION_SHOW_CMD_DIALOG,

    ACTION_NO
};

enum ActionStateType {
    ACTION_STATE_MAXIMIZED, ACTION_STATE_FULLSCREEN,
    ACTION_STATE_SHADED, ACTION_STATE_STICKY,
    ACTION_STATE_ALWAYS_ONTOP, ACTION_STATE_ALWAYS_BELOW,
    ACTION_STATE_DECOR_BORDER, ACTION_STATE_DECOR_TITLEBAR,
    ACTION_STATE_DECOR, ACTION_STATE_TITLE,
    ACTION_STATE_ICONIFIED, ACTION_STATE_TAGGED,
    ACTION_STATE_MARKED, ACTION_STATE_SKIP,
    ACTION_STATE_CFG_DENY,

#ifdef HARBOUR
    ACTION_STATE_HARBOUR_HIDDEN,
#endif // HARBOUR

    ACTION_STATE_GLOBAL_GROUPING,

    ACTION_STATE_NO
};

enum StateAction {
    STATE_SET = ACTION_SET,
    STATE_UNSET = ACTION_UNSET,
    STATE_TOGGLE = ACTION_TOGGLE
};

enum MoveResizeActionType {
    MOVE_HORIZONTAL = 1, MOVE_VERTICAL,
    RESIZE_HORIZONTAL, RESIZE_VERTICAL,
    MOVE_SNAP,
    MOVE_CANCEL, MOVE_END,
    NO_MOVERESIZE_ACTION = 0
};

enum CmdDialogAction {
    CMD_D_INSERT, CMD_D_REMOVE,
    CMD_D_CLEAR, CMD_D_CLEARFROMCURSOR, CMD_D_EXEC, CMD_D_CLOSE,
    CMD_D_COMPLETE,
    CMD_D_CURS_NEXT, CMD_D_CURS_PREV,
    CMD_D_CURS_END, CMD_D_CURS_BEGIN,
    CMD_D_HIST_NEXT, CMD_D_HIST_PREV,
    CMD_D_NO_ACTION
};

// Action Utils
namespace ActionUtil {
    //! @brief Determines if state needs toggling.
    //! @return true if state needs toggling, else false.
    inline bool needToggle(StateAction sa, bool state) {
        if (((state == true) && (sa == STATE_SET))
            || ((state == false) && (sa == STATE_UNSET))) {
            return false;
        }
        return true;
    }
}

// Structs and Classes

class Action {
public:
    Action(void) { }
    ~Action(void) { }

    inline uint getAction(void) const { return _action; }
inline int getParamI(uint n) const { return _param_i[(n < 3) ? n : 0]; }
    inline const std::string &getParamS(void) const { return _param_s; }

    inline void setAction(uint action) { _action = action; }
inline void setParamI(uint n, int param) { _param_i[(n < 3) ? n : 0] = param; }
    inline void setParamS(const std::string param) { _param_s = param; }

private:
    uint _action;

    int _param_i[3];
    std::string _param_s;
};

class ActionEvent {
public:
    ActionEvent(void) { }
    ~ActionEvent(void) { }

    inline bool isOnlyAction(uint action) const {
        if ((action_list.size() == 1) &&
                (action_list.front().getAction() == action)) {
            return true;
        }
        return false;
    }

public:
    uint mod, sym; // event matching
    uint type, threshold; // more matching, press, release etc

    std::list<Action> action_list;
};

class ActionPerformed {
public:
    ActionPerformed(PWinObj *w, const ActionEvent &a) : wo(w), ae(a), type(0) { }
    ~ActionPerformed(void) { }

    PWinObj *wo;
    const ActionEvent &ae;

    int type;
    union _event {
        XButtonEvent *button;
        XKeyEvent *key;
        XMotionEvent *motion;
        XCrossingEvent *crossing;
        XExposeEvent *expose;
    } event;
};

#endif // _ACTION_HH_
