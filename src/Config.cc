//
// Config.cc for pekwm
// Copyright (C) 2002-2015 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Config.hh"

#include "Compat.hh"
#include "PFont.hh"
#include "Util.hh"
#include "Workspaces.hh"
#include "x11.hh" // for DPY in keyconfig code

#include <iostream>
#include <fstream>

#include <cstdlib>

extern "C" {
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
}

using std::cerr;
using std::endl;
using std::string;
using std::wstring;
using std::map;
using std::vector;
using std::pair;
using std::ifstream;
using std::ofstream;
using std::strtol;
using std::getenv;

Config* Config::_instance = 0;

const int FRAME_MASK =
    FRAME_OK|FRAME_BORDER_OK|CLIENT_OK|WINDOWMENU_OK|
    KEYGRABBER_OK|BUTTONCLICK_OK;
const int ANY_MASK =
    KEYGRABBER_OK|FRAME_OK|FRAME_BORDER_OK|CLIENT_OK|ROOTCLICK_OK|
    BUTTONCLICK_OK|WINDOWMENU_OK|ROOTMENU_OK|SCREEN_EDGE_OK;

/**
 * Parse width and height limits.
 */
bool
SizeLimits::parse(const std::string &minimum, const std::string &maximum)
{
    return (parseLimit(minimum, _limits[WIDTH_MIN], _limits[HEIGHT_MIN])
            && parseLimit(maximum, _limits[WIDTH_MAX], _limits[HEIGHT_MAX]));
}

/**
 * Parse single limit.
 */
bool
SizeLimits::parseLimit(const std::string &limit, unsigned int &min, unsigned int &max)
{
    bool status = false;
    vector<string> tokens;
    if ((Util::splitString(limit, tokens, "x", 2, true)) == 2) {
        min = strtol(tokens[0].c_str(), 0, 10);
        max = strtol(tokens[1].c_str(), 0, 10);
        status = true;
    } else {
        min = 0;
        max = 0;
    }

    return status;
}

//! @brief Constructor for Config class
Config::Config(void) :
        _moveresize_edgeattract(0), _moveresize_edgeresist(0),
        _moveresize_woattract(0), _moveresize_woresist(0),
        _moveresize_opaquemove(0), _moveresize_opaqueresize(0),
        _screen_workspaces(4),
        _screen_workspaces_per_row(0), _screen_workspace_name_default(L"Workspace"),
        _screen_edge_indent(false),
        _screen_doubleclicktime(250), _screen_fullscreen_above(true),
        _screen_fullscreen_detect(true),
        _screen_showframelist(true),
        _screen_show_status_window(true), _screen_show_status_window_on_root(false),
        _screen_show_client_id(false),
        _screen_show_workspace_indicator(500), _screen_workspace_indicator_scale(16),
        _screen_workspace_indicator_opacity(EWMH_OPAQUE_WINDOW),
        _screen_place_new(true), _screen_focus_new(false),
        _screen_focus_new_child(true), _screen_focus_steal_protect(0), _screen_honour_randr(true),
        _screen_honour_aspectratio(true),
        _screen_placement_row(false),
        _screen_placement_ltr(true), _screen_placement_ttb(true),
        _screen_placement_offset_x(0), _screen_placement_offset_y(0),
        _place_trans_parent(true),
        _screen_client_unique_name(true),
        _screen_client_unique_name_pre(" #"), _screen_client_unique_name_post(""),
        _screen_report_all_clients(false),
        _menu_select_mask(0), _menu_enter_mask(0), _menu_exec_mask(0),
        _menu_display_icons(true),
        _menu_focus_opacity(EWMH_OPAQUE_WINDOW),
        _menu_unfocus_opacity(EWMH_OPAQUE_WINDOW),
        _cmd_dialog_history_unique(true), _cmd_dialog_history_size(1024),
        _cmd_dialog_history_file("~/.pekwm/history"), _cmd_dialog_history_save_interval(16),
        _harbour_da_min_s(0), _harbour_da_max_s(0),
        _harbour_ontop(true), _harbour_maximize_over(false),
        _harbour_placement(TOP), _harbour_orientation(TOP_TO_BOTTOM), _harbour_head_nr(0),
        _harbour_opacity(EWMH_OPAQUE_WINDOW)
{
    if (_instance) {
        throw string("Config, trying to create multiple instances");
    }
    _instance = this;

    for (uint i = 0; i <= SCREEN_EDGE_NO; ++i) {
        _screen_edge_sizes.push_back(0);
    }

    // fill parsing maps
    _action_map[""] = pair<ActionType, uint>(ACTION_NO, 0);
    _action_map["Focus"] = pair<ActionType, uint>(ACTION_FOCUS, ANY_MASK);
    _action_map["UnFocus"] = pair<ActionType, uint>(ACTION_UNFOCUS, ANY_MASK);

    _action_map["Set"] = pair<ActionType, uint>(ACTION_SET, ANY_MASK);
    _action_map["Unset"] = pair<ActionType, uint>(ACTION_UNSET, ANY_MASK);
    _action_map["Toggle"] = pair<ActionType, uint>(ACTION_TOGGLE, ANY_MASK);

    _action_map["MaxFill"] = pair<ActionType, uint>(ACTION_MAXFILL, FRAME_MASK);
    _action_map["GrowDirection"] = pair<ActionType, uint>(ACTION_GROW_DIRECTION, FRAME_MASK);
    _action_map["Close"] = pair<ActionType, uint>(ACTION_CLOSE, FRAME_MASK);
    _action_map["CloseFrame"] = pair<ActionType, uint>(ACTION_CLOSE_FRAME, FRAME_MASK);
    _action_map["Kill"] = pair<ActionType, uint>(ACTION_KILL, FRAME_MASK);
    _action_map["SetGeometry"] = pair<ActionType, uint>(ACTION_SET_GEOMETRY, FRAME_MASK);
    _action_map["Raise"] = pair<ActionType, uint>(ACTION_RAISE, FRAME_MASK);
    _action_map["Lower"] = pair<ActionType, uint>(ACTION_LOWER, FRAME_MASK);
    _action_map["ActivateOrRaise"] = pair<ActionType, uint>(ACTION_ACTIVATE_OR_RAISE, FRAME_MASK);
    _action_map["ActivateClientRel"] = pair<ActionType, uint>(ACTION_ACTIVATE_CLIENT_REL, FRAME_MASK);
    _action_map["MoveClientRel"] = pair<ActionType, uint>(ACTION_MOVE_CLIENT_REL, FRAME_MASK);
    _action_map["ActivateClient"] = pair<ActionType, uint>(ACTION_ACTIVATE_CLIENT, FRAME_MASK);
    _action_map["ActivateClientNum"] = pair<ActionType, uint>(ACTION_ACTIVATE_CLIENT_NUM, KEYGRABBER_OK);
    _action_map["Resize"] = pair<ActionType, uint>(ACTION_RESIZE, BUTTONCLICK_OK|CLIENT_OK|FRAME_OK|FRAME_BORDER_OK);
    _action_map["Move"] = pair<ActionType, uint>(ACTION_MOVE, FRAME_OK|FRAME_BORDER_OK|CLIENT_OK);
    _action_map["MoveResize"] = pair<ActionType, uint>(ACTION_MOVE_RESIZE, KEYGRABBER_OK);
    _action_map["GroupingDrag"] = pair<ActionType, uint>(ACTION_GROUPING_DRAG, FRAME_OK|CLIENT_OK);
    _action_map["WarpToWorkspace"] = pair<ActionType, uint>(ACTION_WARP_TO_WORKSPACE, SCREEN_EDGE_OK);
    _action_map["MoveToHead"] = pair<ActionType, uint>(ACTION_MOVE_TO_HEAD, FRAME_MASK);
    _action_map["MoveToEdge"] = pair<ActionType, uint>(ACTION_MOVE_TO_EDGE, KEYGRABBER_OK);
    _action_map["NextFrame"] = pair<ActionType, uint>(ACTION_NEXT_FRAME, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["PrevFrame"] = pair<ActionType, uint>(ACTION_PREV_FRAME, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["NextFrameMRU"] = pair<ActionType, uint>(ACTION_NEXT_FRAME_MRU, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["PrevFrameMRU"] = pair<ActionType, uint>(ACTION_PREV_FRAME_MRU, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["FocusDirectional"] = pair<ActionType, uint>(ACTION_FOCUS_DIRECTIONAL, FRAME_MASK);
    _action_map["AttachMarked"] = pair<ActionType, uint>(ACTION_ATTACH_MARKED, FRAME_MASK);
    _action_map["AttachClientInNextFrame"] = pair<ActionType, uint>(ACTION_ATTACH_CLIENT_IN_NEXT_FRAME, FRAME_MASK);
    _action_map["AttachClientInPrevFrame"] = pair<ActionType, uint>(ACTION_ATTACH_CLIENT_IN_PREV_FRAME, FRAME_MASK);
    _action_map["FindClient"] = pair<ActionType, uint>(ACTION_FIND_CLIENT, ANY_MASK);
    _action_map["GotoClientID"] = pair<ActionType, uint>(ACTION_GOTO_CLIENT_ID, ANY_MASK);
    _action_map["Detach"] = pair<ActionType, uint>(ACTION_DETACH, FRAME_MASK);
    _action_map["SendToWorkspace"] = pair<ActionType, uint>(ACTION_SEND_TO_WORKSPACE, ANY_MASK);
    _action_map["GoToWorkspace"] = pair<ActionType, uint>(ACTION_GOTO_WORKSPACE, ANY_MASK );
    _action_map["Exec"] = pair<ActionType, uint>(ACTION_EXEC, FRAME_MASK|ROOTMENU_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["Reload"] = pair<ActionType, uint>(ACTION_RELOAD, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["Restart"] = pair<ActionType, uint>(ACTION_RESTART, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["RestartOther"] = pair<ActionType, uint>(ACTION_RESTART_OTHER, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["Exit"] = pair<ActionType, uint>(ACTION_EXIT, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["ShowCmdDialog"] = pair<ActionType, uint>(ACTION_SHOW_CMD_DIALOG, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK|ROOTMENU_OK|WINDOWMENU_OK);
    _action_map["ShowSearchDialog"] = pair<ActionType, uint>(ACTION_SHOW_SEARCH_DIALOG, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK|ROOTMENU_OK|WINDOWMENU_OK);
    _action_map["ShowMenu"] = pair<ActionType, uint>(ACTION_SHOW_MENU, FRAME_MASK|ROOTCLICK_OK|SCREEN_EDGE_OK|ROOTMENU_OK|WINDOWMENU_OK);
    _action_map["HideAllMenus"] = pair<ActionType, uint>(ACTION_HIDE_ALL_MENUS, FRAME_MASK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["SubMenu"] = pair<ActionType, uint>(ACTION_MENU_SUB, ROOTMENU_OK|WINDOWMENU_OK);
    _action_map["Dynamic"] = pair<ActionType, uint>(ACTION_MENU_DYN, ROOTMENU_OK|WINDOWMENU_OK);
    _action_map["SendKey"] = pair<ActionType, uint>(ACTION_SEND_KEY, ANY_MASK);
    _action_map["SetOpacity"] = pair<ActionType, uint>(ACTION_SET_OPACITY, FRAME_MASK);
    _action_map["Debug"] = pair<ActionType, uint>(ACTION_DEBUG, ANY_MASK);

    _action_access_mask_map[""] = ACTION_ACCESS_NO;
    _action_access_mask_map["MOVE"] = ACTION_ACCESS_MOVE;
    _action_access_mask_map["RESIZE"] = ACTION_ACCESS_RESIZE;
    _action_access_mask_map["ICONIFY"] = ACTION_ACCESS_ICONIFY;
    _action_access_mask_map["SHADE"] = ACTION_ACCESS_SHADE;
    _action_access_mask_map["STICK"] = ACTION_ACCESS_STICK;
    _action_access_mask_map["MAXIMIZEHORIZONTAL"] = ACTION_ACCESS_MAXIMIZE_HORZ;
    _action_access_mask_map["MAXIMIZEVERTICAL"] = ACTION_ACCESS_MAXIMIZE_VERT;
    _action_access_mask_map["FULLSCREEN"] = ACTION_ACCESS_FULLSCREEN;
    _action_access_mask_map["SETWORKSPACE"] = ACTION_ACCESS_CHANGE_DESKTOP;
    _action_access_mask_map["CLOSE"] = ACTION_ACCESS_CLOSE;

    _edge_map[""] = NO_EDGE;
    _edge_map["TOPLEFT"] = TOP_LEFT;
    _edge_map["TOPEDGE"] = TOP_EDGE;
    _edge_map["TOPCENTEREDGE"] = TOP_CENTER_EDGE;
    _edge_map["TOPRIGHT"] = TOP_RIGHT;
    _edge_map["BOTTOMRIGHT"] = BOTTOM_RIGHT;
    _edge_map["BOTTOMEDGE"] = BOTTOM_EDGE;
    _edge_map["BOTTOMCENTEREDGE"] = BOTTOM_CENTER_EDGE;
    _edge_map["BOTTOMLEFT"] = BOTTOM_LEFT;
    _edge_map["LEFTEDGE"] = LEFT_EDGE;
    _edge_map["LEFTCENTEREDGE"] = LEFT_CENTER_EDGE;
    _edge_map["RIGHTEDGE"] = RIGHT_EDGE;
    _edge_map["RIGHTCENTEREDGE"] = RIGHT_CENTER_EDGE;
    _edge_map["CENTER"] = CENTER;

    _raise_map[""] = NO_RAISE;
    _raise_map["ALWAYSRAISE"] = ALWAYS_RAISE;
    _raise_map["ENDRAISE"] = END_RAISE;
    _raise_map["NEVERRAISE"] = NEVER_RAISE;
    _raise_map["TEMPRAISE"] = TEMP_RAISE;

    _skip_map[""] = SKIP_NONE;
    _skip_map["MENUS"] = SKIP_MENUS;
    _skip_map["FOCUSTOGGLE"] = SKIP_FOCUS_TOGGLE;
    _skip_map["SNAP"] = SKIP_SNAP;
    _skip_map["PAGER"] = SKIP_PAGER;
    _skip_map["TASKBAR"] = SKIP_TASKBAR;

    _layer_map[""] = LAYER_NONE;
    _layer_map["DESKTOP"] = LAYER_DESKTOP;
    _layer_map["BELOW"] = LAYER_BELOW;
    _layer_map["NORMAL"] = LAYER_NORMAL;
    _layer_map["ONTOP"] = LAYER_ONTOP;
    _layer_map["HARBOUR"] = LAYER_DOCK;
    _layer_map["ABOVEHARBOUR"] = LAYER_ABOVE_DOCK;
    _layer_map["MENU"] = LAYER_MENU;

    _moveresize_map[""] = NO_MOVERESIZE_ACTION;
    _moveresize_map["MOVEHORIZONTAL"] = MOVE_HORIZONTAL;
    _moveresize_map["MOVEVERTICAL"] = MOVE_VERTICAL;
    _moveresize_map["RESIZEHORIZONTAL"] = RESIZE_HORIZONTAL;
    _moveresize_map["RESIZEVERTICAL"] = RESIZE_VERTICAL;
    _moveresize_map["MOVESNAP"] = MOVE_SNAP;
    _moveresize_map["CANCEL"] = MOVE_CANCEL;
    _moveresize_map["END"] = MOVE_END;

    _inputdialog_map[""] = INPUT_NO_ACTION;
    _inputdialog_map["INSERT"] = INPUT_INSERT;
    _inputdialog_map["ERASE"] = INPUT_REMOVE;
    _inputdialog_map["CLEAR"] = INPUT_CLEAR;
    _inputdialog_map["CLEARFROMCURSOR"] = INPUT_CLEARFROMCURSOR;
    _inputdialog_map["EXEC"] = INPUT_EXEC;
    _inputdialog_map["CLOSE"] = INPUT_CLOSE;
    _inputdialog_map["COMPLETE"] = INPUT_COMPLETE;
    _inputdialog_map["COMPLETEABORT"] = INPUT_COMPLETE_ABORT;
    _inputdialog_map["CURSNEXT"] = INPUT_CURS_NEXT;
    _inputdialog_map["CURSPREV"] = INPUT_CURS_PREV;
    _inputdialog_map["CURSEND"] = INPUT_CURS_END;
    _inputdialog_map["CURSBEGIN"] = INPUT_CURS_BEGIN;
    _inputdialog_map["HISTNEXT"] = INPUT_HIST_NEXT;
    _inputdialog_map["HISTPREV"] = INPUT_HIST_PREV;

    _direction_map[""] = DIRECTION_NO;
    _direction_map["UP"] = DIRECTION_UP;
    _direction_map["DOWN"] = DIRECTION_DOWN;
    _direction_map["LEFT"] = DIRECTION_LEFT;
    _direction_map["RIGHT"] = DIRECTION_RIGHT;

    _workspace_change_map[""] = WORKSPACE_NO;
    _workspace_change_map["LEFT"] = WORKSPACE_LEFT;
    _workspace_change_map["LEFTN"] = WORKSPACE_LEFT_N;
    _workspace_change_map["PREV"] = WORKSPACE_PREV;
    _workspace_change_map["PREVN"] = WORKSPACE_PREV_N;
    _workspace_change_map["RIGHT"] = WORKSPACE_RIGHT;
    _workspace_change_map["RIGHTN"] = WORKSPACE_RIGHT_N;
    _workspace_change_map["NEXT"] = WORKSPACE_NEXT;
    _workspace_change_map["NEXTN"] = WORKSPACE_NEXT_N;
    _workspace_change_map["PREVV"] = WORKSPACE_PREV_V;
    _workspace_change_map["UP"] = WORKSPACE_UP;
    _workspace_change_map["NEXTV"] = WORKSPACE_NEXT_V;
    _workspace_change_map["DOWN"] = WORKSPACE_DOWN;
    _workspace_change_map["LAST"] = WORKSPACE_LAST;

    _borderpos_map[""] = BORDER_NO_POS;
    _borderpos_map["TOPLEFT"] = BORDER_TOP_LEFT;
    _borderpos_map["TOP"] = BORDER_TOP;
    _borderpos_map["TOPRIGHT"] = BORDER_TOP_RIGHT;
    _borderpos_map["LEFT"] = BORDER_LEFT;
    _borderpos_map["RIGHT"] = BORDER_RIGHT;
    _borderpos_map["BOTTOMLEFT"] = BORDER_BOTTOM_LEFT;
    _borderpos_map["BOTTOM"] = BORDER_BOTTOM;
    _borderpos_map["BOTTOMRIGHT"] = BORDER_BOTTOM_RIGHT;

    _mouse_event_map[""] = MOUSE_EVENT_NO;
    _mouse_event_map["BUTTONPRESS"] = MOUSE_EVENT_PRESS;
    _mouse_event_map["BUTTONRELEASE"] = MOUSE_EVENT_RELEASE;
    _mouse_event_map["DOUBLECLICK"] = MOUSE_EVENT_DOUBLE;
    _mouse_event_map["MOTION"] = MOUSE_EVENT_MOTION;
    _mouse_event_map["ENTER"] = MOUSE_EVENT_ENTER;
    _mouse_event_map["LEAVE"] = MOUSE_EVENT_LEAVE;
    _mouse_event_map["ENTERMOVING"] = MOUSE_EVENT_ENTER_MOVING;
    _mouse_event_map["MOTIONPRESSED"] = MOUSE_EVENT_MOTION_PRESSED;

    _mod_map[""] = 0;
    _mod_map["NONE"] = 0;
    _mod_map["SHIFT"] = ShiftMask;
    _mod_map["CTRL"] = ControlMask;
    _mod_map["MOD1"] = Mod1Mask;
    _mod_map["MOD2"] = Mod2Mask;
    _mod_map["MOD3"] = Mod3Mask;
    _mod_map["MOD4"] = Mod4Mask;
    _mod_map["MOD5"] = Mod5Mask;
    _mod_map["ANY"] = MOD_ANY;

    _action_state_map[""] = ACTION_STATE_NO;
    _action_state_map["Maximized"] = ACTION_STATE_MAXIMIZED;
    _action_state_map["Fullscreen"] = ACTION_STATE_FULLSCREEN;
    _action_state_map["Shaded"] = ACTION_STATE_SHADED;
    _action_state_map["Sticky"] = ACTION_STATE_STICKY;
    _action_state_map["AlwaysOnTop"] = ACTION_STATE_ALWAYS_ONTOP;
    _action_state_map["AlwaysBelow"] = ACTION_STATE_ALWAYS_BELOW;
    _action_state_map["Decor"] = ACTION_STATE_DECOR;
    _action_state_map["DecorBorder"] = ACTION_STATE_DECOR_BORDER;
    _action_state_map["DecorTitlebar"] = ACTION_STATE_DECOR_TITLEBAR;
    _action_state_map["Iconified"] = ACTION_STATE_ICONIFIED;
    _action_state_map["Tagged"] = ACTION_STATE_TAGGED;
    _action_state_map["Marked"] = ACTION_STATE_MARKED;
    _action_state_map["Skip"] = ACTION_STATE_SKIP;
    _action_state_map["CfgDeny"] = ACTION_STATE_CFG_DENY;
    _action_state_map["Opaque"] = ACTION_STATE_OPAQUE;
    _action_state_map["Title"] = ACTION_STATE_TITLE;
    _action_state_map["HarbourHidden"] = ACTION_STATE_HARBOUR_HIDDEN;
    _action_state_map["GlobalGrouping"] = ACTION_STATE_GLOBAL_GROUPING;


    _cfg_deny_map["POSITION"] = CFG_DENY_POSITION;
    _cfg_deny_map["SIZE"] = CFG_DENY_SIZE;
    _cfg_deny_map["STACKING"] = CFG_DENY_STACKING;
    _cfg_deny_map["ACTIVEWINDOW"] = CFG_DENY_ACTIVE_WINDOW;
    _cfg_deny_map["MAXIMIZEDVERT"] = CFG_DENY_STATE_MAXIMIZED_VERT;
    _cfg_deny_map["MAXIMIZEDHORZ"] = CFG_DENY_STATE_MAXIMIZED_HORZ;
    _cfg_deny_map["HIDDEN"] = CFG_DENY_STATE_HIDDEN;
    _cfg_deny_map["FULLSCREEN"] = CFG_DENY_STATE_FULLSCREEN;
    _cfg_deny_map["ABOVE"] = CFG_DENY_STATE_ABOVE;
    _cfg_deny_map["BELOW"] = CFG_DENY_STATE_BELOW;
    _cfg_deny_map["STRUT"] = CFG_DENY_STRUT;

    _menu_action_map[""] = ACTION_MENU_NEXT;
    _menu_action_map["NEXTITEM"] = ACTION_MENU_NEXT;
    _menu_action_map["PREVITEM"] = ACTION_MENU_PREV;
    _menu_action_map["SELECT"] = ACTION_MENU_SELECT;
    _menu_action_map["ENTERSUBMENU"] = ACTION_MENU_ENTER_SUBMENU;
    _menu_action_map["LEAVESUBMENU"] = ACTION_MENU_LEAVE_SUBMENU;
    _menu_action_map["CLOSE"] = ACTION_CLOSE;

    _harbour_placement_map[""] = NO_HARBOUR_PLACEMENT;
    _harbour_placement_map["TOP"] = TOP;
    _harbour_placement_map["LEFT"] = LEFT;
    _harbour_placement_map["RIGHT"] = RIGHT;
    _harbour_placement_map["BOTTOM"] = BOTTOM;

    _harbour_orientation_map[""] = NO_ORIENTATION;
    _harbour_orientation_map["TOPTOBOTTOM"] = TOP_TO_BOTTOM;
    _harbour_orientation_map["LEFTTORIGHT"] = TOP_TO_BOTTOM;
    _harbour_orientation_map["BOTTOMTOTOP"] = BOTTOM_TO_TOP;
    _harbour_orientation_map["RIGHTTOLEFT"] = BOTTOM_TO_TOP;

    // fill the mouse action map
    _mouse_action_map[MOUSE_ACTION_LIST_TITLE_FRAME] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_TITLE_OTHER] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_CHILD_FRAME] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_CHILD_OTHER] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_ROOT] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_MENU] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_OTHER] = new vector<ActionEvent>;

    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_T] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_B] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_L] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_R] = new vector<ActionEvent>;

    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_TL] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_T] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_TR] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_L] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_R] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_BL] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_B] = new vector<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_BR] = new vector<ActionEvent>;
}

//! @brief Destructor for Config class
Config::~Config(void)
{
    _instance = 0;

    map<MouseActionListName, vector<ActionEvent>* >::iterator it;
    for (it = _mouse_action_map.begin(); it != _mouse_action_map.end(); ++it) {
        delete it->second;
    }
}

/**
 * Returns an array of NULL-terminated desktop names in UTF-8.
 *
 * @param names *names will be set to an array of desktop names or 0. The caller has to delete [] *names
 * @param length *length will be set to the complete length of array *names points to or 0.
 */
void
Config::getDesktopNamesUTF8(uchar **names, uint *length) const
{
    if (! _screen_workspace_names.size()) {
        *names = 0;
        *length = 0;
        return;
    }

    // Convert strings to UTF-8 and calculate total length
    string utf8_names;
    vector<wstring>::const_iterator it(_screen_workspace_names.begin());
    for (; it != _screen_workspace_names.end(); ++it) {
        string utf8_name(Util::to_utf8_str(*it));
        utf8_names.append(utf8_name.c_str(), utf8_name.size() + 1);
    }

    *names = new uchar[utf8_names.size()];
    ::memcpy(*names, utf8_names.c_str(), utf8_names.size());
    *length = utf8_names.size();
}

/**
 * Sets the desktop names.
 *
 * @param names names is expected to point to an array of NULL-terminated utf8-strings.
 * @param length The length of the array "names".
 */
void
Config::setDesktopNamesUTF8(char *names, ulong length)
{
    _screen_workspace_names.clear();

    if (! names || ! length) {
        return;
    }

    for (ulong i = 0; i < length;) {
        _screen_workspace_names.push_back(Util::from_utf8_str(names));
        i += strlen(names) + 1;
        names += strlen(names) + 1;
    }
}

//! @brief Tries to load config_file, ~/.pekwm/config, SYSCONFDIR/config
bool
Config::load(const std::string &config_file)
{
    if (! _cfg_files.requireReload(config_file)) {
        return false;
    }

    CfgParser cfg;

    _config_file = config_file;
    bool success = tryHardLoadConfig(cfg, _config_file);

    // Make sure config is reloaded next time as content is dynamically
    // generated from the configuration file.
    if (! success || cfg.isDynamicContent()) {
        _cfg_files.clear();
    } else {
        _cfg_files = cfg.getCfgFiles();
    }

    if (! success) {
        cerr << " *** WARNING: unable to load configuration files!" << endl;
        return false;
    }

    // Update PEKWM_CONFIG_FILE environment if needed (to reflect active file)
    char *cfg_env = getenv("PEKWM_CONFIG_FILE");
    if (! cfg_env || (strcmp(cfg_env, _config_file.c_str()) != 0)) {
        setenv("PEKWM_CONFIG_FILE", _config_file.c_str(), 1);
    }

    string o_file_mouse; // temporary filepath for mouseconfig

    CfgParser::Entry *section;

    // Get other config files dests.
    section = cfg.getEntryRoot()->findSection("FILES");
    if (section) {
        loadFiles(section);
    }

    // Parse moving / resizing options.
    section = cfg.getEntryRoot()->findSection("MOVERESIZE");
    if (section) {
        loadMoveResize(section);
    }

    // Screen, important stuff such as number of workspaces
    section = cfg.getEntryRoot()->findSection("SCREEN");
    if (section) {
        loadScreen(section);
    }

    section = cfg.getEntryRoot()->findSection("MENU");
    if (section) {
        loadMenu(section);
    }

    section = cfg.getEntryRoot()->findSection("CMDDIALOG");
    if (section) {
      loadCmdDialog(section);
    }

    section = cfg.getEntryRoot()->findSection("HARBOUR");
    if (section) {
        loadHarbour(section);
    }

    return true;
}

//! @brief Loads file section of configuration
//! @param section Pointer to FILES section.
void
Config::loadFiles(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyPath("KEYS", _files_keys, SYSCONFDIR "/keys"));
    keys.push_back(new CfgParserKeyPath("MOUSE", _files_mouse, SYSCONFDIR "/mouse"));
    keys.push_back(new CfgParserKeyPath("MENU", _files_menu, SYSCONFDIR "/menu"));
    keys.push_back(new CfgParserKeyPath("START", _files_start, SYSCONFDIR "/start"));
    keys.push_back(new CfgParserKeyPath("AUTOPROPS", _files_autoprops, SYSCONFDIR "/autoproperties"));
    keys.push_back(new CfgParserKeyPath("THEME", _files_theme, DATADIR "/pekwm/themes/default/theme"));
    keys.push_back(new CfgParserKeyPath("ICONS", _files_icon_path, DATADIR "/pekwm/icons"));

    // Parse
    section->parseKeyValues(keys.begin(), keys.end());

    // Free up resources
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
}

//! @brief Loads MOVERESIZE section of main configuration
//! @param section Pointer to MOVERESIZE section.
void
Config::loadMoveResize(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyNumeric<int>("EDGEATTRACT", _moveresize_edgeattract, 0, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("EDGERESIST", _moveresize_edgeresist, 0, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WINDOWATTRACT",_moveresize_woattract, 0, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WINDOWRESIST", _moveresize_woresist, 0, 0));
    keys.push_back(new CfgParserKeyBool("OPAQUEMOVE", _moveresize_opaquemove));
    keys.push_back(new CfgParserKeyBool("OPAQUERESIZE", _moveresize_opaqueresize));

    // Parse data
    section->parseKeyValues(keys.begin(), keys.end());

    // Free up resources
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
}

//! @brief Loads SCREEN section of main configuration
//! @param section Pointer to SCREEN section.
void
Config::loadScreen(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    // Parse data
    string edge_size, workspace_names, trim_title;
    CfgParser::Entry *value;

    vector<CfgParserKey*> keys;
    keys.push_back(new CfgParserKeyNumeric<int>("WORKSPACES", _screen_workspaces, 4, 1));
    keys.push_back(new CfgParserKeyNumeric<int>("WORKSPACESPERROW", _screen_workspaces_per_row, 0, 0));
    keys.push_back(new CfgParserKeyString("WORKSPACENAMES", workspace_names));
    keys.push_back(new CfgParserKeyString("EDGESIZE", edge_size));
    keys.push_back(new CfgParserKeyBool("EDGEINDENT", _screen_edge_indent));
    keys.push_back(new CfgParserKeyNumeric<int>("DOUBLECLICKTIME", _screen_doubleclicktime, 250, 0));
    keys.push_back(new CfgParserKeyString("TRIMTITLE", trim_title));
    keys.push_back(new CfgParserKeyBool("FULLSCREENABOVE", _screen_fullscreen_above, true));
    keys.push_back(new CfgParserKeyBool("FULLSCREENDETECT", _screen_fullscreen_detect, true));
    keys.push_back(new CfgParserKeyBool("SHOWFRAMELIST", _screen_showframelist));
    keys.push_back(new CfgParserKeyBool("SHOWSTATUSWINDOW", _screen_show_status_window));
    keys.push_back(new CfgParserKeyBool("SHOWSTATUSWINDOWCENTEREDONROOT", _screen_show_status_window_on_root, false));
    keys.push_back(new CfgParserKeyBool("SHOWCLIENTID", _screen_show_client_id));
    keys.push_back(new CfgParserKeyNumeric<int>("SHOWWORKSPACEINDICATOR",
                                           _screen_show_workspace_indicator, 500, 0));
    keys.push_back(new CfgParserKeyNumeric<int>("WORKSPACEINDICATORSCALE",
                                           _screen_workspace_indicator_scale, 16, 2)); 
    keys.push_back(new CfgParserKeyNumeric<uint>("WORKSPACEINDICATOROPACITY",
                                           _screen_workspace_indicator_opacity, 100, 0, 100));
    keys.push_back(new CfgParserKeyBool("PLACENEW", _screen_place_new));
    keys.push_back(new CfgParserKeyBool("FOCUSNEW", _screen_focus_new));
    keys.push_back(new CfgParserKeyBool("FOCUSNEWCHILD", _screen_focus_new_child, true));
    keys.push_back(new CfgParserKeyNumeric<uint>("FOCUSSTEALPROTECT", _screen_focus_steal_protect, 0));
    keys.push_back(new CfgParserKeyBool("HONOURRANDR", _screen_honour_randr, true));
    keys.push_back(new CfgParserKeyBool("HONOURASPECTRATIO", _screen_honour_aspectratio, true));
    keys.push_back(new CfgParserKeyBool("REPORTALLCLIENTS", _screen_report_all_clients, false));

    // Parse data
    section->parseKeyValues(keys.begin(), keys.end());

    // Free up resources
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
    keys.clear();

    PFont::setTrimString(trim_title);

    // Convert opacity from percent to absolute value
    CONV_OPACITY(_screen_workspace_indicator_opacity);

    int edge_size_all = 0;
    _screen_edge_sizes.clear();
    if (edge_size.size()) {
        vector<string> sizes;
        if (Util::splitString(edge_size, sizes, " \t", 4) == 4) {
            for (vector<string>::iterator it(sizes.begin()); it != sizes.end(); ++it) {
                _screen_edge_sizes.push_back(strtol(it->c_str(), 0, 10));
            }
        } else {
            edge_size_all = strtol(edge_size.c_str(), 0, 10);
        }
    }

    for (uint i = 0; i < SCREEN_EDGE_NO; ++i) {
        _screen_edge_sizes.push_back(edge_size_all);
    }
    // Add SCREEN_EDGE_NO to the list for safety
    _screen_edge_sizes.push_back(0);

    // Workspace names
    _screen_workspace_names.clear();

    vector<string> vs;
    if (Util::splitString(workspace_names, vs, ";", 0, true)) {
        vector<string>::iterator vs_it(vs.begin());
        for (; vs_it != vs.end(); ++vs_it) {
            _screen_workspace_names.push_back(Util::to_wide_str(*vs_it));
        }
    }

    CfgParser::Entry *sub = section->findSection("PLACEMENT");
    if (sub) {
        value = sub->findEntry("MODEL");
        if (value) {
            Workspace::setDefaultLayouter(value->getValue());
        }

        CfgParser::Entry *sub_2 = sub->findSection("SMART");
        if (sub_2) {
            keys.push_back(new CfgParserKeyBool("ROW", _screen_placement_row));
            keys.push_back(new CfgParserKeyBool("LEFTTORIGHT", _screen_placement_ltr));
            keys.push_back(new CfgParserKeyBool("TOPTOBOTTOM", _screen_placement_ttb));
            keys.push_back(new CfgParserKeyNumeric<int>("OFFSETX", _screen_placement_offset_x, 0, 0));
            keys.push_back(new CfgParserKeyNumeric<int>("OFFSETY", _screen_placement_offset_y, 0, 0));

            // Do the parsing
            sub_2->parseKeyValues(keys.begin(), keys.end());

            // Freeup resources
            for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
            keys.clear();
        }

        keys.push_back(new CfgParserKeyBool("TRANSIENTONPARENT", _place_trans_parent, true));
        sub->parseKeyValues(keys.begin(), keys.end());
        for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
        keys.clear();
    }

    sub = section->findSection("UNIQUENAMES");
    if (sub) {
        keys.push_back(new CfgParserKeyBool("SETUNIQUE", _screen_client_unique_name));
        keys.push_back(new CfgParserKeyString("PRE", _screen_client_unique_name_pre));
        keys.push_back(new CfgParserKeyString("POST", _screen_client_unique_name_post));

        // Parse data
        sub->parseKeyValues(keys.begin(), keys.end());

        // Free up resources
        for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
        keys.clear();
    }
}

//! @brief Loads the MENU section of the main configuration
//! @param section Pointer to MENU section
void
Config::loadMenu(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    vector<CfgParserKey*> keys;
    string value_select, value_enter, value_exec;

    keys.push_back(new CfgParserKeyString("SELECT", value_select, "MOTION", 0));
    keys.push_back(new CfgParserKeyString("ENTER", value_enter, "BUTTONPRESS", 0));
    keys.push_back(new CfgParserKeyString("EXEC", value_exec, "BUTTONRELEASE", 0));
    keys.push_back(new CfgParserKeyBool("DISPLAYICONS", _menu_display_icons, true));
    keys.push_back(new CfgParserKeyNumeric<uint>("FOCUSOPACITY", _menu_focus_opacity, 100, 0, 100));
    keys.push_back(new CfgParserKeyNumeric<uint>("UNFOCUSOPACITY", _menu_unfocus_opacity, 100, 0, 100));

    // Parse data
    section->parseKeyValues(keys.begin(), keys.end());

    _menu_select_mask = getMenuMask(value_select);
    _menu_enter_mask = getMenuMask(value_enter);
    _menu_exec_mask = getMenuMask(value_exec);

    // Free up resources
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
    keys.clear();

    // Parse icon size limits
    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if (*(*it) == "ICONS") {
            loadMenuIcons((*it)->getSection());
        }
    }

    // Convert opacity from percent to absolute value
    CONV_OPACITY(_menu_focus_opacity);
    CONV_OPACITY(_menu_unfocus_opacity);
}

/**
 * Load Icon size limits for menu.
 */
void
Config::loadMenuIcons(CfgParser::Entry *section)
{
    if (! section || ! section->getValue().size()) {
        return;
    }

    vector<CfgParserKey*> keys;
    string minimum, maximum;

    keys.push_back(new CfgParserKeyString("MINIMUM", minimum, "16x16", 3));
    keys.push_back(new CfgParserKeyString("MAXIMUM", maximum, "16x16", 3));

    // Parse data
    section->parseKeyValues(keys.begin(), keys.end());

    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());

    SizeLimits limits;
    if (limits.parse(minimum, maximum)) {
        _menu_icon_limits[section->getValue()] = limits;
    }
}

/**
 * Load configuration from CmdDialog section.
 */
void
Config::loadCmdDialog(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    vector<CfgParserKey*> keys;

    keys.push_back(new CfgParserKeyBool("HISTORYUNIQUE", _cmd_dialog_history_unique));
    keys.push_back(new CfgParserKeyNumeric<int>("HISTORYSIZE", _cmd_dialog_history_size, 1024, 1));
    keys.push_back(new CfgParserKeyPath("HISTORYFILE", _cmd_dialog_history_file, "~/.pekwm/history"));
    keys.push_back(new CfgParserKeyNumeric<int>("HISTORYSAVEINTERVAL", _cmd_dialog_history_save_interval, 16, 0));

    section->parseKeyValues(keys.begin(), keys.end());

    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
}

//! @brief Loads the HARBOUR section of the main configuration
void
Config::loadHarbour(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    vector<CfgParserKey*> keys;
    string value_placement, value_orientation;

    keys.push_back(new CfgParserKeyBool("ONTOP", _harbour_ontop, true));
    keys.push_back(new CfgParserKeyBool("MAXIMIZEOVER", _harbour_maximize_over, false));
    keys.push_back(new CfgParserKeyNumeric<int>("HEAD", _harbour_head_nr, 0, 0));
    keys.push_back(new CfgParserKeyString("PLACEMENT", value_placement, "RIGHT", 0));
    keys.push_back(new CfgParserKeyString("ORIENTATION", value_orientation, "TOPTOBOTTOM", 0));
    keys.push_back(new CfgParserKeyNumeric<uint>("OPACITY", _harbour_opacity, 100, 0, 100));

    // Parse data
    section->parseKeyValues(keys.begin(), keys.end());

    // Free up resources
    for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
    keys.clear();

    // Convert opacity from percent to absolute value
    CONV_OPACITY(_harbour_opacity);

    _harbour_placement = ParseUtil::getValue<HarbourPlacement>(value_placement, _harbour_placement_map);
    _harbour_orientation = ParseUtil::getValue<Orientation>(value_orientation, _harbour_orientation_map);
    if (_harbour_placement == NO_HARBOUR_PLACEMENT) {
        _harbour_placement = RIGHT;
    }
    if (_harbour_orientation == NO_ORIENTATION) {
        _harbour_orientation = TOP_TO_BOTTOM;
    }

    CfgParser::Entry *sub = section->findSection("DOCKAPP");
    if (sub) {
        keys.push_back(new CfgParserKeyNumeric<int>("SIDEMIN", _harbour_da_min_s, 64, 0));
        keys.push_back(new CfgParserKeyNumeric<int>("SIDEMAX", _harbour_da_max_s, 64, 0));

        // Parse data
        sub->parseKeyValues(keys.begin(), keys.end());

        // Free up resources
        for_each(keys.begin(), keys.end(), Util::Free<CfgParserKey*>());
        keys.clear();
    }
}

ActionType
Config::getAction(const std::string &name, uint mask)
{
    pair<ActionType, uint> val(ParseUtil::getValue<pair<ActionType, uint> >(name, _action_map));

    if ((val.first != ACTION_NO) && (val.second&mask)) {
        return val.first;
    }
    return ACTION_NO;
}

ActionAccessMask
Config::getActionAccessMask(const std::string &name)
{
    ActionAccessMask mask = ParseUtil::getValue<ActionAccessMask>(name, _action_access_mask_map);
    return mask;
}

bool
Config::parseKey(const std::string &key_string, uint &mod, uint &key)
{
    // used for parsing
    vector<string> tok;
    vector<string>::iterator it;

    uint num;

    // chop the string up separating mods and the end key/button
    if (Util::splitString(key_string, tok, " \t")) {
        num = tok.size() - 1;
        if ((tok[num].size() > 1) && (tok[num][0] == '#')) {
            key = strtol(tok[num].c_str() + 1, 0, 10);
        } else if (strcasecmp(tok[num].c_str(), "ANY") == 0) {
            // Do no matching, anything goes.
            key = 0;
        } else {
            KeySym keysym = XStringToKeysym(tok[num].c_str());

            // XStringToKeysym() may fail. Perhaps we have luck after some
            // simple transformations. First we convert the string to lowercase
            // and try again. Then we try with only the first character in
            // uppercase and at last we try a complete uppercase string. If all
            // fails, we print a warning and return false.
            if (keysym == NoSymbol) {
                string str = tok[num];
                Util::to_lower(str);
                keysym = XStringToKeysym(str.c_str());
                if (keysym == NoSymbol) {
                    str[0] = ::toupper(str[0]);
                    keysym = XStringToKeysym(str.c_str());
                    if (keysym == NoSymbol) {
                        Util::to_upper(str);
                        keysym = XStringToKeysym(str.c_str());
                        if (keysym == NoSymbol) {
                            cerr << " *** WARNING: Couldn't find keysym for " << tok[num] << endl;
                            return false;
                        }
                    }
                }
            }
            key = XKeysymToKeycode(X11::getDpy(), keysym);
        }

        // if the last token isn't an key/button, the action isn't valid
        if ((key != 0) || (strcasecmp(tok[num].c_str(), "ANY") == 0)) {
            tok.pop_back(); // remove the key/button

            // add the modifier
            mod = 0;
            for (it = tok.begin(); it != tok.end(); ++it) {
                mod |= getMod(*it);
            }

            return true;
        }
    }

    return false;
}

bool
Config::parseButton(const std::string &button_string, uint &mod, uint &button)
{
    // used for parsing
    vector<string> tok;
    vector<string>::iterator it;

    // chop the string up separating mods and the end key/button
    if (Util::splitString(button_string, tok, " \t")) {
        // if the last token isn't an key/button, the action isn't valid
        button = getMouseButton(tok[tok.size() - 1]);
        if (button != BUTTON_NO) {
            tok.pop_back(); // remove the key/button

            // add the modifier
            mod = 0;
            uint tmp_mod;

            for (it = tok.begin(); it != tok.end(); ++it) {
                tmp_mod = getMod(*it);
                if (tmp_mod == MOD_ANY) {
                    mod = MOD_ANY;
                    break;
                } else {
                    mod |= tmp_mod;
                }
            }

            return true;
        }
    }

    return false;
}

//! @brief Parse a single action and fills action.
//! @param action_string String representation of action.
//! @param action Action structure to fill in.
//! @param mask Mask action is valid for.
//! @return true on success, else false
bool
Config::parseAction(const std::string &action_string, Action &action, uint mask)
{
    vector<string> tok;

    // chop the string up separating the action and parameters
    if (Util::splitString(action_string, tok, " \t", 2)) {
        action.setAction(getAction(tok[0], mask));
        if (action.getAction() != ACTION_NO) {
            if (tok.size() == 2) { // we got enough tok for a parameter
                switch (action.getAction()) {
                case ACTION_EXEC:
                case ACTION_RESTART_OTHER:
                case ACTION_FIND_CLIENT:
                case ACTION_SHOW_CMD_DIALOG:
                case ACTION_SHOW_SEARCH_DIALOG:
                case ACTION_SEND_KEY:
                case ACTION_MENU_DYN:
                case ACTION_DEBUG:
                    action.setParamS(tok[1]);
                    break;
                case ACTION_SET_GEOMETRY:
                    parseActionSetGeometry(action, tok[1]);
                    break;
                case ACTION_ACTIVATE_CLIENT_REL:
                case ACTION_MOVE_CLIENT_REL:
                case ACTION_GOTO_CLIENT_ID:
                case ACTION_MOVE_TO_HEAD:
                    action.setParamI(0, strtol(tok[1].c_str(), 0, 10));
                    break;
                case ACTION_SET:
                case ACTION_UNSET:
                case ACTION_TOGGLE:
                    parseActionState(action, tok[1]);
                    break;
                case ACTION_MAXFILL:
                    if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
                        action.setParamI(0, Util::isTrue(tok[tok.size() - 2]));
                        action.setParamI(1, Util::isTrue(tok[tok.size() - 1]));
                    } else {
                        cerr << "*** WARNING: Missing argument to MaxFill." << endl;
                    }
                    break;
                case ACTION_GROW_DIRECTION:
                    action.setParamI(0, ParseUtil::getValue<DirectionType>(tok[1], _direction_map));
                    break;
                case ACTION_ACTIVATE_CLIENT_NUM:
                    action.setParamI(0, strtol(tok[1].c_str(), 0, 10) - 1);
                    if (action.getParamI(0) < 0) {
                        cerr << "*** WARNING: Negative number to ActivateClientNum." << endl;
                        action.setParamI(0, 0);
                    }
                    break;
                case ACTION_WARP_TO_WORKSPACE:
                case ACTION_SEND_TO_WORKSPACE:
                case ACTION_GOTO_WORKSPACE:
                    action.setParamI(0, parseWorkspaceNumber(tok[1]));
                    break;
                case ACTION_GROUPING_DRAG:
                    action.setParamI(0, Util::isTrue(tok[1]));
                    break;
                case ACTION_MOVE_TO_EDGE:
                    action.setParamI(0, ParseUtil::getValue<OrientationType>(tok[1], _edge_map));
                    break;
                case ACTION_NEXT_FRAME:
                case ACTION_NEXT_FRAME_MRU:
                case ACTION_PREV_FRAME:
                case ACTION_PREV_FRAME_MRU:
                    if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
                        action.setParamI(0, ParseUtil::getValue<Raise>(tok[tok.size() - 2],
                                         _raise_map));
                        action.setParamI(1, Util::isTrue(tok[tok.size() - 1]));
                    } else {
                        action.setParamI(0, ParseUtil::getValue<Raise>(tok[1],
                                         _raise_map));
                        action.setParamI(1, false);
                    }
                    break;
                case ACTION_FOCUS_DIRECTIONAL:
                    if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
                        action.setParamI(0, ParseUtil::getValue<DirectionType>(tok[tok.size() - 2], _direction_map));
                        action.setParamI(1, Util::isTrue(tok[tok.size() - 1])); // raise?
                    } else {
                        action.setParamI(0, ParseUtil::getValue<DirectionType>(tok[1], _direction_map));
                        action.setParamI(1, true); // default to raise
                    }
                    break;
                case ACTION_RESIZE:
                    action.setParamI(0, 1 + ParseUtil::getValue<BorderPosition>(tok[1], _borderpos_map));
                    break;
                case ACTION_RAISE:
                case ACTION_LOWER:
                    if ((Util::splitString(tok[1], tok, " \t", 1)) == 1) {
                        action.setParamI(0, Util::isTrue(tok[tok.size() - 1]));
                    } else {
                        action.setParamI(0, false);
                    }
                    break;
                case ACTION_SHOW_MENU:
                    if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
                        Util::to_upper(tok[tok.size() - 2]);
                        action.setParamS(tok[tok.size() - 2]);
                        action.setParamI(0, Util::isTrue(tok[tok.size() - 1]));
                    } else {
                        Util::to_upper(tok[1]);
                        action.setParamS(tok[1]);
                        action.setParamI(0, false); // Default to non-sticky
                    }
                    break;
                case ACTION_SET_OPACITY:
                    if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
                        action.setParamI(0, std::atoi(tok[tok.size() - 2].c_str()));
                        action.setParamI(1, std::atoi(tok[tok.size() - 1].c_str()));
                    } else {
                        action.setParamI(0, std::atoi(tok[1].c_str()));
                        action.setParamI(1, std::atoi(tok[1].c_str()));
                    }
                    break;
                default:
                    // do nothing
                    break;
                }
            } else {
                switch (action.getAction()) {
                case ACTION_MAXFILL:
                    action.setParamI(0, 1);
                    action.setParamI(1, 1);
                    break;
                default:
                    // do nothing
                    break;
                }
            }

            return true;
        }
    }

    return false;
}

bool
Config::parseActionAccessMask(const std::string &action_mask, uint &mask)
{
    mask = ACTION_ACCESS_NO;

    vector<string> tok;
    if (Util::splitString(action_mask, tok, " \t")) {
        vector<string>::iterator it(tok.begin());
        for (; it != tok.end(); ++it) {
            mask |= getActionAccessMask(*it);
        }
    }

    return true;
}

bool
Config::parseActionState(Action &action, const std::string &as_action)
{
    vector<string> tok;

    // chop the string up separating the action and parameters
    if (Util::splitString(as_action, tok, " \t", 2)) {
        action.setParamI(0, ParseUtil::getValue<ActionStateType>(tok[0], _action_state_map));
        if (action.getParamI(0) != ACTION_STATE_NO) {
            if (tok.size() == 2) { // we got enough tok for a parameter
                string directions;

                switch (action.getParamI(0)) {
                case ACTION_STATE_MAXIMIZED:
                    // Using copy of token here to silence valgrind checks.
                    directions = tok[1];

                    Util::splitString(directions, tok, " \t", 2);
                    if (tok.size() == 4) {
                        action.setParamI(1, Util::isTrue(tok[2]));
                        action.setParamI(2, Util::isTrue(tok[3]));
                    } else {
                        cerr << "*** WARNING: Missing argument to Maximized." << endl;
                    }
                    break;
                case ACTION_STATE_TAGGED:
                    action.setParamI(1, Util::isTrue(tok[1]));
                    break;
                case ACTION_STATE_SKIP:
                    action.setParamI(1, getSkip(tok[1]));
                    break;
                case ACTION_STATE_CFG_DENY:
                    action.setParamI(1, getCfgDeny(tok[1]));
                    break;
                case ACTION_STATE_DECOR:
                case ACTION_STATE_TITLE:
                    action.setParamS(tok[1]);
                    break;
                };
            } else {
                switch (action.getParamI(0)) {
                case ACTION_STATE_MAXIMIZED:
                    action.setParamI(1, 1);
                    action.setParamI(2, 1);
                    break;
                default:
                    // do nothing
                    break;
                }
            }

            return true;
        }
    }

    return false;
}

bool
Config::parseActions(const std::string &action_string, ActionEvent &ae, uint mask)
{
    vector<string> tok;
    vector<string>::iterator it;
    Action action;

    // reset the action event
    ae.action_list.clear();

    // chop the string up separating the actions
    if (Util::splitString(action_string, tok, ";", 0, false, '\\')) {
        for (it = tok.begin(); it != tok.end(); ++it) {
            if (parseAction(*it, action, mask)) {
                ae.action_list.push_back(action);
                action.clear();
            }
        }

        return true;
    }

    return false;
}

bool
Config::parseActionEvent(CfgParser::Entry *section, ActionEvent &ae, uint mask, bool button)
{
    CfgParser::Entry *value = section->findEntry("ACTIONS");
    if (! value && section->getSection()) {
        value = section->getSection()->findEntry("ACTIONS");
    }

    if (! value) {
        return false;
    }

    string str_button = section->getValue();
    if (! str_button.size()) {
        if ((ae.type == MOUSE_EVENT_ENTER) || (ae.type == MOUSE_EVENT_LEAVE)) {
            str_button = "1";
        } else {
            return false;
        }
    }

    bool ok;
    if (button) {
        ok = parseButton(str_button, ae.mod, ae.sym);
    } else {
        ok = parseKey(str_button, ae.mod, ae.sym);
    }
    
    if (ok) {
        return parseActions(value->getValue(), ae, mask);
    }
    return false;
}

bool
Config::parseMoveResizeAction(const std::string &action_string, Action &action)
{
    vector<string> tok;

    // Chop the string up separating the actions.
    if (Util::splitString(action_string, tok, " \t", 2)) {
        action.setAction(ParseUtil::getValue<MoveResizeActionType>(tok[0],
                         _moveresize_map));
        if (action.getAction() != NO_MOVERESIZE_ACTION) {
            if (tok.size() == 2) { // we got enough tok for a paremeter
                switch (action.getAction()) {
                case MOVE_HORIZONTAL:
                case MOVE_VERTICAL:
                case RESIZE_HORIZONTAL:
                case RESIZE_VERTICAL:
                case MOVE_SNAP:
                    action.setParamI(0, strtol(tok[1].c_str(), 0, 10));
                    break;
                default:
                    // Do nothing.
                    break;
                }
            }

            return true;
        }
    }

    return false;
}

bool
Config::parseMoveResizeActions(const std::string &action_string, ActionEvent& ae)
{
    vector<string> tok;
    vector<string>::iterator it;
    Action action;

    // reset the action event
    ae.action_list.clear();

    // chop the string up separating the actions
    if (Util::splitString(action_string, tok, ";")) {
        for (it = tok.begin(); it != tok.end(); ++it) {
            if (parseMoveResizeAction(*it, action)) {
                ae.action_list.push_back(action);
                action.clear();
            }
        }

        return true;
    }

    return false;
}

//! @brief Parses MoveResize Event.
bool
Config::parseMoveResizeEvent(CfgParser::Entry *section, ActionEvent& ae)
{
    CfgParser::Entry *value;

    if (! section->getValue().size ()) {
        return false;
    }
    
    if (parseKey(section->getValue(), ae.mod, ae.sym)) {
        value = section->getSection()->findEntry("ACTIONS");
        if (value) {
            return parseMoveResizeActions(value->getValue(), ae);
        }
    }

    return false;
}

bool
Config::parseInputDialogAction(const std::string &val, Action &action)
{
    action.setAction(ParseUtil::getValue<InputDialogAction>(val, _inputdialog_map));
    return (action.getAction() != INPUT_NO_ACTION);
}

bool
Config::parseInputDialogActions(const std::string &actions, ActionEvent &ae)
{
    vector<string> tok;
    vector<string>::iterator it;
    Action action;
    string::size_type first, last;

    // reset the action event
    ae.action_list.clear();

    // chop the string up separating the actions
    if (Util::splitString(actions, tok, ";")) {
        for (it = tok.begin(); it != tok.end(); ++it) {
            first = (*it).find_first_not_of(" \t\n");
            if (first == string::npos)
                continue;
            last = (*it).find_last_not_of(" \t\n");
            (*it) = (*it).substr(first, last-first+1);
            if (parseInputDialogAction(*it, action)) {
                ae.action_list.push_back(action);
                action.clear();
            }
        }

        return true;
    }

    return false;

}

//! @brief Parses InputDialog Event.
bool
Config::parseInputDialogEvent(CfgParser::Entry *section, ActionEvent &ae)
{
    CfgParser::Entry *value;

    if (! section->getValue().size()) {
        return false;
    }
    
    if (parseKey(section->getValue(), ae.mod, ae.sym)) {
        value = section->getSection()->findEntry("ACTIONS");
        if (value) {
            return parseInputDialogActions(value->getValue(), ae);
        }
    }

    return false;
}

/**
 * Get mask for handling menu events.
 */
uint
Config::getMenuMask(const std::string &mask)
{
    uint mask_return = 0, val;

    vector<string> tok;
    Util::splitString(mask, tok, " \t");

    vector<string>::iterator it(tok.begin());
    for (; it != tok.end(); ++it) {
        val = ParseUtil::getValue<MouseEventType>(*it, _mouse_event_map);
        if (val != MOUSE_EVENT_NO) {
            mask_return |= val;
        }
    }
    return mask_return;
}

bool
Config::parseMenuAction(const std::string &action_string, Action &action)
{
    vector<string> tok;

    // chop the string up separating the actions
    if (Util::splitString(action_string, tok, " \t", 2)) {
        action.setAction(ParseUtil::getValue<ActionType>(tok[0], _menu_action_map));
        if (action.getAction() != ACTION_NO) {
            return true;
        }
    }

    return false;
}

bool
Config::parseMenuActions(const std::string &actions, ActionEvent &ae)
{
    vector<string> tok;
    vector<string>::iterator it;
    Action action;

    // reset the action event
    ae.action_list.clear();

    // chop the string up separating the actions
    if (Util::splitString(actions, tok, ";", 0, false, '\\')) {
        for (it = tok.begin(); it != tok.end(); ++it) {
            if (parseMenuAction(*it, action)) {
                ae.action_list.push_back(action);
                action.clear();
            }
        }

        return true;
    }

    return false;
}

//! @brief Parses MenuEvent.
bool
Config::parseMenuEvent(CfgParser::Entry *section, ActionEvent& ae)
{
    CfgParser::Entry *value;

    if (! section->getValue().size()) {
        return false;
    }

    if (parseKey(section->getValue(), ae.mod, ae.sym)) {
        value = section->getSection()->findEntry("ACTIONS");
        if (value) {
            return parseMenuActions(value->getValue(), ae);
        }
    }

    return false;
}

uint
Config::getMouseButton(const std::string &button)
{
    uint btn;

    if (button.size() == 1 || button.size() == 2) { // it's a button
        btn = unsigned(strtol(button.c_str(), 0, 10));
    } else if (strcasecmp(button.c_str(), "ANY") == 0) { // any button
        btn = BUTTON_ANY;
    } else {
        btn = BUTTON_NO;
    }

    if (btn > BUTTON_NO) {
        btn = BUTTON_NO;
    }

    return btn;
}

/**
 * Load main configuration file, priority as follows:
 *
 *   1. Load command line specified file.
 *   2. Load ~/.pekwm/config
 *   3. Copy configuration and load ~/.pekwm/config
 *   4. Load system configuration
 */
bool
Config::tryHardLoadConfig(CfgParser &cfg, std::string &file)
{
    bool success = false;

    // Try loading command line specified file.
    if (file.size()) {
        success = cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
    }

    // Try loading ~/.pekwm/config
    if (! success) {
        file = string(getenv("HOME")) + string("/.pekwm/config");
        success = cfg.parse(file, CfgParserSource::SOURCE_FILE, true);

        // Copy cfg files to ~/.pekwm and try loading ~/.pekwm/config again.
        if (! success) {
            copyConfigFiles();
            success = cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
        }
    }

    // Try loading system configuration files.
    if (! success) {
        file = string(SYSCONFDIR "/config");
        success = cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
    }

    return success;
}

//! @brief Populates the ~/.pekwm/ dir with config files
void
Config::copyConfigFiles(void)
{
    string cfg_dir = getenv("HOME") + string("/.pekwm");

    string cfg_file = cfg_dir + string("/config");
    string keys_file = cfg_dir + string("/keys");
    string mouse_file = cfg_dir + string("/mouse");
    string menu_file = cfg_dir + string("/menu");
    string autoprops_file = cfg_dir + string("/autoproperties");
    string start_file = cfg_dir + string("/start");
    string vars_file = cfg_dir + string("/vars");
    string themes_dir = cfg_dir + string("/themes");

    bool cp_config, cp_keys, cp_mouse, cp_menu;
    bool cp_autoprops, cp_start, cp_vars;
    bool make_themes = false;
    cp_config = cp_keys = cp_mouse = cp_menu = false;
    cp_autoprops = cp_start = cp_vars = false;

    struct stat stat_buf;
    // check and see if we already have a ~/.pekwm/ directory
    if (stat(cfg_dir.c_str(), &stat_buf) == 0) {
        // is it a dir or file?
        if (! S_ISDIR(stat_buf.st_mode)) {
            cerr << cfg_dir << " already exists and isn't a directory" << endl
                 << "Can't copy config files !" << endl;
            return;
        }

        // we already have a directory, see if it's writeable and executable
        bool cfg_dir_ok = false;

        if (getuid() == stat_buf.st_uid) {
            if ((stat_buf.st_mode&S_IWUSR) && (stat_buf.st_mode&(S_IXUSR))) {
                cfg_dir_ok = true;
            }
        }
        if (! cfg_dir_ok) {
            if (getgid() == stat_buf.st_gid) {
                if ((stat_buf.st_mode&S_IWGRP) && (stat_buf.st_mode&(S_IXGRP))) {
                    cfg_dir_ok = true;
                }
            }
        }

        if (! cfg_dir_ok) {
            if (! (stat_buf.st_mode&S_IWOTH) || ! (stat_buf.st_mode&(S_IXOTH))) {
                cerr << "You don't have the rights to add files to the: " << cfg_dir
                << " directory! Therefore I can't copy the config files!" << endl;
                return;
            }
        }

        // we apparently could write and exec that dir, now see if we have any
        // files in it
        if (stat(cfg_file.c_str(), &stat_buf))
            cp_config = true;
        if (stat(keys_file.c_str(), &stat_buf))
            cp_keys = true;
        if (stat(mouse_file.c_str(), &stat_buf))
            cp_mouse = true;
        if (stat(menu_file.c_str(), &stat_buf))
            cp_menu = true;
        if (stat(autoprops_file.c_str(), &stat_buf))
            cp_autoprops = true;
        if (stat(start_file.c_str(), &stat_buf))
            cp_start = true;
        if (stat(vars_file.c_str(), &stat_buf))
            cp_vars = true;
        if (stat(themes_dir.c_str(), &stat_buf)) {
            make_themes = true;
        }
    } else { // we didn't have a ~/.pekwm directory already, lets create one
        if (mkdir(cfg_dir.c_str(), 0700)) {
            cerr << "Can't create " << cfg_dir << " directory!" << endl;
            cerr << "Can't copy config files !" << endl;
            return;
        }

        cp_config = cp_keys = cp_mouse = cp_menu = true;
        cp_autoprops = cp_start = cp_vars = true;
        make_themes = true;
    }

    if (cp_config) {
        Util::copyTextFile(SYSCONFDIR "/config", cfg_file);
    }
    if (cp_keys) {
        Util::copyTextFile(SYSCONFDIR "/keys", keys_file);
    }
    if (cp_mouse) {
        Util::copyTextFile(SYSCONFDIR "/mouse", mouse_file);
    }
    if (cp_menu) {
        Util::copyTextFile(SYSCONFDIR "/menu", menu_file);
    }
    if (cp_autoprops) {
        Util::copyTextFile(SYSCONFDIR "/autoproperties", autoprops_file);
    }
    if (cp_start) {
        Util::copyTextFile(SYSCONFDIR "/start", start_file);
    }
    if (cp_vars) {
        Util::copyTextFile(SYSCONFDIR "/vars", vars_file);
    }
    if (make_themes) {
        mkdir(themes_dir.c_str(), 0700);
    }
}

/**
 * Parses mouse configuration file.
 */
bool
Config::loadMouseConfig(const std::string &mouse_file)
{
    if (! _cfg_files_mouse.requireReload(mouse_file)) {
        return false;
    }
    
    CfgParser mouse_cfg;
    if (! mouse_cfg.parse(mouse_file, CfgParserSource::SOURCE_FILE, true)
        && ! mouse_cfg.parse(SYSCONFDIR "/mouse", CfgParserSource::SOURCE_FILE, true)) {
        _cfg_files_mouse.clear();
        return false;
    }
    
    if (mouse_cfg.isDynamicContent()) {
        _cfg_files_mouse.clear();
    } else {
        _cfg_files_mouse = mouse_cfg.getCfgFiles();
    }

    // Make sure old actions get unloaded.
    map<MouseActionListName, vector<ActionEvent>* >::iterator it;
    for (it = _mouse_action_map.begin(); it != _mouse_action_map.end(); ++it) {
        it->second->clear();
    }

    CfgParser::Entry *section;

    section = mouse_cfg.getEntryRoot()->findSection("FRAMETITLE");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_TITLE_FRAME], FRAME_OK);
    }

    section = mouse_cfg.getEntryRoot()->findSection("OTHERTITLE");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_TITLE_OTHER], FRAME_OK);
    }
    
    section = mouse_cfg.getEntryRoot()->findSection("CLIENT");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_CHILD_FRAME], CLIENT_OK);
    }
    
    section = mouse_cfg.getEntryRoot()->findSection("ROOT");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_ROOT], ROOTCLICK_OK);
    }
    
    section = mouse_cfg.getEntryRoot()->findSection("MENU");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_MENU], FRAME_OK);
    }
    
    section = mouse_cfg.getEntryRoot()->findSection("OTHER");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_OTHER], FRAME_OK);
    }
    
    section = mouse_cfg.getEntryRoot()->findSection("SCREENEDGE");
    if (section) {
        CfgParser::iterator edge_it(section->begin());
        for (; edge_it != section->end(); ++edge_it)  {
            uint pos = ParseUtil::getValue<DirectionType>((*edge_it)->getName(), _direction_map);

            if (pos != SCREEN_EDGE_NO) {
                parseButtons((*edge_it)->getSection(), getEdgeListFromPosition(pos), SCREEN_EDGE_OK);
            }
        }
    }
    
    section = mouse_cfg.getEntryRoot()->findSection("BORDER");
    if (section) {
        CfgParser::iterator border_it(section->begin());
        for (; border_it != section->end(); ++border_it) {
            uint pos = ParseUtil::getValue<BorderPosition>((*border_it)->getName(), _borderpos_map);
            if (pos != BORDER_NO_POS) {
                parseButtons((*border_it)->getSection(), getBorderListFromPosition(pos), FRAME_BORDER_OK);
            }
        }
    }

    return true;
}

//! @brief Parses mouse config section, like FRAME
void
Config::parseButtons(CfgParser::Entry *section, vector<ActionEvent>* mouse_list, ActionOk action_ok)
{
    if (! section || ! mouse_list) {
        return;
    }

    ActionEvent ae;
    CfgParser::Entry *value;

    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if (! (*it)->getSection()) {
            continue;
        }
        
        ae.type = ParseUtil::getValue<MouseEventType>((*it)->getName(), _mouse_event_map);

        if (ae.type == MOUSE_EVENT_NO) {
            continue;
        }

        if (ae.type == MOUSE_EVENT_MOTION) {
            value = (*it)->getSection()->findEntry("THRESHOLD");
            if (value) {
                ae.threshold = strtol(value->getValue().c_str(), 0, 10);
            } else {
                ae.threshold = 0;
            }
        }

        if (parseActionEvent((*it), ae, action_ok, true)) {
            mouse_list->push_back(ae);
        }
    }
}

// frame border configuration

vector<ActionEvent>*
Config::getBorderListFromPosition(uint pos)
{
    vector<ActionEvent> *ret = 0;

    switch (pos) {
    case BORDER_TOP_LEFT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_TL];
        break;
    case BORDER_TOP:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_T];
        break;
    case BORDER_TOP_RIGHT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_TR];
        break;
    case BORDER_LEFT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_L];
        break;
    case BORDER_RIGHT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_R];
        break;
    case BORDER_BOTTOM_LEFT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_BL];
        break;
    case BORDER_BOTTOM:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_B];
        break;
    case BORDER_BOTTOM_RIGHT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_BORDER_BR];
        break;
    }

    return ret;
}

vector<ActionEvent>*
Config::getEdgeListFromPosition(uint pos)
{
    vector<ActionEvent> *ret = 0;

    switch (pos) {
    case SCREEN_EDGE_TOP:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_EDGE_T];
        break;
    case SCREEN_EDGE_BOTTOM:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_EDGE_B];
        break;
    case SCREEN_EDGE_LEFT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_EDGE_L];
        break;
    case SCREEN_EDGE_RIGHT:
        ret = _mouse_action_map[MOUSE_ACTION_LIST_EDGE_R];
        break;
    };

    return ret;
}


//! @brief Parses workspace number
int
Config::parseWorkspaceNumber(const std::string &workspace)
{
    // Get workspace looking for relative numbers
    uint num = ParseUtil::getValue<WorkspaceChangeType>(workspace, _workspace_change_map);

    if (num == WORKSPACE_NO) {
        // Workspace isn't relative, check for 2x2 and ordinary specification
        vector<string> tok;
        if (Util::splitString(workspace, tok, "x", 2, true) == 2) {
            uint row = strtol(tok[0].c_str(), 0, 10) - 1;
            uint col = strtol(tok[1].c_str(), 0, 10) - 1;

            num = _screen_workspaces_per_row * row + col;

        } else {
            num = strtol(workspace.c_str(), 0, 10) - 1;
        }
    }

    return num;
}

//! @brief Parses a string which contains two opacity values
bool
Config::parseOpacity(const std::string value, uint &focused, uint &unfocused)
{
    std::vector<string> tokens;
    switch ((Util::splitString(value, tokens, " ,", 2))) {
    case 2:
        focused = std::atoi(tokens.at(0).c_str());
        unfocused = std::atoi(tokens.at(1).c_str());
        break;
    case 1:
        focused = unfocused = std::atoi(tokens.at(0).c_str());
        break;
    default:
        return false;
    }
    CONV_OPACITY(focused);
    CONV_OPACITY(unfocused);
    return true;
}

/**
 * Parse SetGeometry action parameters.
 *
 * SetGeometry 1x+0+0 [(screen|current|0-9) [HonourStrut]]
 */
void
Config::parseActionSetGeometry(Action& action, const std::string &str)
{
    std::vector<std::string> tok;

    if (! Util::splitString(str, tok, " \t", 3)) {
        return;
    }

    // geometry
    action.setParamS(tok[0]);

    // screen, current head or head number
    if (tok.size() > 1) {
        if (strcasecmp(tok[1].c_str(), "SCREEN") == 0) {
            action.setParamI(0, -1);
        } else if (strcasecmp(tok[1].c_str(), "CURRENT") == 0) {
            action.setParamI(0, -2);
        } else {
            action.setParamI(0, strtol(tok[1].c_str(), 0, 10));
        }
    } else {
        action.setParamI(0, -1);
    }

    // honour strut option
    if (tok.size() > 2) {
        action.setParamI(1, strcasecmp(tok[2].c_str(), "HONOURSTRUT") ? 0 : 1);
    } else {
        action.setParamI(1, 0);
    }
}
