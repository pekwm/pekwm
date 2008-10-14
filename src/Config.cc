//
// Config.cc for pekwm
// Copyright © 2002-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Config.hh"

#include "Util.hh"
#include "PScreen.hh" // for DPY in keyconfig code

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
using std::list;
using std::map;
using std::vector;
using std::pair;
using std::ifstream;
using std::ofstream;

Config* Config::_instance = NULL;

const int FRAME_MASK =
    FRAME_OK|FRAME_BORDER_OK|CLIENT_OK|WINDOWMENU_OK|
    KEYGRABBER_OK|BUTTONCLICK_OK;
const int ANY_MASK =
    KEYGRABBER_OK|FRAME_OK|FRAME_BORDER_OK|CLIENT_OK|ROOTCLICK_OK|
    BUTTONCLICK_OK|WINDOWMENU_OK|ROOTMENU_OK|SCREEN_EDGE_OK;

//! @brief Constructor for Config class
Config::Config(void) :
        _moveresize_edgeattract(0), _moveresize_edgeresist(0),
        _moveresize_woattract(0), _moveresize_woresist(0),
        _moveresize_opaquemove(0), _moveresize_opaqueresize(0),
        _screen_workspaces(4), _screen_pixmap_cache_size(20),
        _screen_workspaces_per_row(0), _screen_workspace_name_default(L"Workspace"),
        _screen_edge_indent(false),
        _screen_doubleclicktime(250), _screen_fullscreen_above(true),
        _screen_fullscreen_detect(true),
        _screen_showframelist(true),
        _screen_show_status_window(true), _screen_show_client_id(false),
        _screen_show_workspace_indicator(500), _screen_workspace_indicator_scale(16),
        _screen_place_new(true), _screen_focus_new(false),
        _screen_focus_new_child(true), _screen_honour_randr(true),
        _screen_placement_row(false),
        _screen_placement_ltr(true), _screen_placement_ttb(true),
        _screen_placement_offset_x(0), _screen_placement_offset_y(0),
        _screen_client_unique_name(true),
        _screen_client_unique_name_pre(" #"), _screen_client_unique_name_post(""),
        _menu_select_mask(0), _menu_enter_mask(0), _menu_exec_mask(0),
        _menu_icon_width(16), _menu_icon_height(16),
        _cmd_dialog_history_unique(true), _cmd_dialog_history_size(1024),
        _cmd_dialog_history_file("~/.pekwm/history"), _cmd_dialog_history_save_interval(16)
#ifdef HARBOUR
       ,_harbour_da_min_s(0), _harbour_da_max_s(0),
        _harbour_head_nr(0)
#endif // HARBOUR
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
    _action_map["FOCUS"] = pair<ActionType, uint>(ACTION_FOCUS, ANY_MASK);
    _action_map["UNFOCUS"] = pair<ActionType, uint>(ACTION_UNFOCUS, ANY_MASK);

    _action_map["SET"] = pair<ActionType, uint>(ACTION_SET, ANY_MASK);
    _action_map["UNSET"] = pair<ActionType, uint>(ACTION_UNSET, ANY_MASK);
    _action_map["TOGGLE"] = pair<ActionType, uint>(ACTION_TOGGLE, ANY_MASK);

    _action_map["MAXFILL"] = pair<ActionType, uint>(ACTION_MAXFILL, FRAME_MASK);
    _action_map["GROWDIRECTION"] = pair<ActionType, uint>(ACTION_GROW_DIRECTION, FRAME_MASK);
    _action_map["CLOSE"] = pair<ActionType, uint>(ACTION_CLOSE, FRAME_MASK);
    _action_map["CLOSEFRAME"] = pair<ActionType, uint>(ACTION_CLOSE_FRAME, FRAME_MASK);
    _action_map["KILL"] = pair<ActionType, uint>(ACTION_KILL, FRAME_MASK);
    _action_map["RAISE"] = pair<ActionType, uint>(ACTION_RAISE, FRAME_MASK);
    _action_map["LOWER"] = pair<ActionType, uint>(ACTION_LOWER, FRAME_MASK);
    _action_map["ACTIVATEORRAISE"] = pair<ActionType, uint>(ACTION_ACTIVATE_OR_RAISE, FRAME_MASK);
    _action_map["ACTIVATECLIENTREL"] = pair<ActionType, uint>(ACTION_ACTIVATE_CLIENT_REL, FRAME_MASK);
    _action_map["MOVECLIENTREL"] = pair<ActionType, uint>(ACTION_MOVE_CLIENT_REL, FRAME_MASK);
    _action_map["ACTIVATECLIENT"] = pair<ActionType, uint>(ACTION_ACTIVATE_CLIENT, FRAME_MASK);
    _action_map["ACTIVATECLIENTNUM"] = pair<ActionType, uint>(ACTION_ACTIVATE_CLIENT_NUM, KEYGRABBER_OK);
    _action_map["RESIZE"] = pair<ActionType, uint>(ACTION_RESIZE, BUTTONCLICK_OK|CLIENT_OK|FRAME_OK|FRAME_BORDER_OK);
    _action_map["MOVE"] = pair<ActionType, uint>(ACTION_MOVE, FRAME_OK|FRAME_BORDER_OK|CLIENT_OK);
    _action_map["MOVERESIZE"] = pair<ActionType, uint>(ACTION_MOVE_RESIZE, KEYGRABBER_OK);
    _action_map["GROUPINGDRAG"] = pair<ActionType, uint>(ACTION_GROUPING_DRAG, FRAME_OK|CLIENT_OK);
    _action_map["WARPTOWORKSPACE"] = pair<ActionType, uint>(ACTION_WARP_TO_WORKSPACE, SCREEN_EDGE_OK);
    _action_map["MOVETOEDGE"] = pair<ActionType, uint>(ACTION_MOVE_TO_EDGE, KEYGRABBER_OK);
    _action_map["NEXTFRAME"] = pair<ActionType, uint>(ACTION_NEXT_FRAME, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["PREVFRAME"] = pair<ActionType, uint>(ACTION_PREV_FRAME, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["NEXTFRAMEMRU"] = pair<ActionType, uint>(ACTION_NEXT_FRAME_MRU, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["PREVFRAMEMRU"] = pair<ActionType, uint>(ACTION_PREV_FRAME_MRU, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["FOCUSDIRECTIONAL"] = pair<ActionType, uint>(ACTION_FOCUS_DIRECTIONAL, FRAME_MASK);
    _action_map["ATTACHMARKED"] = pair<ActionType, uint>(ACTION_ATTACH_MARKED, FRAME_MASK);
    _action_map["ATTACHCLIENTINNEXTFRAME"] = pair<ActionType, uint>(ACTION_ATTACH_CLIENT_IN_NEXT_FRAME, FRAME_MASK);
    _action_map["ATTACHCLIENTINPREVFRAME"] = pair<ActionType, uint>(ACTION_ATTACH_CLIENT_IN_PREV_FRAME, FRAME_MASK);
    _action_map["FINDCLIENT"] = pair<ActionType, uint>(ACTION_FIND_CLIENT, ANY_MASK);
    _action_map["GOTOCLIENTID"] = pair<ActionType, uint>(ACTION_GOTO_CLIENT_ID, ANY_MASK);
    _action_map["DETACH"] = pair<ActionType, uint>(ACTION_DETACH, FRAME_MASK);
    _action_map["SENDTOWORKSPACE"] = pair<ActionType, uint>(ACTION_SEND_TO_WORKSPACE, ANY_MASK);
    _action_map["GOTOWORKSPACE"] = pair<ActionType, uint>(ACTION_GOTO_WORKSPACE, ANY_MASK );
    _action_map["EXEC"] = pair<ActionType, uint>(ACTION_EXEC, FRAME_MASK|ROOTMENU_OK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["RELOAD"] = pair<ActionType, uint>(ACTION_RELOAD, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["RESTART"] = pair<ActionType, uint>(ACTION_RESTART, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["RESTARTOTHER"] = pair<ActionType, uint>(ACTION_RESTART_OTHER, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["EXIT"] = pair<ActionType, uint>(ACTION_EXIT, KEYGRABBER_OK|ROOTMENU_OK);
    _action_map["SHOWCMDDIALOG"] = pair<ActionType, uint>(ACTION_SHOW_CMD_DIALOG, KEYGRABBER_OK|ROOTCLICK_OK|SCREEN_EDGE_OK|ROOTMENU_OK|WINDOWMENU_OK);
#ifdef MENUS
    _action_map["SHOWMENU"] = pair<ActionType, uint>(ACTION_SHOW_MENU, FRAME_MASK|ROOTCLICK_OK|SCREEN_EDGE_OK|ROOTMENU_OK|WINDOWMENU_OK);
    _action_map["HIDEALLMENUS"] = pair<ActionType, uint>(ACTION_HIDE_ALL_MENUS, FRAME_MASK|ROOTCLICK_OK|SCREEN_EDGE_OK);
    _action_map["SUBMENU"] = pair<ActionType, uint>(ACTION_MENU_SUB, ROOTMENU_OK|WINDOWMENU_OK);
    _action_map["DYNAMIC"] = pair<ActionType, uint>(ACTION_MENU_DYN, ROOTMENU_OK|WINDOWMENU_OK);
#endif // MENUS
    _action_map["SENDKEY"] = pair<ActionType, uint>(ACTION_SEND_KEY, ANY_MASK);

    _placement_map[""] = PLACE_NO;
    _placement_map["SMART"] = PLACE_SMART;
    _placement_map["MOUSENOTUNDER"] = PLACE_MOUSE_NOT_UNDER;
    _placement_map["MOUSECENTERED"] = PLACE_MOUSE_CENTERED;
    _placement_map["MOUSETOPLEFT"] = PLACE_MOUSE_TOP_LEFT;
    _placement_map["CENTEREDONPARENT"] = PLACE_CENTERED_ON_PARENT;

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
    _workspace_change_map["PREV"] = WORKSPACE_PREV;
    _workspace_change_map["RIGHT"] = WORKSPACE_RIGHT;
    _workspace_change_map["NEXT"] = WORKSPACE_NEXT;
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
    _action_state_map["MAXIMIZED"] = ACTION_STATE_MAXIMIZED;
    _action_state_map["FULLSCREEN"] = ACTION_STATE_FULLSCREEN;
    _action_state_map["SHADED"] = ACTION_STATE_SHADED;
    _action_state_map["STICKY"] = ACTION_STATE_STICKY;
    _action_state_map["ALWAYSONTOP"] = ACTION_STATE_ALWAYS_ONTOP;
    _action_state_map["ALWAYSBELOW"] = ACTION_STATE_ALWAYS_BELOW;
    _action_state_map["DECORBORDER"] = ACTION_STATE_DECOR_BORDER;
    _action_state_map["DECORTITLEBAR"] = ACTION_STATE_DECOR_TITLEBAR;
    _action_state_map["ICONIFIED"] = ACTION_STATE_ICONIFIED;
    _action_state_map["TAGGED"] = ACTION_STATE_TAGGED;
    _action_state_map["MARKED"] = ACTION_STATE_MARKED;
    _action_state_map["SKIP"] = ACTION_STATE_SKIP;
    _action_state_map["CFGDENY"] = ACTION_STATE_CFG_DENY;
    _action_state_map["TITLE"] = ACTION_STATE_TITLE;
#ifdef HARBOUR
    _action_state_map["HARBOURHIDDEN"] = ACTION_STATE_HARBOUR_HIDDEN;
#endif // HARBOUR
    _action_state_map["GLOBALGROUPING"] = ACTION_STATE_GLOBAL_GROUPING;


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

#ifdef MENUS
    _menu_action_map[""] = ACTION_MENU_NEXT;
    _menu_action_map["NEXTITEM"] = ACTION_MENU_NEXT;
    _menu_action_map["PREVITEM"] = ACTION_MENU_PREV;
    _menu_action_map["SELECT"] = ACTION_MENU_SELECT;
    _menu_action_map["ENTERSUBMENU"] = ACTION_MENU_ENTER_SUBMENU;
    _menu_action_map["LEAVESUBMENU"] = ACTION_MENU_LEAVE_SUBMENU;
    _menu_action_map["CLOSE"] = ACTION_CLOSE;
#endif // MENUS

#ifdef HARBOUR
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
#endif // HARBOUR

    // fill the mouse action map
    _mouse_action_map[MOUSE_ACTION_LIST_TITLE_FRAME] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_TITLE_OTHER] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_CHILD_FRAME] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_CHILD_OTHER] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_ROOT] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_MENU] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_OTHER] = new list<ActionEvent>;

    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_T] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_B] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_L] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_EDGE_R] = new list<ActionEvent>;

    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_TL] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_T] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_TR] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_L] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_R] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_BL] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_B] = new list<ActionEvent>;
    _mouse_action_map[MOUSE_ACTION_LIST_BORDER_BR] = new list<ActionEvent>;
}

//! @brief Destructor for Config class
Config::~Config(void)
{
    _instance = NULL;

    map<MouseActionListName, list<ActionEvent>* >::iterator it;
    for (it = _mouse_action_map.begin(); it != _mouse_action_map.end(); ++it) {
        delete it->second;
    }
}

//! @brief Tries to load config_file, ~/.pekwm/config, SYSCONFDIR/config
void
Config::load(const std::string &config_file)
{
    CfgParser cfg;
    bool success = false;
    string file_success;

    _config_file = config_file;

    // Try loading command line specified file.
    if (_config_file.size()) {
        success = cfg.parse(config_file, CfgParserSource::SOURCE_FILE, true);
        if (success) {
            file_success = config_file;
        }
    }

    // Try loading ~/.pekwm/config
    if (! success) {
        _config_file = string(getenv("HOME")) + string("/.pekwm/config");
        success = cfg.parse(_config_file, CfgParserSource::SOURCE_FILE, true);
        if (success) {
            file_success = _config_file;
        }
    }

    // Copy cfg files to ~/.pekwm and then try loading global config
    if (! success) {
        copyConfigFiles();

        _config_file = string(SYSCONFDIR "/config");
        success = cfg.parse(_config_file, CfgParserSource::SOURCE_FILE, true);
        if (success) {
            file_success = _config_file;
        }
    }

    if (! success) {
        cerr << " *** WARNING: unable to load configuration files!" << endl;
        return;
    }

    // Update PEKWM_CONFIG_FILE environment if needed ( to reflect active file )
    char *cfg_env = getenv("PEKWM_CONFIG_FILE");
    if (! cfg_env || (strcmp(cfg_env, file_success.c_str()) != 0))
        setenv("PEKWM_CONFIG_FILE", file_success.c_str(), 1);

    string o_file_mouse; // temporary filepath for mouseconfig

    CfgParser::Entry *section;

    // Get other config files dests.
    section = cfg.get_entry_root()->find_section("FILES");
    if (section) {
        loadFiles(section);
    }

    // Parse moving / resizing options.
    section = cfg.get_entry_root()->find_section("MOVERESIZE");
    if (section) {
        loadMoveReszie(section);
    }

    // Screen, important stuff such as number of workspaces
    section = cfg.get_entry_root()->find_section("SCREEN");
    if (section) {
        loadScreen(section);
    }

    section = cfg.get_entry_root()->find_section("MENU");
    if (section) {
        loadMenu(section);
    }

    section = cfg.get_entry_root()->find_section("CMDDIALOG");
    if (section) {
      loadCmdDialog(section);
    }

#ifdef HARBOUR
    section = cfg.get_entry_root()->find_section("HARBOUR");
    if (section) {
        loadHarbour(section);
    }

#endif // HARBOUR
}

//! @brief Loads file section of configuration
//! @param section Pointer to FILES section.
void
Config::loadFiles(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    // Mouse file loading in here as well
    string file_mouse;

    list<CfgParserKey*> key_list;
    key_list.push_back(new CfgParserKeyPath("KEYS", _files_keys, SYSCONFDIR "/keys"));
    key_list.push_back(new CfgParserKeyPath("MOUSE", file_mouse, SYSCONFDIR "/mouse"));
    key_list.push_back(new CfgParserKeyPath("MENU", _files_menu, SYSCONFDIR "/menu"));
    key_list.push_back(new CfgParserKeyPath("START", _files_start, SYSCONFDIR "/start"));
    key_list.push_back(new CfgParserKeyPath("AUTOPROPS", _files_autoprops, SYSCONFDIR "/autoproperties"));
    key_list.push_back(new CfgParserKeyPath("THEME", _files_theme, DATADIR "/pekwm/themes/default/theme"));

    // Parse
    section->parse_key_values(key_list.begin(), key_list.end());

    // Free up resources
    for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());

    // Load the mouse configuration
    loadMouseConfig(file_mouse);
}

//! @brief Loads MOVERESIZE section of main configuration
//! @param section Pointer to MOVERESIZE section.
void
Config::loadMoveReszie(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    list<CfgParserKey*> key_list;
    key_list.push_back(new CfgParserKeyInt("EDGEATTRACT", _moveresize_edgeattract, 0, 0));
    key_list.push_back(new CfgParserKeyInt("EDGERESIST", _moveresize_edgeresist, 0, 0));
    key_list.push_back(new CfgParserKeyInt("WINDOWATTRACT",_moveresize_woattract, 0, 0));
    key_list.push_back(new CfgParserKeyInt("WINDOWRESIST", _moveresize_woresist, 0, 0));
    key_list.push_back(new CfgParserKeyBool("OPAQUEMOVE", _moveresize_opaquemove));
    key_list.push_back(new CfgParserKeyBool("OPAQUERESIZE", _moveresize_opaqueresize));

    // Parse data
    section->parse_key_values(key_list.begin(), key_list.end());

    // Free up resources
    for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
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
    string edge_size, workspace_names;
    CfgParser::Entry *value;

    list<CfgParserKey*> key_list;
    key_list.push_back(new CfgParserKeyInt("WORKSPACES", _screen_workspaces, 4, 1));
    key_list.push_back(new CfgParserKeyInt("PIXMAPCACHESIZE", _screen_pixmap_cache_size));
    key_list.push_back(new CfgParserKeyInt("WORKSPACESPERROW", _screen_workspaces_per_row, 0, 0));
    key_list.push_back(new CfgParserKeyString("WORKSPACENAMES", workspace_names));
    key_list.push_back(new CfgParserKeyString("EDGESIZE", edge_size));
    key_list.push_back(new CfgParserKeyBool("EDGEINDENT", _screen_edge_indent));
    key_list.push_back(new CfgParserKeyInt("DOUBLECLICKTIME", _screen_doubleclicktime, 250, 0));
    key_list.push_back(new CfgParserKeyString("TRIMTITLE",_screen_trim_title));
    key_list.push_back(new CfgParserKeyBool("FULLSCREENABOVE", _screen_fullscreen_above, true));
    key_list.push_back(new CfgParserKeyBool("FULLSCREENDETECT", _screen_fullscreen_detect, true));
    key_list.push_back(new CfgParserKeyBool("SHOWFRAMELIST", _screen_showframelist));
    key_list.push_back(new CfgParserKeyBool("SHOWSTATUSWINDOW", _screen_show_status_window));
    key_list.push_back(new CfgParserKeyBool("SHOWCLIENTID", _screen_show_client_id));
    key_list.push_back(new CfgParserKeyInt("SHOWWORKSPACEINDICATOR",
                                           _screen_show_workspace_indicator, 500, 0));
    key_list.push_back(new CfgParserKeyInt("WORKSPACEINDICATORSCALE",
                                           _screen_workspace_indicator_scale, 16, 2)); 
    key_list.push_back(new CfgParserKeyBool("PLACENEW", _screen_place_new));
    key_list.push_back(new CfgParserKeyBool("FOCUSNEW", _screen_focus_new));
    key_list.push_back(new CfgParserKeyBool("FOCUSNEWCHILD", _screen_focus_new_child, true));
    key_list.push_back(new CfgParserKeyBool("HONOURRANDR", _screen_honour_randr, true));

    // Parse data
    section->parse_key_values(key_list.begin(), key_list.end());

    // Free up resources
    for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
    key_list.clear();

    // Convert input data
    int edge_size_all = 0;
    _screen_edge_sizes.clear();
    if (edge_size.size()) {
        vector<string> sizes;
        if (Util::splitString(edge_size, sizes, " \t", 4) == 4) {
            for (vector<string>::iterator it(sizes.begin()); it != sizes.end(); ++it) {
                _screen_edge_sizes.push_back(strtol(it->c_str(), NULL, 10));
            }
        } else {
            edge_size_all = strtol(edge_size.c_str(), NULL, 10);
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
    if (Util::splitString(workspace_names, vs, ";")) {
        vector<string>::iterator vs_it(vs.begin());
        for (; vs_it != vs.end(); ++vs_it) {
            _screen_workspace_names.push_back(Util::to_wide_str(*vs_it));
        }
    }

    CfgParser::Entry *sub = section->find_section("PLACEMENT");
    if (sub) {
        value = sub->find_entry("MODEL");
        if (value) {
            _screen_placementmodels.clear();

            vector<string> models;
            if (Util::splitString(value->get_value(), models, " \t")) {
                vector<string>::iterator it(models.begin());
                for (; it != models.end(); ++it)
                    _screen_placementmodels.push_back(ParseUtil::getValue<PlacementModel>(*it, _placement_map));
            }
        }

        CfgParser::Entry *sub_2 = sub->find_section("SMART");
        if (sub_2) {
            key_list.push_back(new CfgParserKeyBool("ROW", _screen_placement_row));
            key_list.push_back(new CfgParserKeyBool("LEFTTORIGHT", _screen_placement_ltr));
            key_list.push_back(new CfgParserKeyBool("TOPTOBOTTOM", _screen_placement_ttb));
            key_list.push_back(new CfgParserKeyInt("OFFSETX", _screen_placement_offset_x, 0, 0));
            key_list.push_back(new CfgParserKeyInt("OFFSETY", _screen_placement_offset_y, 0, 0));

            // Do the parsing
            sub_2->parse_key_values(key_list.begin(), key_list.end());

            // Freeup resources
            for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
            key_list.clear();
        }
    }

    // Fallback value
    if (! _screen_placementmodels.size()) {
        _screen_placementmodels.push_back(PLACE_MOUSE_CENTERED);
    }

    sub = section->find_section("UNIQUENAMES");
    if (sub) {
        key_list.push_back(new CfgParserKeyBool("SETUNIQUE", _screen_client_unique_name));
        key_list.push_back(new CfgParserKeyString("PRE", _screen_client_unique_name_pre));
        key_list.push_back(new CfgParserKeyString("POST", _screen_client_unique_name_post));

        // Parse data
        sub->parse_key_values(key_list.begin(), key_list.end());

        // Free up resources
        for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
        key_list.clear();
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

    list<CfgParserKey*> key_list;
    string value_select, value_enter, value_exec;

    key_list.push_back(new CfgParserKeyString("SELECT", value_select, "MOTION", 0));
    key_list.push_back(new CfgParserKeyString("ENTER", value_enter, "BUTTONPRESS", 0));
    key_list.push_back(new CfgParserKeyString("EXEC", value_exec, "BUTTONRELEASE", 0));

    // Parse data
    section->parse_key_values(key_list.begin(), key_list.end());

    _menu_select_mask = getMenuMask(value_select);
    _menu_enter_mask = getMenuMask(value_enter);
    _menu_exec_mask = getMenuMask(value_exec);

    // Free up resources
    for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
    key_list.clear();
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

  list<CfgParserKey*> key_list;

  key_list.push_back(new CfgParserKeyBool("HISTORYUNIQUE", _cmd_dialog_history_unique));
  key_list.push_back(new CfgParserKeyInt("HISTORYSIZE", _cmd_dialog_history_size, 1024, 1));
  key_list.push_back(new CfgParserKeyPath("HISTORYFILE", _cmd_dialog_history_file, "~/.pekwm/history"));
  key_list.push_back(new CfgParserKeyInt("HISTORYSAVEINTERVAL", _cmd_dialog_history_save_interval, 16, 0));

  section->parse_key_values(key_list.begin(), key_list.end());

  for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
}

#ifdef HARBOUR
//! @brief Loads the HARBOUR section of the main configuration
void
Config::loadHarbour(CfgParser::Entry *section)
{
    if (! section) {
        return;
    }

    list<CfgParserKey*> key_list;
    string value_placement, value_orientation;

    key_list.push_back(new CfgParserKeyBool("ONTOP", _harbour_ontop));
    key_list.push_back(new CfgParserKeyBool("MAXIMIZEOVER", _harbour_maximize_over));
    key_list.push_back(new CfgParserKeyInt("HEAD", _harbour_head_nr, 0, 0));
    key_list.push_back(new CfgParserKeyString("PLACEMENT", value_placement, "RIGHT", 0));
    key_list.push_back(new CfgParserKeyString("ORIENTATION", value_orientation, "TOPTOBOTTOM", 0));

    // Parse data
    section->parse_key_values(key_list.begin(), key_list.end());

    // Free up resources
    for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
    key_list.clear();

    _harbour_placement = ParseUtil::getValue<HarbourPlacement>(value_placement, _harbour_placement_map);
    _harbour_orientation = ParseUtil::getValue<Orientation>(value_orientation, _harbour_orientation_map);
    if (_harbour_placement == NO_HARBOUR_PLACEMENT) {
        _harbour_placement = TOP;
    }
    if (_harbour_orientation == NO_ORIENTATION) {
        _harbour_orientation = TOP_TO_BOTTOM;
    }

    CfgParser::Entry *sub = section->find_section("DOCKAPP");
    if (sub) {
        key_list.push_back(new CfgParserKeyInt("SIDEMIN", _harbour_da_min_s, 64, 0));
        key_list.push_back(new CfgParserKeyInt("SIDEMAX", _harbour_da_max_s, 64, 0));

        // Parse data
        sub->parse_key_values(key_list.begin(), key_list.end());

        // Free up resources
        for_each(key_list.begin(), key_list.end(), Util::Free<CfgParserKey*>());
        key_list.clear();
    }
}
#endif

//! @brief
ActionType
Config::getAction(const std::string &name, uint mask)
{
    pair<ActionType, uint> val(ParseUtil::getValue<pair<ActionType, uint> >(name, _action_map));

    if ((val.first != ACTION_NO) && (val.second&mask)) {
        return val.first;
    }
    return ACTION_NO;
}

//! @brief
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
            key = strtol(tok[num].c_str() + 1, NULL, 10);
        } else {
            key = XKeysymToKeycode(PScreen::instance()->getDpy(),
                                   XStringToKeysym(tok[num].c_str()));
            if (strcasecmp(tok[num].c_str(), "ANY") == 0) {
                key = 0; // FIXME: for now, but there's no XK_Any?
            }
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
                case ACTION_SEND_KEY:
#ifdef MENUS
                case ACTION_MENU_DYN:
#endif // MENUS
                    action.setParamS(tok[1]);
                    break;
                case ACTION_ACTIVATE_CLIENT_REL:
                case ACTION_MOVE_CLIENT_REL:
                case ACTION_GOTO_CLIENT_ID:
                    action.setParamI(0, strtol(tok[1].c_str(), NULL, 10));
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
                    action.setParamI(0, strtol(tok[1].c_str(), NULL, 10) - 1);
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
#ifdef MENUS
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
#endif // MENUS
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

//! @brief
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

//! @brief
bool
Config::parseActions(const std::string &action_string, ActionEvent &ae, uint mask)
{
    vector<string> tok;
    vector<string>::iterator it;
    Action action;

    // reset the action event
    ae.action_list.clear();

    // chop the string up separating the actions
    if (Util::splitString(action_string, tok, ";")) {
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

//! @brief
bool
Config::parseActionEvent(CfgParser::Entry *section, ActionEvent &ae, uint mask, bool button)
{
    CfgParser::Entry *value = section->find_entry("ACTIONS");
    if (! value && section->get_section()) {
        value = section->get_section()->find_entry("ACTIONS");
    }

    if (! value) {
        return false;
    }

    string str_button = section->get_value();
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
        return parseActions(value->get_value(), ae, mask);
    }
    return false;
}

//! @brief
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
                    action.setParamI(0, strtol(tok[1].c_str(), NULL, 10));
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

//! @brief
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

    if (! section->get_value().size ()) {
        return false;
    }
    
    if (parseKey(section->get_value(), ae.mod, ae.sym)) {
        value = section->get_section()->find_entry("ACTIONS");
        if (value) {
            return parseMoveResizeActions(value->get_value(), ae);
        }
    }

    return false;
}

//! @brief
bool
Config::parseInputDialogAction(const std::string &val, Action &action)
{
    action.setAction(ParseUtil::getValue<InputDialogAction>(val, _inputdialog_map));
    return (action.getAction() != INPUT_NO_ACTION);
}

//! @brief
bool
Config::parseInputDialogActions(const std::string &actions, ActionEvent &ae)
{
    vector<string> tok;
    vector<string>::iterator it;
    Action action;

    // reset the action event
    ae.action_list.clear();

    // chop the string up separating the actions
    if (Util::splitString(actions, tok, ";")) {
        for (it = tok.begin(); it != tok.end(); ++it) {
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

    if (! section->get_value().size()) {
        return false;
    }
    
    if (parseKey(section->get_value(), ae.mod, ae.sym)) {
        value = section->get_section()->find_entry("ACTIONS");
        if (value) {
            return parseInputDialogActions(value->get_value(), ae);
        }
    }

    return false;
}

//! @brief
uint
Config::getMenuMask(const std::string &mask)
{
    uint mask_return = 0, val;

    vector<string> tok;
    Util::splitString(mask, tok, " \t");

    vector<string>::iterator it(tok.begin());
    for (; it != tok.end(); ++it) {
        val = ParseUtil::getValue<MouseEventType>(*it, _mouse_event_map);
        if (val < MOUSE_EVENT_ENTER) {
            mask_return |= val;
        }
    }
    return mask_return;
}

#ifdef MENUS
//! @brief
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

//! @brief
bool
Config::parseMenuActions(const std::string &actions, ActionEvent &ae)
{
    vector<string> tok;
    vector<string>::iterator it;
    Action action;

    // reset the action event
    ae.action_list.clear();

    // chop the string up separating the actions
    if (Util::splitString(actions, tok, ";")) {
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

    if (! section->get_value().size()) {
        return false;
    }

    if (parseKey(section->get_value(), ae.mod, ae.sym)) {
        value = section->get_section()->find_entry("ACTIONS");
        if (value) {
            return parseMenuActions(value->get_value(), ae);
        }
    }

    return false;
}
#endif // MENUS

//! @brief
uint
Config::getMouseButton(const std::string &button)
{
    uint btn;

    if (button.size() == 1) { // it's a button
        btn = unsigned(strtol(button.c_str(), NULL, 10));
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

    bool cp_config, cp_keys, cp_mouse, cp_menu;
    bool cp_autoprops, cp_start, cp_vars;
    cp_config = cp_keys = cp_mouse = cp_menu = false;
    cp_autoprops = cp_start = cp_vars = false;

    struct stat stat_buf;
    // check and see if we allready have a ~/.pekwm/ directory
    if (stat(cfg_dir.c_str(), &stat_buf) == 0) {
        // is it a dir or file?
        if (! S_ISDIR(stat_buf.st_mode)) {
            cerr << cfg_dir << " allready exists and isn't a directory" << endl
                 << "Can't copy config files !" << endl;
            return;
        }

        // we allready have a directory, see if it's writeable and executable
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
                << " directory! Therefor I can't copy the config files!" << endl;
                return;
            }
        }

        // we apperently could write and exec that dir, now see if we have any
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

    } else { // we didn't have a ~/.pekwm directory allready, lets create one
        if (mkdir(cfg_dir.c_str(), 0700)) {
            cerr << "Can't create " << cfg_dir << " directory!" << endl;
            cerr << "Can't copy config files !" << endl;
            return;
        }

        cp_config = cp_keys = cp_mouse = cp_menu = true;
        cp_autoprops = cp_start = cp_vars = true;
    }

    if (cp_config)
        copyTextFile(SYSCONFDIR "/config", cfg_file);
    if (cp_keys)
        copyTextFile(SYSCONFDIR "/keys", keys_file);
    if (cp_mouse)
        copyTextFile(SYSCONFDIR "/mouse", mouse_file);
    if (cp_menu)
        copyTextFile(SYSCONFDIR "/menu", menu_file);
    if (cp_autoprops)
        copyTextFile(SYSCONFDIR "/autoproperties", autoprops_file);
    if (cp_start)
        copyTextFile(SYSCONFDIR "/start", start_file);
    if (cp_vars)
        copyTextFile(SYSCONFDIR "/vars", vars_file);
}

//! @brief Copies a single text file.
void
Config::copyTextFile(const std::string &from, const std::string &to)
{
    if ((from.length() == 0) || (to.length() == 0)) {
        return;
    }

    ifstream stream_from(from.c_str());
    if (! stream_from.good()) {
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Can't copy: " << from << " to: " << to << endl
             << "Shutting down" << endl;
        exit(1);
    }

    ofstream stream_to(to.c_str());
    if (! stream_to.good()) {
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Can't copy: " << from << " to: " << to << endl;
    }

    stream_to << stream_from.rdbuf();
}

//! @brief Parses the Section for mouse actions.
void
Config::loadMouseConfig(const std::string &file)
{
    if (! file.size()) {
        return;
    }
    
    CfgParser mouse_cfg;

    bool success = mouse_cfg.parse(file, CfgParserSource::SOURCE_FILE);
    if (! success) {
        success = mouse_cfg.parse(string(SYSCONFDIR "/mouse"), CfgParserSource::SOURCE_FILE);
    }
    
    if (! success) {
        return;
    }

    // Make sure old actions get unloaded.
    map<MouseActionListName, list<ActionEvent>* >::iterator it;
    for (it = _mouse_action_map.begin(); it != _mouse_action_map.end(); ++it) {
        it->second->clear();
    }

    CfgParser::Entry *section;

    section = mouse_cfg.get_entry_root()->find_section("FRAMETITLE");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_TITLE_FRAME], FRAME_OK);
    }

    section = mouse_cfg.get_entry_root()->find_section("OTHERTITLE");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_TITLE_OTHER], FRAME_OK);
    }
    
    section = mouse_cfg.get_entry_root()->find_section("CLIENT");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_CHILD_FRAME], CLIENT_OK);
    }
    
    section = mouse_cfg.get_entry_root()->find_section("ROOT");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_ROOT], ROOTCLICK_OK);
    }
    
    section = mouse_cfg.get_entry_root()->find_section("MENU");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_MENU], FRAME_OK);
    }
    
    section = mouse_cfg.get_entry_root()->find_section("OTHER");
    if (section) {
        parseButtons(section, _mouse_action_map[MOUSE_ACTION_LIST_OTHER], FRAME_OK);
    }
    
    section = mouse_cfg.get_entry_root()->find_section("SCREENEDGE");
    if (section) {
        CfgParser::iterator edge_it(section->begin());
        for (; edge_it != section->end(); ++edge_it)  {
            uint pos = ParseUtil::getValue<DirectionType>((*edge_it)->get_name(), _direction_map);

            if (pos != SCREEN_EDGE_NO) {
                parseButtons((*edge_it), getEdgeListFromPosition(pos), SCREEN_EDGE_OK);
            }
        }
    }
    
    section = mouse_cfg.get_entry_root()->find_section("BORDER");
    if (section) {
        CfgParser::iterator border_it(section->begin());
        for (; border_it != section->end(); ++border_it) {
            uint pos = ParseUtil::getValue<BorderPosition>((*border_it)->get_name(), _borderpos_map);
            if (pos != BORDER_NO_POS) {
                parseButtons((*border_it), getBorderListFromPosition(pos), FRAME_BORDER_OK);
            }
        }
    }
}

//! @brief Parses mouse config section, like FRAME
void
Config::parseButtons(CfgParser::Entry *section, std::list<ActionEvent>* mouse_list, ActionOk action_ok)
{
    if (! section || ! mouse_list) {
        return;
    }

    ActionEvent ae;
    CfgParser::Entry *value;

    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        if (! (*it)->get_section()) {
            continue;
        }
        
        ae.type = ParseUtil::getValue<MouseEventType>((*it)->get_name(), _mouse_event_map);

        if (ae.type == MOUSE_EVENT_NO) {
            continue;
        }

        if (ae.type == MOUSE_EVENT_MOTION) {
            value = (*it)->get_section()->find_entry("THRESHOLD");
            if (value) {
                ae.threshold = strtol(value->get_value().c_str(), NULL, 10);
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

list<ActionEvent>*
Config::getBorderListFromPosition(uint pos)
{
    list<ActionEvent> *ret = NULL;

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

list<ActionEvent>*
Config::getEdgeListFromPosition(uint pos)
{
    list<ActionEvent> *ret = NULL;

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
        if (Util::splitString(workspace, tok, "x", 2) == 2) {
            uint row = strtol(tok[0].c_str(), NULL, 10) - 1;
            uint col = strtol(tok[1].c_str(), NULL, 10) - 1;

            num = _screen_workspaces_per_row * row + col;

        } else {
            num = strtol(workspace.c_str(), NULL, 10) - 1;
        }
    }

    // Fallback to 0 if something went wrong
    if (num < 0) {
        num = 0;
    }

    return num;
}
