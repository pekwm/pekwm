//
// Config.cc for pekwm
// Copyright (C) 2002-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
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
_screen_workspaces(4), _screen_pixmap_cache_size(20),_screen_edge_size(0),
_screen_doubleclicktime(250), _screen_showframelist(true),
_screen_place_new(true), _screen_focus_new(false),
_screen_focus_new_child (true),
_screen_placement_row(false),
_screen_placement_ltr(true), _screen_placement_ttb(true),
_screen_placement_offset_x(0), _screen_placement_offset_y(0),
_screen_client_unique_name(true),
_screen_client_unique_name_pre(" #"), _screen_client_unique_name_post(""),
_menu_select_mask(0), _menu_enter_mask(0), _menu_exec_mask(0),
#ifdef HARBOUR
_harbour_da_min_s(0), _harbour_da_max_s(0),
#ifdef HAVE_XINERAMA
_harbour_head_nr(0),
#endif // HAVE_XINERAMA
#endif // HARBOUR
_viewport_cols(1), _viewport_rows(1)
{
	if (_instance != NULL) {
		throw string("Config, trying to create multiple instances");
	}
	_instance = this;

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
	_action_map["WARPTOVIEWPORT"] = pair<ActionType, uint>(ACTION_WARP_TO_VIEWPORT, SCREEN_EDGE_OK);
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
	_action_map["VIEWPORTMOVEXY"] = pair<ActionType, uint>(ACTION_VIEWPORT_MOVE_XY, ANY_MASK);
	_action_map["VIEWPORTMOVEDRAG"] = pair<ActionType, uint>(ACTION_VIEWPORT_MOVE_DRAG, ANY_MASK);
	_action_map["VIEWPORTMOVEDIRECTION"] = pair<ActionType, uint>(ACTION_VIEWPORT_MOVE_DIRECTION, ANY_MASK);
	_action_map["VIEWPORTSCROLL"] = pair<ActionType, uint>(ACTION_VIEWPORT_SCROLL, ANY_MASK);
	_action_map["VIEWPORTGOTO"] = pair<ActionType, uint>(ACTION_VIEWPORT_GOTO, ANY_MASK);
	_action_map["SENDTOVIEWPORT"] = pair<ActionType, uint>(ACTION_SEND_TO_VIEWPORT, ANY_MASK);
	_action_map["FINDCLIENT"] = pair<ActionType, uint>(ACTION_FIND_CLIENT, ANY_MASK);
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

	_cmddialog_map[""] = CMD_D_NO_ACTION;
	_cmddialog_map["INSERT"] = CMD_D_INSERT;
	_cmddialog_map["ERASE"] = CMD_D_REMOVE;
	_cmddialog_map["CLEAR"] = CMD_D_CLEAR;
	_cmddialog_map["EXEC"] = CMD_D_EXEC;
	_cmddialog_map["CLOSE"] = CMD_D_CLOSE;
	_cmddialog_map["COMPLETE"] = CMD_D_COMPLETE;
	_cmddialog_map["CURSNEXT"] = CMD_D_CURS_NEXT;
	_cmddialog_map["CURSPREV"] = CMD_D_CURS_PREV;
	_cmddialog_map["CURSEND"] = CMD_D_CURS_END;
	_cmddialog_map["CURSBEGIN"] = CMD_D_CURS_BEGIN;
	_cmddialog_map["HISTNEXT"] = CMD_D_HIST_NEXT;
	_cmddialog_map["HISTPREV"] = CMD_D_HIST_PREV;

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
  if (_config_file.size ())
    {
      success = cfg.parse (config_file, CfgParserSource::SOURCE_FILE);
      if (success)
        file_success = config_file;
    }

  // Try loading ~/.pekwm/config
  if (!success)
    {
      _config_file = string(getenv("HOME")) + string("/.pekwm/config");
      success = cfg.parse (_config_file, CfgParserSource::SOURCE_FILE);
      if (success)
        file_success = _config_file;
    }

  // Copy cfg files to ~/.pekwm and then try loading global config
  if (!success)
    {
      copyConfigFiles();

      _config_file = string(SYSCONFDIR "/config");
      success = cfg.parse (_config_file, CfgParserSource::SOURCE_FILE);
      if (success)
        file_success = _config_file;
    }

  if (!success)
    {
      cerr << " *** WARNING: unable to load configuration files!" << endl;
      return;
    }

  // Update PEKWM_CONFIG_FILE environment if needed ( to reflect active file )
  char *cfg_env = getenv("PEKWM_CONFIG_FILE");
  if (!cfg_env || (strcmp(cfg_env, file_success.c_str()) != 0))
    setenv("PEKWM_CONFIG_FILE", file_success.c_str(), 1);

  string o_file_mouse; // temporary filepath for mouseconfig

  CfgParser::Entry *op_section;

  // Get other config files dests.
  op_section = cfg.get_entry_root ()->find_section ("FILES");
  if (op_section) {
		loadFiles(op_section->get_section());
	}

  // Parse moving / resizing options.
  op_section = cfg.get_entry_root ()->find_section ("MOVERESIZE");
  if (op_section) {
		loadMoveReszie(op_section->get_section());
	}

  // Screen, important stuff such as number of workspaces
  op_section = cfg.get_entry_root ()->find_section ("SCREEN");
  if (op_section) {
		loadScreen(op_section->get_section());
	}

  op_section = cfg.get_entry_root ()->find_section ("MENU");
  if (op_section) {
		loadMenu(op_section->get_section());
	}

#ifdef HARBOUR
  op_section = cfg.get_entry_root ()->find_section ("HARBOUR");
  if (op_section) {
		loadHarbour(op_section->get_section());
	}

#endif // HARBOUR
}

//! @brief Loads file section of configuration
//! @param op_section Pointer to FILES section.
void
Config::loadFiles(CfgParser::Entry *op_section)
{
	if (!op_section) {
		return;
	}

	// Mouse file loading in here as well
	string file_mouse;

	list<CfgParserKey*> o_key_list;
	o_key_list.push_back (new CfgParserKeyPath ("KEYS", _files_keys,
																							SYSCONFDIR "/keys"));
	o_key_list.push_back (new CfgParserKeyPath ("MOUSE", file_mouse,
																							SYSCONFDIR "/mouse"));
	o_key_list.push_back (new CfgParserKeyPath ("MENU", _files_menu,
																							SYSCONFDIR "/menu"));
	o_key_list.push_back (new CfgParserKeyPath ("START", _files_start,
																							SYSCONFDIR "/start"));
	o_key_list.push_back (new CfgParserKeyPath ("AUTOPROPS", _files_autoprops,
																							SYSCONFDIR "/autoproperties"));
	o_key_list.push_back (new CfgParserKeyPath ("THEME", _files_theme,
																							DATADIR "/pekwm/themes/default/theme"));

	// Parse
	op_section->parse_key_values (o_key_list.begin (), o_key_list.end ());

	// Free up resources
	for_each (o_key_list.begin (), o_key_list.end (),
						Util::Free<CfgParserKey*>());


  // Load the mouse configuration
  loadMouseConfig(file_mouse);
}

//! @brief Loads MOVERESIZE section of main configuration
//! @param op_section Pointer to MOVERESIZE section.
void
Config::loadMoveReszie(CfgParser::Entry *op_section)
{
	if (!op_section) {
		return;
	}

	list<CfgParserKey*> o_key_list;
	o_key_list.push_back (new CfgParserKeyInt ("EDGEATTRACT",
																						 _moveresize_edgeattract, 0, 0));
	o_key_list.push_back (new CfgParserKeyInt ("EDGERESIST",
																						 _moveresize_edgeresist, 0, 0));
	o_key_list.push_back (new CfgParserKeyInt ("WINDOWATTRACT",
																						 _moveresize_woattract, 0, 0));
	o_key_list.push_back (new CfgParserKeyInt ("WINDOWRESIST",
																						 _moveresize_woresist, 0, 0));
	o_key_list.push_back (new CfgParserKeyBool ("OPAQUEMOVE",
																							_moveresize_opaquemove));
	o_key_list.push_back (new CfgParserKeyBool ("OPAQUERESIZE",
																							_moveresize_opaqueresize));

	// Parse data
	op_section->parse_key_values (o_key_list.begin (), o_key_list.end ());

	// Free up resources
	for_each (o_key_list.begin (), o_key_list.end (),
						Util::Free<CfgParserKey*>());
}

//! @brief Loads SCREEN section of main configuration
//! @param op_section Pointer to SCREEN section.
void
Config::loadScreen(CfgParser::Entry *op_section)
{
	if (! op_section) {
		return;
	}

	// Parse data
	CfgParser::Entry *value;

	list<CfgParserKey*> o_key_list;
	o_key_list.push_back (new CfgParserKeyInt ("WORKSPACES",
																						 _screen_workspaces, 4, 1));
	o_key_list.push_back (new CfgParserKeyInt ("PIXMAPCACHESIZE",
																						 _screen_pixmap_cache_size));
	o_key_list.push_back (new CfgParserKeyInt ("EDGESIZE", _screen_edge_size, 0, 0));
	o_key_list.push_back (new CfgParserKeyInt ("DOUBLECLICKTIME",
																						 _screen_doubleclicktime, 250, 0));
	o_key_list.push_back (new CfgParserKeyString ("TRIMTITLE",
																								_screen_trim_title));
	o_key_list.push_back (new CfgParserKeyBool ("SHOWFRAMELIST",
																							_screen_showframelist));
	o_key_list.push_back (new CfgParserKeyBool ("SHOWSTATUSWINDOW",
																							_screen_show_status_window));

	o_key_list.push_back (new CfgParserKeyBool ("PLACENEW", _screen_place_new));
	o_key_list.push_back (new CfgParserKeyBool ("FOCUSNEW", _screen_focus_new));
	o_key_list.push_back (new CfgParserKeyBool ("FOCUSNEWCHILD", _screen_focus_new_child, true));

	// Parse data
	op_section->parse_key_values (o_key_list.begin (), o_key_list.end ());

	// Free up resources
	for_each (o_key_list.begin (), o_key_list.end (),
						Util::Free<CfgParserKey*>());
	o_key_list.clear();

	CfgParser::Entry *op_sub = op_section->find_section("VIEWPORTS");
	if (op_sub) {
		op_sub = op_sub->get_section ();

		o_key_list.push_back (new CfgParserKeyInt ("COLUMNS", _viewport_cols,
																							 1, 1));
		o_key_list.push_back (new CfgParserKeyInt ("ROWS", _viewport_rows,
																							 1, 1));

		// Parse data
		op_sub->parse_key_values (o_key_list.begin (), o_key_list.end ());

		// Free up resources
		for_each (o_key_list.begin (), o_key_list.end (),
							Util::Free<CfgParserKey*>());
		o_key_list.clear();
	}

	op_sub = op_section->find_section ("PLACEMENT");
	if (op_sub) {
		op_sub = op_sub->get_section ();

		value = op_sub->find_entry ("MODEL");
		if (value) {
			_screen_placementmodels.clear ();

			vector<string> models;
			if (Util::splitString (value->get_value (), models, " \t")) {
				vector<string>::iterator it( models.begin());
				for (; it != models.end(); ++it)
					_screen_placementmodels.push_back(ParseUtil::getValue<PlacementModel> (*it, _placement_map));
			}
		}

		CfgParser::Entry *op_sub_2 = op_sub->find_section ("SMART");
		if (op_sub_2) {
			op_sub_2 = op_sub_2->get_section ();

			o_key_list.push_back (new CfgParserKeyBool ("ROW",
																									_screen_placement_row));
			o_key_list.push_back (new CfgParserKeyBool ("LEFTTORIGHT",
																									_screen_placement_ltr));
			o_key_list.push_back (new CfgParserKeyBool ("TOPTOBOTTOM",
																									_screen_placement_ttb));
			o_key_list.push_back (new CfgParserKeyInt ("OFFSETX",
																								 _screen_placement_offset_x, 0, 0));
			o_key_list.push_back (new CfgParserKeyInt ("OFFSETY",
																								 _screen_placement_offset_y, 0, 0));

			// Do the parsing
			op_sub_2->parse_key_values (o_key_list.begin (), o_key_list.end ());

			// Freeup resources
			for_each (o_key_list.begin (), o_key_list.end (),
								Util::Free<CfgParserKey*>());
			o_key_list.clear();
		}
	}

	// Fallback value
	if (!_screen_placementmodels.size()) {
		_screen_placementmodels.push_back(PLACE_MOUSE_CENTERED);
	}

	op_sub = op_section->find_section ("UNIQUENAMES");
	if (op_sub) {
		op_sub = op_sub->get_section ();

		o_key_list.push_back (new CfgParserKeyBool ("SETUNIQUE",
																								_screen_client_unique_name));
		o_key_list.push_back (new CfgParserKeyString ("PRE",
																									_screen_client_unique_name_pre));
		o_key_list.push_back (new CfgParserKeyString ("POST",
																									_screen_client_unique_name_post));

		// Parse data
		op_sub->parse_key_values (o_key_list.begin (), o_key_list.end ());

		// Free up resources
		for_each (o_key_list.begin (), o_key_list.end (),
							Util::Free<CfgParserKey*>());
		o_key_list.clear();
	}
}

//! @brief Loads the MENU section of the main configuration
//! @param op_section Pointer to MENU section
void
Config::loadMenu(CfgParser::Entry *op_section)
{
	if (!op_section) {
		return;
	}

	list<CfgParserKey*> o_key_list;
	string o_value_select, o_value_enter, o_value_exec;

	o_key_list.push_back (new CfgParserKeyString ("SELECT", o_value_select,
																								"MOTION", 0));
	o_key_list.push_back (new CfgParserKeyString ("ENTER", o_value_enter,
																								"BUTTONPRESS", 0));
	o_key_list.push_back (new CfgParserKeyString ("EXEC", o_value_exec,
																								"BUTTONRELEASE", 0));

	// Parse data
	op_section->parse_key_values (o_key_list.begin (), o_key_list. end());

	_menu_select_mask = getMenuMask(o_value_select);
	_menu_enter_mask = getMenuMask(o_value_enter);
	_menu_exec_mask = getMenuMask(o_value_exec);

	// Free up resources
	for_each (o_key_list.begin (), o_key_list.end (),
						Util::Free<CfgParserKey*>());
	o_key_list.clear();
}

//! @brief Loads the HARBOUR section of the main configuration
void
Config::loadHarbour(CfgParser::Entry *op_section)
{
	if (!op_section) {
		return;
	}


	list<CfgParserKey*> o_key_list;
	string value_placement, value_orientation;

	o_key_list.push_back (new CfgParserKeyBool ("ONTOP", _harbour_ontop));
	o_key_list.push_back (new CfgParserKeyBool ("MAXIMIZEOVER",
																							_harbour_maximize_over));
#ifdef HAVE_XINERAMA
	o_key_list.push_back (new CfgParserKeyInt ("HEAD", _harbour_head_nr,
																						 0, 0));
#endif // HAVE_XINERAMA
	o_key_list.push_back (new CfgParserKeyString ("PLACEMENT", value_placement,
																								"RIGHT", 0));
	o_key_list.push_back (new CfgParserKeyString ("ORIENTATION",
																								value_orientation,
																								"TOPTOBOTTOM", 0));

	// Parse data
	op_section->parse_key_values (o_key_list.begin (), o_key_list.end ());

	// Free up resources
	for_each (o_key_list.begin (), o_key_list.end (),
						Util::Free<CfgParserKey*>());
	o_key_list.clear();

	_harbour_placement =
		ParseUtil::getValue<HarbourPlacement>(value_placement,
																					_harbour_placement_map);
	_harbour_orientation =
		ParseUtil::getValue<Orientation>(value_orientation,
																		 _harbour_orientation_map);
	if (_harbour_placement == NO_HARBOUR_PLACEMENT)
		_harbour_placement = TOP;
	if (_harbour_orientation == NO_ORIENTATION)
		_harbour_orientation = TOP_TO_BOTTOM;

	CfgParser::Entry *op_sub = op_section->find_section ("DOCKAPP");
	if (op_sub) {
		op_sub = op_sub->get_section ();

		o_key_list.push_back (new CfgParserKeyInt ("SIDEMIN",
																							 _harbour_da_min_s, 64, 0));
		o_key_list.push_back (new CfgParserKeyInt ("SIDEMAX",
																							 _harbour_da_max_s, 64, 0));

		// Parse data
		op_sub->parse_key_values (o_key_list.begin (), o_key_list.end ());
	 
		// Free up resources
		for_each (o_key_list.begin (), o_key_list.end (),
							Util::Free<CfgParserKey*>());
		o_key_list.clear();
	}
}

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
	static vector<string> tok;
	static vector<string>::iterator it;

	static uint num;

	// chop the string up separating mods and the end key/button
	tok.clear();
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
	static vector<string> tok;
	static vector<string>::iterator it;

	// chop the string up separating mods and the end key/button
	tok.clear();
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

bool
Config::parseAction(const std::string &action_string, Action &action, uint mask)
{
	static vector<string> tok;

	// chop the string up separating the action and parameters
	tok.clear();
	if (Util::splitString(action_string, tok, " \t", 2)) {
		action.setAction(getAction(tok[0], mask));
		if (action.getAction() != ACTION_NO) {
			if (tok.size() == 2) { // we got enough tok for a parameter
				switch (action.getAction()) {
				case ACTION_EXEC:
				case ACTION_RESTART_OTHER:
				case ACTION_FIND_CLIENT:
#ifdef MENUS
				case ACTION_MENU_DYN:
#endif // MENUS
					action.setParamS(tok[1]);
					break;
				case ACTION_ACTIVATE_CLIENT_REL:
				case ACTION_MOVE_CLIENT_REL:
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
					action.setParamI(0, ParseUtil::getValue<WorkspaceChangeType>(tok[1], _workspace_change_map));
					if (static_cast<uint>(action.getParamI(0)) == WORKSPACE_NO) {
						action.setParamI(0, strtol(tok[1].c_str(), NULL, 10) - 1);
						if (action.getParamI(0) < 0) {
							action.setParamI(0, 0);
						}
					}
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
					action.setParamI(0, ParseUtil::getValue<BorderPosition>(tok[1], _borderpos_map));
					break;
				case ACTION_WARP_TO_VIEWPORT:
				case ACTION_VIEWPORT_MOVE_DIRECTION:
					action.setParamI(0, ParseUtil::getValue<DirectionType>(tok[1], _direction_map));
					break;
				case ACTION_VIEWPORT_SCROLL:
				case ACTION_VIEWPORT_MOVE_XY:
					if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
						action.setParamI(0, strtol(tok[tok.size() - 2].c_str(), NULL, 10));
						action.setParamI(1, strtol(tok[tok.size() - 1].c_str(), NULL, 10));
					}
					break;
				case ACTION_VIEWPORT_GOTO:
					if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
						action.setParamI(0, strtol(tok[tok.size() - 2].c_str(), NULL, 10) - 1);
						action.setParamI(1, strtol(tok[tok.size() - 1].c_str(), NULL, 10) - 1);
					}
					break;
				case ACTION_SEND_TO_VIEWPORT:
					if ((Util::splitString(tok[1], tok, " \t", 2)) == 2) {
						action.setParamI(0, strtol(tok[tok.size() - 2].c_str(), NULL, 10) - 1);
						action.setParamI(1, strtol(tok[tok.size() - 1].c_str(), NULL, 10) - 1);
					}
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
	static vector<string> tok;

	// chop the string up separating the action and parameters
	tok.clear();
	if (Util::splitString(as_action, tok, " \t", 2)) {
		action.setParamI(0, ParseUtil::getValue<ActionStateType>(tok[0], _action_state_map));
		if (action.getParamI(0) != ACTION_STATE_NO) {
			if (tok.size() == 2) { // we got enough tok for a parameter
				switch (action.getParamI(0)) {
				case ACTION_STATE_MAXIMIZED:
					Util::splitString(tok[1], tok, " \t", 2);
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
	static vector<string> tok;
	static vector<string>::iterator it;
	static Action action;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	tok.clear();
	if (Util::splitString(action_string, tok, ";")) {
		for (it = tok.begin(); it != tok.end(); ++it) {
			if (parseAction(*it, action, mask)) {
				ae.action_list.push_back(action);
				action.setParamS("");
			}
		}

		return true;
	}

	return false;
}

//! @brief
bool
Config::parseActionEvent(CfgParser::Entry *op_section, ActionEvent &ae,
                         uint mask, bool button)
{
  CfgParser::Entry *op_value;

  string o_button = op_section->get_value ();
  if (!o_button.size())
    {
      if ((ae.type == MOUSE_EVENT_ENTER) || (ae.type == MOUSE_EVENT_LEAVE))
        o_button = "1";
      else
        return false;
    }

  bool ok;
  if (button)
    ok = parseButton (o_button, ae.mod, ae.sym);
  else
    ok = parseKey (o_button, ae.mod, ae.sym);

  op_value = op_section->get_section ()->find_entry ("ACTIONS");
  if (ok && op_value)
    return parseActions (op_value->get_value (), ae, mask);
  return false;
}

//! @brief
bool
Config::parseMoveResizeAction(const std::string &action_string, Action &action)
{
  static vector<string> tok;

  // Chop the string up separating the actions.
  tok.clear();
  if (Util::splitString(action_string, tok, " \t", 2))
    {
      action.setAction(ParseUtil::getValue<MoveResizeActionType>(tok[0],
                                                             _moveresize_map));
      if (action.getAction() != NO_MOVERESIZE_ACTION)
        {
          if (tok.size() == 2) { // we got enough tok for a paremeter
            switch (action.getAction())
              {
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
	static vector<string> tok;
	static vector<string>::iterator it;
	static Action action;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	tok.clear();
	if (Util::splitString(action_string, tok, ";")) {
		for (it = tok.begin(); it != tok.end(); ++it) {
			if (parseMoveResizeAction(*it, action)) {
				ae.action_list.push_back(action);
				action.setParamS("");
			}
		}

		return true;
	}

	return false;
}

//! @brief Parses MoveResize Event.
bool
Config::parseMoveResizeEvent(CfgParser::Entry *op_section, ActionEvent& ae)
{
  CfgParser::Entry *op_value;

  if (!op_section->get_value ().size ())
    return false;

  if (parseKey (op_section->get_value (), ae.mod, ae.sym))
    {
      op_value = op_section->get_section ()->find_entry ("ACTIONS");
      if (op_value)
        return parseMoveResizeActions (op_value->get_value (), ae);
    }

  return false;
}

//! @brief
bool
Config::parseCmdDialogAction(const std::string &val, Action &action)
{
	action.setAction(ParseUtil::getValue<CmdDialogAction>(val, _cmddialog_map));
	return (action.getAction() != CMD_D_NO_ACTION);
}

//! @brief
bool
Config::parseCmdDialogActions(const std::string &actions, ActionEvent &ae)
{
	static vector<string> tok;
	static vector<string>::iterator it;
	static Action action;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	tok.clear();
	if (Util::splitString(actions, tok, ";")) {
		for (it = tok.begin(); it != tok.end(); ++it) {
			if (parseCmdDialogAction(*it, action)) {
				ae.action_list.push_back(action);
			}
		}

		return true;
	}

	return false;

}

//! @brief Parses CmdDialog Event.
bool
Config::parseCmdDialogEvent (CfgParser::Entry *op_section, ActionEvent &ae)
{
  CfgParser::Entry *op_value;

  if (!op_section->get_value ().size ())
    return false;

  if (parseKey (op_section->get_value (), ae.mod, ae.sym))
    {
      op_value = op_section->get_section ()->find_entry ("ACTIONS");
      if (op_value)
        return parseCmdDialogActions(op_value->get_value (), ae);
    }

  return false;
}

//! @brief
uint
Config::getMenuMask (const std::string &mask)
{
  uint mask_return = 0, val;

  vector<string> tok;
  Util::splitString (mask, tok, " \t");

  vector<string>::iterator it (tok.begin ());
  for (; it != tok.end(); ++it)
    {
      val = ParseUtil::getValue<MouseEventType> (*it, _mouse_event_map);
      if (val < MOUSE_EVENT_ENTER)
        mask_return |= val;
    }
  return mask_return;
}

#ifdef MENUS
//! @brief
bool
Config::parseMenuAction(const std::string &action_string, Action &action)
{
	static vector<string> tok;

	// chop the string up separating the actions
	tok.clear();
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
	static vector<string> tok;
	static vector<string>::iterator it;
	static Action action;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	tok.clear();
	if (Util::splitString(actions, tok, ";")) {
		for (it = tok.begin(); it != tok.end(); ++it) {
			if (parseMenuAction(*it, action)) {
				ae.action_list.push_back(action);
				action.setParamS("");
			}
		}

		return true;
	}

	return false;
}

//! @brief Parses MenuEvent.
bool
Config::parseMenuEvent (CfgParser::Entry *op_section, ActionEvent& ae)
{
  CfgParser::Entry *op_value;

  if (!op_section->get_value ().size ())
    return false;

  if (parseKey (op_section->get_value (), ae.mod, ae.sym))
    {
      op_value = op_section->get_section ()->find_entry ("ACTIONS");
      if (op_value)
        return parseMenuActions (op_value->get_value (), ae);
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
		if (!S_ISDIR(stat_buf.st_mode)) {
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

		if (!cfg_dir_ok) {
			if (!(stat_buf.st_mode&S_IWOTH) || !(stat_buf.st_mode&(S_IXOTH))) {
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
	if (stream_from.good() == false) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Can't copy: " << from << " to: " << to << endl
				 << "Shutting down" << endl;
		exit(1);
	}

	ofstream stream_to(to.c_str());
	if (stream_to.good() == false) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Can't copy: " << from << " to: " << to << endl;
	}

	stream_to << stream_from.rdbuf();
}

//! @brief Parses the Section for mouse actions.
void
Config::loadMouseConfig(const std::string &file)
{
  if (!file.size())
    return;

  CfgParser mouse_cfg;

  bool success = mouse_cfg.parse (file, CfgParserSource::SOURCE_FILE);
  if (!success)
    success = mouse_cfg.parse (string(SYSCONFDIR "/mouse"),
                               CfgParserSource::SOURCE_FILE);

  if (!success)
    return;

  // Make sure old actions get unloaded.
  map<MouseActionListName, list<ActionEvent>* >::iterator it;
  for (it = _mouse_action_map.begin (); it != _mouse_action_map.end (); ++it)
    it->second->clear ();

  CfgParser::Entry *op_section, *op_sub;

  op_section = mouse_cfg.get_entry_root ()->find_section ("FRAMETITLE");
  if (op_section)
    {
      parseButtons (op_section,
                    _mouse_action_map[MOUSE_ACTION_LIST_TITLE_FRAME], FRAME_OK);
    }
  op_section = mouse_cfg.get_entry_root ()->find_section ("OTHERTITLE");
  if (op_section)
    {
      parseButtons(op_section,
                   _mouse_action_map[MOUSE_ACTION_LIST_TITLE_OTHER], FRAME_OK);
    }
  op_section = mouse_cfg.get_entry_root ()->find_section ("CLIENT");
  if (op_section)
    {
      parseButtons (op_section,
                    _mouse_action_map[MOUSE_ACTION_LIST_CHILD_FRAME], CLIENT_OK);
    }
  op_section = mouse_cfg.get_entry_root ()->find_section ("ROOT");
  if (op_section)
    {
      parseButtons(op_section,
                   _mouse_action_map[MOUSE_ACTION_LIST_ROOT], ROOTCLICK_OK);
    }
  op_section = mouse_cfg.get_entry_root ()->find_section ("MENU");
  if (op_section)
    {
      parseButtons (op_section,
                    _mouse_action_map[MOUSE_ACTION_LIST_MENU], FRAME_OK);
    }
  op_section = mouse_cfg.get_entry_root ()->find_section ("OTHER");
  if (op_section)
    {
      parseButtons (op_section,
                    _mouse_action_map[MOUSE_ACTION_LIST_OTHER], FRAME_OK);
    }
  op_section = mouse_cfg.get_entry_root ()->find_section ("SCREENEDGE");
  if (op_section)
    {
      op_section = op_section->get_section ();
      uint pos;
      for (op_sub = op_section->get_section_next (); op_sub != NULL; op_sub = op_sub->get_section_next ())
        {
          pos = ParseUtil::getValue<DirectionType>(op_sub->get_name (),
                                                   _direction_map);

          if (pos != SCREEN_EDGE_NO)
            parseButtons (op_sub, getEdgeListFromPosition (pos), SCREEN_EDGE_OK);
	}
    }
  op_section = mouse_cfg.get_entry_root ()->find_section ("BORDER");
  if (op_section)
    {
      op_section = op_section->get_section ();
      uint pos;
      for (op_sub = op_section->get_section_next (); op_sub; op_sub = op_sub->get_section_next ())
        {
          pos = ParseUtil::getValue<BorderPosition>(op_sub->get_name(),
                                                    _borderpos_map);
          if (pos != BORDER_NO_POS)
            parseButtons(op_sub, getBorderListFromPosition (pos), FRAME_BORDER_OK);
        }
    }
}

//! @brief Parses mouse config section, like FRAME
void
Config::parseButtons(CfgParser::Entry *op_section,
                     std::list<ActionEvent>* mouse_list, ActionOk action_ok)
{
  if (!op_section || !mouse_list)
    return;
  op_section = op_section->get_section ();

  ActionEvent ae;
  CfgParser::Entry *op_value;

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      ae.type = ParseUtil::getValue<MouseEventType>(op_section->get_name (),
                                                    _mouse_event_map);

      if (ae.type == MOUSE_EVENT_NO)
        continue;

      if (ae.type == MOUSE_EVENT_MOTION)
        {
          op_value = op_section->get_section ()->find_entry ("THRESHOLD");
          if (op_value)
            ae.threshold = strtol (op_value->get_value ().c_str (), NULL, 10);
          else
            ae.threshold = 0;
        }

        if (parseActionEvent (op_section, ae, action_ok, true))
          mouse_list->push_back(ae);
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
