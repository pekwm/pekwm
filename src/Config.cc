//
// Config.cc for pekwm
// Copyright (C) 2002-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Charset.hh"
#include "Config.hh"
#include "Compat.hh"
#include "Debug.hh"
#include "PFont.hh"
#include "Util.hh"
#include "Workspaces.hh"
#include "X11.hh" // for DPY in keyconfig code

#include <fstream>

#include <cstdlib>

extern "C" {
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
}

static Util::StringTo<ActionAccessMask> action_access_mask_map[] =
	{{"MOVE", ACTION_ACCESS_MOVE},
	 {"RESIZE", ACTION_ACCESS_RESIZE},
	 {"ICONIFY", ACTION_ACCESS_ICONIFY},
	 {"SHADE", ACTION_ACCESS_SHADE},
	 {"STICK", ACTION_ACCESS_STICK},
	 {"MAXIMIZEHORIZONTAL", ACTION_ACCESS_MAXIMIZE_HORZ},
	 {"MAXIMIZEVERTICAL", ACTION_ACCESS_MAXIMIZE_VERT},
	 {"FULLSCREEN", ACTION_ACCESS_FULLSCREEN},
	 {"SETWORKSPACE", ACTION_ACCESS_CHANGE_DESKTOP},
	 {"CLOSE", ACTION_ACCESS_CLOSE},
	 {nullptr, ACTION_ACCESS_NO}};

static Util::StringTo<MoveResizeActionType> moveresize_map[] =
	{{"MOVEHORIZONTAL", MOVE_HORIZONTAL},
	 {"MOVEVERTICAL", MOVE_VERTICAL},
	 {"RESIZEHORIZONTAL", RESIZE_HORIZONTAL},
	 {"RESIZEVERTICAL", RESIZE_VERTICAL},
	 {"MOVESNAP", MOVE_SNAP},
	 {"CANCEL", MOVE_CANCEL},
	 {"END", MOVE_END},
	 {nullptr, NO_MOVERESIZE_ACTION}};

static Util::StringTo<InputDialogAction> inputdialog_map[] =
	{{"INSERT", INPUT_INSERT},
	 {"ERASE", INPUT_REMOVE},
	 {"CLEAR", INPUT_CLEAR},
	 {"CLEARFROMCURSOR", INPUT_CLEARFROMCURSOR},
	 {"EXEC", INPUT_EXEC},
	 {"CLOSE", INPUT_CLOSE},
	 {"COMPLETE", INPUT_COMPLETE},
	 {"COMPLETEABORT", INPUT_COMPLETE_ABORT},
	 {"CURSNEXT", INPUT_CURS_NEXT},
	 {"CURSPREV", INPUT_CURS_PREV},
	 {"CURSEND", INPUT_CURS_END},
	 {"CURSBEGIN", INPUT_CURS_BEGIN},
	 {"HISTNEXT", INPUT_HIST_NEXT},
	 {"HISTPREV", INPUT_HIST_PREV},
	 {nullptr, INPUT_NO_ACTION}};

static Util::StringTo<MouseEventType> mouse_event_map[] =
	{{"BUTTONPRESS", MOUSE_EVENT_PRESS},
	 {"BUTTONRELEASE", MOUSE_EVENT_RELEASE},
	 {"DOUBLECLICK", MOUSE_EVENT_DOUBLE},
	 {"MOTION", MOUSE_EVENT_MOTION},
	 {"ENTER", MOUSE_EVENT_ENTER},
	 {"LEAVE", MOUSE_EVENT_LEAVE},
	 {"ENTERMOVING", MOUSE_EVENT_ENTER_MOVING},
	 {"MOTIONPRESSED", MOUSE_EVENT_MOTION_PRESSED},
	 {nullptr, MOUSE_EVENT_NO}};

static Util::StringTo<ActionType> menu_action_map[] =
	{{"NEXTITEM", ACTION_MENU_NEXT},
	 {"PREVITEM", ACTION_MENU_PREV},
	 {"SELECT", ACTION_MENU_SELECT},
	 {"ENTERSUBMENU", ACTION_MENU_ENTER_SUBMENU},
	 {"LEAVESUBMENU", ACTION_MENU_LEAVE_SUBMENU},
	 {"CLOSE", ACTION_CLOSE},
	 {nullptr, ACTION_MENU_NEXT}};

static Util::StringTo<HarbourPlacement> harbour_placement_map[] =
	{{"TOP", TOP},
	 {"LEFT", LEFT},
	 {"RIGHT", RIGHT},
	 {"BOTTOM", BOTTOM},
	 {nullptr, NO_HARBOUR_PLACEMENT}};

static Util::StringTo<Orientation> harbour_orientation_map[] =
	{{"TOPTOBOTTOM", TOP_TO_BOTTOM},
	 {"LEFTTORIGHT", TOP_TO_BOTTOM},
	 {"BOTTOMTOTOP", BOTTOM_TO_TOP},
	 {"RIGHTTOLEFT", BOTTOM_TO_TOP},
	 {nullptr, NO_ORIENTATION}};

static Util::StringTo<CurrHeadSelector> curr_head_selector_map[] =
	{{"CURSOR", CURR_HEAD_SELECTOR_CURSOR},
	 {"FOCUSEDWINDOW", CURR_HEAD_SELECTOR_FOCUSED_WINDOW},
	 {nullptr, CURR_HEAD_SELECTOR_NO}};

/**
 * Parse width and height limits.
 */
bool
SizeLimits::parse(const std::string &minimum, const std::string &maximum)
{
	return (parseLimit(minimum,
			   _limits[WIDTH_MIN], _limits[HEIGHT_MIN])
		&& parseLimit(maximum,
			      _limits[WIDTH_MAX], _limits[HEIGHT_MAX]));
}

/**
 * Parse single limit.
 */
bool
SizeLimits::parseLimit(const std::string &limit, uint &min, uint &max)
{
	bool status = false;
	std::vector<std::string> tokens;
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
	_screen_workspaces_per_row(0),
	_screen_workspace_name_default("Workspace"),
	_screen_edge_indent(false),
	_screen_doubleclicktime(250), _screen_fullscreen_above(true),
	_screen_fullscreen_detect(true),
	_screen_showframelist(true),
	_screen_show_status_window(true),
	_screen_show_status_window_on_root(false),
	_screen_show_client_id(false),
	_screen_show_workspace_indicator(500),
	_screen_workspace_indicator_scale(16),
	_screen_workspace_indicator_opacity(EWMH_OPAQUE_WINDOW),
	_screen_place_new(true), _screen_focus_new(false),
	_screen_focus_new_child(true), _screen_focus_steal_protect(0),
	_screen_honour_randr(true),
	_screen_honour_aspectratio(true),
	_screen_curr_head_selector(CURR_HEAD_SELECTOR_CURSOR),
	_screen_default_font_x11(false),
	_screen_placement_row(false),
	_screen_placement_ltr(true), _screen_placement_ttb(true),
	_screen_placement_offset_x(0), _screen_placement_offset_y(0),
	_screen_client_unique_name(true),
	_screen_client_unique_name_pre(" #"),
	_screen_client_unique_name_post(""),
	_screen_report_all_clients(false),
	_menu_select_mask(0), _menu_enter_mask(0), _menu_exec_mask(0),
	_menu_display_icons(true),
	_menu_focus_opacity(EWMH_OPAQUE_WINDOW),
	_menu_unfocus_opacity(EWMH_OPAQUE_WINDOW),
	_cmd_dialog_history_unique(true), _cmd_dialog_history_size(1024),
	_cmd_dialog_history_save_interval(16),
	_harbour_da_min_s(0), _harbour_da_max_s(0),
	_harbour_ontop(true), _harbour_maximize_over(false),
	_harbour_placement(TOP),
	_harbour_orientation(TOP_TO_BOTTOM),
	_harbour_head_nr(0),
	_harbour_opacity(EWMH_OPAQUE_WINDOW)
{
	for (uint i = 0; i <= SCREEN_EDGE_NO; ++i) {
		_screen_edge_sizes.push_back(0);
	}

	// fill the mouse action map
	for (uint i = 0; i <= MOUSE_ACTION_LIST_NO; i++) {
		MouseActionListName maln = static_cast<MouseActionListName>(i);
		_mouse_action_map[maln] = new std::vector<ActionEvent>();
	}
}

//! @brief Destructor for Config class
Config::~Config(void)
{
	mouse_action_map::iterator it = _mouse_action_map.begin();
	for (; it != _mouse_action_map.end(); ++it) {
		delete it->second;
	}
}

/**
 * Returns an array of NULL-terminated desktop names in UTF-8.
 *
 * @param names *names will be set to an array of desktop names or 0.
 * 	  The caller has to delete [] *names
 * @param length *length will be set to the complete length of array *names
 * 	  points to or 0.
 */
void
Config::getDesktopNamesUTF8(uchar **names, uint *length) const
{
	if (! _screen_workspace_names.size()) {
		*names = 0;
		*length = 0;
		return;
	}

	std::string utf8_names;
	std::vector<std::string>::const_iterator it =
		_screen_workspace_names.begin();
	for (; it != _screen_workspace_names.end(); ++it) {
		utf8_names.append(it->c_str(), it->size() + 1);
	}

	*names = new uchar[utf8_names.size()];
	::memcpy(*names, utf8_names.c_str(), utf8_names.size());
	*length = utf8_names.size();
}

/**
 * Sets the desktop names.
 *
 * @param names names is expected to point to an array of NULL-terminated
 *        utf8-strings.
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
		_screen_workspace_names.push_back(names);
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

	std::string cfg_dir;
	_config_file = config_file;
	bool success = tryHardLoadConfig(cfg, _config_file, cfg_dir);

	// Make sure config is reloaded next time as content is dynamically
	// generated from the configuration file.
	if (! success || cfg.isDynamicContent()) {
		_cfg_files.clear();
	} else {
		_cfg_files = cfg.getCfgFiles();
	}

	if (! success) {
		USER_WARN("unable to load configuration files");
		return false;
	}

	// Update PEKWM_CONFIG_FILE environment if needed (to reflect active
	// file)
	std::string cfg_env = Util::getEnv("PEKWM_CONFIG_FILE");
	if (cfg_env.size() == 0 || _config_file.compare(cfg_env) != 0) {
		Util::setEnv("PEKWM_CONFIG_FILE", _config_file);
		Util::setEnv("PEKWM_CONFIG_PATH", cfg_dir);
	}

	std::string o_file_mouse; // temporary filepath for mouseconfig

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

	CfgParserKeys keys;
	keys.add_path("KEYS", _files_keys, SYSCONFDIR "/keys");
	keys.add_path("MOUSE", _files_mouse, SYSCONFDIR "/mouse");
	keys.add_path("MENU", _files_menu, SYSCONFDIR "/menu");
	keys.add_path("START", _files_start, SYSCONFDIR "/start");
	keys.add_path("AUTOPROPS", _files_autoprops,
		      SYSCONFDIR "/autoproperties");
	keys.add_path("THEME", _files_theme, DATADIR "/pekwm/themes/default");
	keys.add_string("THEMEVARIANT", _files_theme_variant);
	keys.add_path("ICONS", _files_icon_path, DATADIR "/pekwm/icons");

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();
}

//! @brief Loads MOVERESIZE section of main configuration
//! @param section Pointer to MOVERESIZE section.
void
Config::loadMoveResize(CfgParser::Entry *section)
{
	if (! section) {
		return;
	}

	CfgParserKeys keys;
	keys.add_numeric<int>("EDGEATTRACT", _moveresize_edgeattract, 0, 0);
	keys.add_numeric<int>("EDGERESIST", _moveresize_edgeresist, 0, 0);
	keys.add_numeric<int>("WINDOWATTRACT", _moveresize_woattract, 0, 0);
	keys.add_numeric<int>("WINDOWRESIST", _moveresize_woresist, 0, 0);
	keys.add_bool("OPAQUEMOVE", _moveresize_opaquemove);
	keys.add_bool("OPAQUERESIZE", _moveresize_opaqueresize);
	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();
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
	std::string edge_size, workspace_names, trim_title, curr_head_selector;

	CfgParserKeys keys;
	keys.add_bool("THEMEBACKGROUND", _screen_theme_background, true);
	keys.add_numeric<uint>("WORKSPACES", _screen_workspaces, 4, 1);
	keys.add_numeric<uint>("WORKSPACESPERROW", _screen_workspaces_per_row,
			       0, 0);
	keys.add_string("WORKSPACENAMES", workspace_names);
	keys.add_string("EDGESIZE", edge_size);
	keys.add_bool("EDGEINDENT", _screen_edge_indent);
	keys.add_numeric<int>("DOUBLECLICKTIME", _screen_doubleclicktime,
			      250, 0);
	keys.add_string("TRIMTITLE", trim_title);
	keys.add_bool("FULLSCREENABOVE", _screen_fullscreen_above, true);
	keys.add_bool("FULLSCREENDETECT", _screen_fullscreen_detect, true);
	keys.add_bool("SHOWFRAMELIST", _screen_showframelist);
	keys.add_bool("SHOWSTATUSWINDOW", _screen_show_status_window);
	keys.add_bool("SHOWSTATUSWINDOWCENTEREDONROOT",
		      _screen_show_status_window_on_root, false);
	keys.add_bool("SHOWCLIENTID", _screen_show_client_id);
	keys.add_numeric<int>("SHOWWORKSPACEINDICATOR",
			      _screen_show_workspace_indicator, 500, 0);
	keys.add_numeric<int>("WORKSPACEINDICATORSCALE",
			      _screen_workspace_indicator_scale, 16, 2);
	keys.add_numeric<uint>("WORKSPACEINDICATOROPACITY",
			       _screen_workspace_indicator_opacity,
			       100, 0, 100);
	keys.add_bool("PLACENEW", _screen_place_new);
	keys.add_bool("FOCUSNEW", _screen_focus_new);
	keys.add_bool("FOCUSNEWCHILD", _screen_focus_new_child, true);
	keys.add_numeric<uint>("FOCUSSTEALPROTECT",
			       _screen_focus_steal_protect, 0);
	keys.add_bool("HONOURRANDR", _screen_honour_randr, true);
	keys.add_bool("HONOURASPECTRATIO", _screen_honour_aspectratio, true);
	keys.add_string("CURRHEADSELECTOR", curr_head_selector, "CURSOR");
	keys.add_bool("FONTDEFAULTX11", _screen_default_font_x11, false);
	keys.add_string("FONTCHARSETOVERRIDE", _screen_font_charset_override);
	keys.add_bool("REPORTALLCLIENTS", _screen_report_all_clients, false);

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	PFont::setTrimString(trim_title);

	// Convert opacity from percent to absolute value
	CONV_OPACITY(_screen_workspace_indicator_opacity);

	int edge_size_all = 0;
	_screen_edge_sizes.clear();
	if (edge_size.size()) {
		std::vector<std::string> sizes;
		if (Util::splitString(edge_size, sizes, " \t", 4) == 4) {
			std::vector<std::string>::iterator it = sizes.begin();
			for (; it != sizes.end(); ++it) {
				_screen_edge_sizes.push_back(std::stoi(*it));
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

	std::vector<std::string> vs;
	if (Util::splitString(workspace_names, vs, ";", 0, true)) {
		std::vector<std::string>::iterator vs_it = vs.begin();
		for (; vs_it != vs.end(); ++vs_it) {
			_screen_workspace_names.push_back(*vs_it);
		}
	}

	_screen_curr_head_selector =
		Util::StringToGet(curr_head_selector_map, curr_head_selector);

	CfgParser::Entry *sub = section->findSection("PLACEMENT");
	loadScreenPlacement(sub);

	sub = section->findSection("UNIQUENAMES");
	if (sub) {
		keys.add_bool("SETUNIQUE", _screen_client_unique_name);
		keys.add_string("PRE", _screen_client_unique_name_pre);
		keys.add_string("POST", _screen_client_unique_name_post);

		sub->parseKeyValues(keys.begin(), keys.end());
		keys.clear();
	}
}

/**
 * Load Placement sub section of the Screen section.
 */
void
Config::loadScreenPlacement(CfgParser::Entry *section)
{
	if (section == nullptr) {
		return;
	}

	CfgParser::Entry *value = section->findEntry("MODEL");
	if (value) {
		std::vector<std::string> tok;
		Util::splitString(value->getValue(), tok, " \t");
		Workspaces::setLayoutModels(tok);
	}

	CfgParser::Entry *sub = section->findSection("SMART");
	if (sub) {
		CfgParserKeys keys;
		keys.add_bool("ROW", _screen_placement_row);
		keys.add_bool("LEFTTORIGHT", _screen_placement_ltr);
		keys.add_bool("TOPTOBOTTOM", _screen_placement_ttb);
		keys.add_numeric<int>("OFFSETX", _screen_placement_offset_x,
				      0, 0);
		keys.add_numeric<int>("OFFSETY", _screen_placement_offset_y,
				      0, 0);
		sub->parseKeyValues(keys.begin(), keys.end());
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

	CfgParserKeys keys;
	std::string value_select, value_enter, value_exec;

	keys.add_string("SELECT", value_select, "MOTION", 0);
	keys.add_string("ENTER", value_enter, "BUTTONPRESS", 0);
	keys.add_string("EXEC", value_exec, "BUTTONRELEASE", 0);
	keys.add_bool("DISPLAYICONS", _menu_display_icons, true);
	keys.add_numeric<uint>("FOCUSOPACITY", _menu_focus_opacity,
			       100, 0, 100);
	keys.add_numeric<uint>("UNFOCUSOPACITY", _menu_unfocus_opacity,
			       100, 0, 100);

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	_menu_select_mask = getMenuMask(value_select);
	_menu_enter_mask = getMenuMask(value_enter);
	_menu_exec_mask = getMenuMask(value_exec);

	// Parse icon size limits
	CfgParser::Entry::entry_cit it(section->begin());
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

	CfgParserKeys keys;
	std::string minimum, maximum;

	keys.add_string("MINIMUM", minimum, "16x16", 3);
	keys.add_string("MAXIMUM", maximum, "16x16", 3);

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

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

	CfgParserKeys keys;

	keys.add_bool("HISTORYUNIQUE", _cmd_dialog_history_unique);
	keys.add_numeric<int>("HISTORYSIZE", _cmd_dialog_history_size,
			      1024, 1);
	keys.add_path("HISTORYFILE", _cmd_dialog_history_file,
		      Util::getConfigDir() + "/history");
	keys.add_numeric<int>("HISTORYSAVEINTERVAL",
			      _cmd_dialog_history_save_interval, 16, 0);

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();
}
//! @brief Loads the HARBOUR section of the main configuration
void
Config::loadHarbour(CfgParser::Entry *section)
{
	if (! section) {
		return;
	}

	CfgParserKeys keys;
	std::string value_placement, value_orientation;

	keys.add_bool("ONTOP", _harbour_ontop, true);
	keys.add_bool("MAXIMIZEOVER", _harbour_maximize_over, false);
	keys.add_numeric<int>("HEAD", _harbour_head_nr, 0, 0);
	keys.add_string("HEADNAME", _harbour_head, "", 0);
	keys.add_string("PLACEMENT", value_placement, "RIGHT", 0);
	keys.add_string("ORIENTATION", value_orientation, "TOPTOBOTTOM", 0);
	keys.add_numeric<uint>("OPACITY", _harbour_opacity, 100, 0, 100);
	keys.add_numeric<uint>("OPACITY", _harbour_opacity, 100, 0, 100);

	section->parseKeyValues(keys.begin(), keys.end());
	keys.clear();

	// Convert opacity from percent to absolute value
	CONV_OPACITY(_harbour_opacity);

	_harbour_placement =
		Util::StringToGet(harbour_placement_map, value_placement);
	_harbour_orientation =
		Util::StringToGet(harbour_orientation_map, value_orientation);
	if (_harbour_placement == NO_HARBOUR_PLACEMENT) {
		_harbour_placement = RIGHT;
	}
	if (_harbour_orientation == NO_ORIENTATION) {
		_harbour_orientation = TOP_TO_BOTTOM;
	}

	CfgParser::Entry *sub = section->findSection("DOCKAPP");
	if (sub) {
		keys.add_numeric<int>("SIDEMIN", _harbour_da_min_s, 64, 0);
		keys.add_numeric<int>("SIDEMAX", _harbour_da_max_s, 64, 0);

		sub->parseKeyValues(keys.begin(), keys.end());
		keys.clear();
	}
}

ActionAccessMask
Config::getActionAccessMask(const std::string &name)
{
	return Util::StringToGet(action_access_mask_map, name);
}

bool
Config::parseActionAccessMask(const std::string &action_mask, uint &mask)
{
	mask = ACTION_ACCESS_NO;

	std::vector<std::string> tok;
	if (Util::splitString(action_mask, tok, " \t")) {
		std::vector<std::string>::iterator it = tok.begin();
		for (; it != tok.end(); ++it) {
			mask |= getActionAccessMask(*it);
		}
	}

	return true;
}

bool
Config::parseMoveResizeAction(const std::string &action_string, Action &action)
{
	std::vector<std::string> tok;

	// Chop the string up separating the actions.
	if (! Util::splitString(action_string, tok, " \t", 2)) {
		return false;
	}
	action.setAction(Util::StringToGet(moveresize_map, tok[0]));
	if (action.getAction() == NO_MOVERESIZE_ACTION) {
		USER_WARN("Unknown move/resize action: " << tok[0]);
		return false;
	}

	if (tok.size() == 2) {
		// Got enough tok for a paremeter
		switch (action.getAction()) {
		case MOVE_HORIZONTAL:
		case MOVE_VERTICAL:
		case RESIZE_HORIZONTAL:
		case RESIZE_VERTICAL: {
			char *endptr = nullptr;
			action.setParamI(0,
					 strtol(tok[1].c_str(), &endptr, 10));
			if (endptr && *endptr == '%') {
				action.setParamI(1, UNIT_PERCENT);
			} else {
				action.setParamI(1, UNIT_PIXEL);
			}
			break;
		}
		case MOVE_SNAP:
		default:
			// Do nothing.
			break;
		}
	}
	return true;
}

bool
Config::parseMoveResizeActions(const std::string &action_string,
			       ActionEvent& ae)
{
	std::vector<std::string> tok;
	std::vector<std::string>::iterator it;
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

	if (ActionConfig::parseKey(section->getValue(), ae.mod, ae.sym)) {
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
	action.setAction(Util::StringToGet(inputdialog_map, val));
	return (action.getAction() != INPUT_NO_ACTION);
}

bool
Config::parseInputDialogActions(const std::string &actions, ActionEvent &ae)
{
	std::vector<std::string> tok;
	std::vector<std::string>::iterator it;
	Action action;
	std::string::size_type first, last;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	if (Util::splitString(actions, tok, ";")) {
		for (it = tok.begin(); it != tok.end(); ++it) {
			first = (*it).find_first_not_of(" \t\n");
			if (first == std::string::npos)
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

	if (ActionConfig::parseKey(section->getValue(), ae.mod, ae.sym)) {
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

	std::vector<std::string> tok;
	Util::splitString(mask, tok, " \t");
	std::vector<std::string>::iterator it = tok.begin();
	for (; it != tok.end(); ++it) {
		val = Util::StringToGet(mouse_event_map, *it);
		if (val != MOUSE_EVENT_NO) {
			mask_return |= val;
		}
	}
	return mask_return;
}

unsigned int
Config::getMenuIconLimit(unsigned int value, SizeLimitType limit,
			 const std::string &name) const
{
	unsigned int limit_val = 0;
	std::map<std::string, SizeLimits>::const_iterator it =
		_menu_icon_limits.find(name);
	if (it == _menu_icon_limits.end()) {
		if (name == "DEFAULT") {
			limit_val = 16;
		} else {
			limit_val = getMenuIconLimit(value, limit, "DEFAULT");
		}
	} else {
		limit_val = it->second.get(limit);
	}

	return limit_val ? limit_val : value;
}

bool
Config::parseMenuAction(const std::string &action_string, Action &action)
{
	std::vector<std::string> tok;

	// chop the string up separating the actions
	if (Util::splitString(action_string, tok, " \t", 2)) {
		action.setAction(Util::StringToGet(menu_action_map, tok[0]));
		if (action.getAction() != ACTION_NO) {
			return true;
		}
	}

	return false;
}

bool
Config::parseMenuActions(const std::string &actions, ActionEvent &ae)
{
	std::vector<std::string> tok;
	std::vector<std::string>::iterator it;
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

	if (ActionConfig::parseKey(section->getValue(), ae.mod, ae.sym)) {
		value = section->getSection()->findEntry("ACTIONS");
		if (value) {
			return parseMenuActions(value->getValue(), ae);
		}
	}

	return false;
}

/**
 * Load main configuration file, priority as follows:
 *
 *   1. Load command line specified file or ~/.pekwm/config if none specified
 *   2. Copy configuration files to dirname of file in step 1
 *   3. Load system configuration
 */
bool
Config::tryHardLoadConfig(CfgParser &cfg, std::string &file,
			  std::string &cfg_dir)
{
	bool success = false;

	// If a command line
	if (file.size()) {
		cfg_dir = Util::getDir(file);
	} else {
		cfg_dir = Util::getConfigDir();
		file = cfg_dir + "/config";
	}

	success = cfg.parse(file, CfgParserSource::SOURCE_FILE, true);

	// Copy config files to cfg_dir and try loading file again.
	if (! success) {
		copyConfigFiles(cfg_dir);
		success = cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
	}

	// Failed to load file, fall back to using system config files
	if (! success) {
		file = std::string(SYSCONFDIR "/config");
		success = cfg.parse(file, CfgParserSource::SOURCE_FILE, true);
	}

	return success;
}

/**
 * Populate cfg_dir with config files from system directory.
 */
void
Config::copyConfigFiles(const std::string &cfg_dir)
{
	const char *files[] = {
		"autoproperties", "config", "keys", "menu", "mouse", "start",
		"vars", nullptr
	};

	if (! ensureConfigDirExists(cfg_dir)) {
		return;
	}

	struct stat sb;
	std::string sys_dir(SYSCONFDIR);
	for (int i = 0; files[i] != nullptr; i++) {
		std::string dst_file = cfg_dir + "/" + files[i];
		if (stat(dst_file.c_str(), &sb)) {
			std::string src_file = sys_dir + "/" + files[i];
			std::cout << "COPY " << src_file
				  << " TO " << dst_file << std::endl;
			Util::copyTextFile(src_file, dst_file);
		}
	}

	// ensure themes directory exists
	std::string themes_dir = cfg_dir + std::string("/themes");
	if (stat(themes_dir.c_str(), &sb)) {
		mkdir(themes_dir.c_str(), 0700);
	}
}

/**
 * Ensure that the provided configuration directory exists and is writable.
 */
bool
Config::ensureConfigDirExists(const std::string &cfg_dir)
{
	// Ensure that cfg_dir exists and is a directory
	struct stat sb;
	if (stat(cfg_dir.c_str(), &sb)) {
		if (mkdir(cfg_dir.c_str(), 0700)) {
			USER_WARN("unable to create the config directory "
				  << cfg_dir << ". can not copy config files");
			return false;
		}
		stat(cfg_dir.c_str(), &sb);
	} else if (! S_ISDIR(sb.st_mode)) {
		USER_WARN(cfg_dir << " already exists and is not a directory."
			  << " can not copy config files");
		return false;
	}

	// Ensure that the config directory has write end execute bits set.
	bool cfg_dir_ok = false;
	if (getuid() == sb.st_uid) {
		if ((sb.st_mode&(S_IWUSR|S_IXUSR)) == (S_IWUSR|S_IXUSR)) {
			cfg_dir_ok = true;
		}
	}
	if (! cfg_dir_ok && getgid() == sb.st_gid) {
		if ((sb.st_mode&(S_IWGRP|S_IXGRP)) == (S_IWGRP|S_IXGRP)) {
			cfg_dir_ok = true;
		}
	}

	if (! cfg_dir_ok) {
		if ((sb.st_mode&(S_IWOTH|S_IXOTH)) == (S_IWOTH|S_IXOTH)) {
			cfg_dir_ok = true;
		} else {
			USER_WARN("no write on " << cfg_dir << " directory."
				  << " can not copy config files");
			return false;
		}
	}

	return cfg_dir_ok;
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
	    && ! mouse_cfg.parse(SYSCONFDIR "/mouse",
				 CfgParserSource::SOURCE_FILE, true)) {
		_cfg_files_mouse.clear();
		return false;
	}

	if (mouse_cfg.isDynamicContent()) {
		_cfg_files_mouse.clear();
	} else {
		_cfg_files_mouse = mouse_cfg.getCfgFiles();
	}

	// Make sure old actions get unloaded.
	mouse_action_map::iterator it = _mouse_action_map.begin();
	for (; it != _mouse_action_map.end(); ++it) {
		it->second->clear();
	}

	CfgParser::Entry *section;

	section = mouse_cfg.getEntryRoot()->findSection("FRAMETITLE");
	if (section) {
		parseButtons(section,
			     _mouse_action_map[MOUSE_ACTION_LIST_TITLE_FRAME],
			     nullptr, FRAME_OK);
	}

	section = mouse_cfg.getEntryRoot()->findSection("OTHERTITLE");
	if (section) {
		parseButtons(section,
			     _mouse_action_map[MOUSE_ACTION_LIST_TITLE_OTHER],
			     nullptr, FRAME_OK);
	}

	section = mouse_cfg.getEntryRoot()->findSection("CLIENT");
	if (section) {
		parseButtons(section,
			     _mouse_action_map[MOUSE_ACTION_LIST_CHILD_FRAME],
			     &_client_mouse_action_buttons, CLIENT_OK);
	}

	section = mouse_cfg.getEntryRoot()->findSection("ROOT");
	if (section) {
		parseButtons(section,
			     _mouse_action_map[MOUSE_ACTION_LIST_ROOT],
			     nullptr, ROOTCLICK_OK);
	}

	section = mouse_cfg.getEntryRoot()->findSection("MENU");
	if (section) {
		parseButtons(section,
			     _mouse_action_map[MOUSE_ACTION_LIST_MENU],
			     nullptr, FRAME_OK);
	}

	section = mouse_cfg.getEntryRoot()->findSection("OTHER");
	if (section) {
		parseButtons(section,
			     _mouse_action_map[MOUSE_ACTION_LIST_OTHER],
			     nullptr, FRAME_OK);
	}

	section = mouse_cfg.getEntryRoot()->findSection("SCREENEDGE");
	if (section) {
		CfgParser::Entry::entry_cit edge_it(section->begin());
		for (; edge_it != section->end(); ++edge_it)  {
			uint pos = ActionConfig::getDirection(
					(*edge_it)->getName());

			if (pos != SCREEN_EDGE_NO) {
				parseButtons((*edge_it)->getSection(),
					     getEdgeListFromPosition(pos),
					     nullptr, SCREEN_EDGE_OK);
			}
		}
	}

	section = mouse_cfg.getEntryRoot()->findSection("BORDER");
	if (section) {
		CfgParser::Entry::entry_cit border_it(section->begin());
		for (; border_it != section->end(); ++border_it) {
			uint pos = ActionConfig::getBorderPos(
					(*border_it)->getName());
			if (pos != BORDER_NO_POS) {
				parseButtons((*border_it)->getSection(),
					     getBorderListFromPosition(pos),
					     nullptr, FRAME_BORDER_OK);
			}
		}
	}

	return true;
}

static void
mouseButtonsAdd(std::vector<BoundButton>* buttons, uint button, uint mod)
{
	if (buttons == nullptr) {
		return;
	}

	std::vector<BoundButton>::iterator bit = buttons->begin();
	for (; bit != buttons->end(); ++bit) {
		if (bit->button == button) {
			std::vector<uint>::iterator mit = bit->mods.begin();
			for (; mit != bit->mods.end(); ++mit) {
				if (*mit == mod) {
					return;
				}
			}
			bit->mods.push_back(mod);
			return;
		}
	}
	buttons->push_back(BoundButton(button, mod));
}

/**
 * Parses mouse config section, like FRAME
 */
void
Config::parseButtons(CfgParser::Entry *section,
		     std::vector<ActionEvent>* mouse_list,
		     std::vector<BoundButton>* mouse_buttons,
		     ActionOk action_ok)
{
	if (! section || ! mouse_list) {
		return;
	}

	ActionEvent ae;
	CfgParser::Entry *value;

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		if (! (*it)->getSection()) {
			continue;
		}

		ae.type = Util::StringToGet(mouse_event_map, (*it)->getName());
		if (ae.type == MOUSE_EVENT_NO) {
			continue;
		}

		if (ae.type == MOUSE_EVENT_MOTION) {
			value = (*it)->getSection()->findEntry("THRESHOLD");
			if (value) {
				ae.threshold = std::stoi(value->getValue());
			} else {
				ae.threshold = 0;
			}
		}

		if (ActionConfig::parseActionEvent((*it), ae, action_ok,
						   true)) {
			mouse_list->push_back(ae);
			mouseButtonsAdd(mouse_buttons, ae.sym, ae.mod);
		}
	}
}

// frame border configuration

std::vector<ActionEvent>*
Config::getBorderListFromPosition(uint pos)
{
	std::vector<ActionEvent> *ret = 0;

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

std::vector<ActionEvent>*
Config::getEdgeListFromPosition(uint pos)
{
	std::vector<ActionEvent> *ret = 0;

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

//! @brief Parses a string which contains two opacity values
bool
Config::parseOpacity(const std::string value, uint &focused, uint &unfocused)
{
	std::vector<std::string> tokens;
	switch ((Util::splitString(value, tokens, " ,", 2))) {
	case 2:
		focused = std::atoi(tokens[0].c_str());
		unfocused = std::atoi(tokens[1].c_str());
		break;
	case 1:
		focused = unfocused = std::atoi(tokens[0].c_str());
		break;
	default:
		return false;
	}
	CONV_OPACITY(focused);
	CONV_OPACITY(unfocused);
	return true;
}

int
Config::getHarbourHead(void) const
{
	if (_harbour_head.empty()) {
		return _harbour_head_nr;
	}
	int head = X11::findHeadByName(_harbour_head);
	if (head == -1) {
		// fallback to primary head, which should always be valid
		head = X11::findHeadByName("PRIMARY");
	}
	return head;
}
