//
// Config.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "Config.hh"

#include "Util.hh"

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
using std::vector;
using std::ifstream;
using std::ofstream;

Config::UnsignedListItem Config::_placementlist[] = {
	{"SMART", SMART},
	{"MOUSECENTERED", MOUSE_CENTERED},
	{"MOUSETOPLEFT", MOUSE_TOP_LEFT},
	{"", NO_PLACEMENT}
};

Config::UnsignedListItem Config::_edgelist[] = {
	{"TOPLEFT", TOP_LEFT},
	{"TOPEDGE", TOP_EDGE},
	{"TOPRIGHT", TOP_RIGHT},
	{"RIGHTEDGE", RIGHT_EDGE},
	{"BOTTOMRIGHT", BOTTOM_RIGHT},
	{"BOTTOMEDGE", BOTTOM_EDGE},
	{"BOTTOMLEFT", BOTTOM_LEFT},
	{"LEFTEDGE", LEFT_EDGE},
	{"CENTER", CENTER},
	{"", NO_EDGE}
};

Config::UnsignedListItem Config::_raiselist[] = {
	{"ALWAYSRAISE", ALWAYS_RAISE},
	{"ENDRAISE", END_RAISE},
	{"NEVERRAISE", NEVER_RAISE},
	{"", NO_RAISE}
};

Config::UnsignedListItem Config::_applyonlist[] = {
	{"START", APPLY_ON_START},
	{"NEW", APPLY_ON_NEW},
	{"RELOAD", APPLY_ON_RELOAD},
	{"WORKSPACE", APPLY_ON_WORKSPACE},
	{"TRANSIENT", APPLY_ON_TRANSIENT},
	{"TRANSIENTONLY", APPLY_ON_TRANSIENT_ONLY},
	{"", APPLY_ON_NONE}
};

Config::UnsignedListItem Config::_skiplist[] = {
	{"MENUS", SKIP_MENUS},
	{"FOCUSTOGGLE", SKIP_FOCUS_TOGGLE},
	{"FRAMESNAP", SKIP_FRAME_SNAP},
	{"", SKIP_NONE}
};

Config::UnsignedListItem Config::_layerlist[] = {
	{"DESKTOP", LAYER_DESKTOP},
	{"BELOW", LAYER_BELOW},
	{"NORMAL", LAYER_NORMAL},
	{"ONTOP", LAYER_ONTOP},
	{"HARBOUR", LAYER_DOCK},
	{"ABOVEHARBOUR", LAYER_ABOVE_DOCK},
	{"MENU", LAYER_MENU},
	{"", LAYER_NONE},
};

Config::UnsignedListItem Config::_moveresizeactionlist[] = {
	{"MOVEHORIZONTAL", MOVE_HORIZONTAL},
	{"MOVEVERTICAL", MOVE_VERTICAL},
	{"RESIZEHORIZONTAL", RESIZE_HORIZONTAL},
	{"RESIZEVERTICAL", RESIZE_VERTICAL},
	{"MOVESNAP", MOVE_SNAP},
	{"CANCEL", MOVE_CANCEL},
	{"END", MOVE_END},
	{"", NO_MOVERESIZE_ACTION}
};

#ifdef MENUS
Config::UnsignedListItem Config::_menutypelist[] = {
	{"WINDOW", WINDOWMENU_TYPE},
	{"ROOT", ROOTMENU_TYPE},
	{"GOTO", GOTOMENU_TYPE},
	{"ICON", ICONMENU_TYPE},
	{"ATTACHCLIENT", ATTACH_CLIENT_TYPE},
	{"ATTACHFRAME", ATTACH_FRAME_TYPE},
	{"ATTACHCLIENTINFRAME", ATTACH_CLIENT_IN_FRAME_TYPE},
	{"ATTACHFRAMEINFRAME", ATTACH_FRAME_IN_FRAME_TYPE},
	{"", NO_MENU_TYPE}
};

Config::UnsignedListItem Config::_menuactionlist[] = {
	{"NEXTITEM", MENU_NEXT},
	{"PREVITEM", MENU_PREV},
	{"SELECT", MENU_SELECT},
	{"ENTERSUBMENU", MENU_ENTER_SUBMENU},
	{"LEAVESUBMENU", MENU_LEAVE_SUBMENU},
	{"CLOSE", MENU_CLOSE}
};
#endif // MENUS

#ifdef HARBOUR
Config::UnsignedListItem Config::_harbour_placementlist[] = {
	{"TOP", TOP},
	{"LEFT", LEFT},
	{"RIGHT", RIGHT},
	{"BOTTOM", BOTTOM},
	{"", NO_HARBOUR_PLACEMENT}
};

Config::UnsignedListItem Config::_orientationlist[] = {
	{"TOPTOBOTTOM", TOP_TO_BOTTOM},
	{"LEFTTORIGHT", LEFT_TO_RIGHT},
	{"BOTTOMTOTOP", BOTTOM_TO_TOP},
	{"RIGHTTOLEFT", RIGHT_TO_LEFT},
	{"", NO_ORIENTATION}
};
#endif // HARBOUR

Config::UnsignedListItem Config::_modlist[] = {
	{"SHIFT", ShiftMask},
	{"CTRL", ControlMask},
	{"MOD1", Mod1Mask},
	//	{"MOD2", Mod2Mask}, // Num Lock
	//	{"MOD3", Mod3Mask}, // Num Lock on Solaris
	{"MOD4", Mod4Mask}, // Meta / Win
	//	{"MOD5", Mod5Mask}, // Scroll Lock
	{"", 0},
};

const int FRAME_MASK =
  FRAME_OK|CLIENT_OK|WINDOWMENU_OK|KEYGRABBER_OK|BUTTONCLICK_OK;

Config::ActionListItem Config::_actionlist[] = {
	{"MAXIMIZE", MAXIMIZE, FRAME_MASK},
	{"MAXIMIZEVERTICAL", MAXIMIZE_VERTICAL, FRAME_MASK},
	{"MAXIMIZEHORIZONTAL", MAXIMIZE_HORIZONTAL, FRAME_MASK},
	{"SHADE", SHADE, FRAME_MASK},
	{"ICONIFY", ICONIFY, FRAME_MASK},
	{"STICK", STICK, FRAME_MASK},
	{"CLOSE", CLOSE, FRAME_MASK},
	{"KILL", KILL, FRAME_MASK},
	{"RAISE", RAISE, FRAME_MASK},
	{"ACTIVATEORRAISE", ACTIVATE_OR_RAISE, FRAME_MASK},
	{"LOWER", LOWER, FRAME_MASK},
	{"ALWAYSONTOP", ALWAYS_ON_TOP, FRAME_MASK},
	{"ALWAYSBELOW", ALWAYS_BELOW, FRAME_MASK},
	{"TOGGLEBORDER", TOGGLE_BORDER, FRAME_MASK},
	{"TOGGLETITLEBAR", TOGGLE_TITLEBAR, FRAME_MASK},
	{"TOGGLEDECOR", TOGGLE_DECOR, FRAME_MASK},
	{"NEXTINFRAME", NEXT_IN_FRAME, FRAME_MASK},
	{"PREVINFRAME", PREV_IN_FRAME, FRAME_MASK},
	{"MOVECLIENTNEXT", MOVE_CLIENT_NEXT, FRAME_MASK},
	{"MOVECLIENTPREV", MOVE_CLIENT_PREV, FRAME_MASK},
	{"ACTIVATECLIENT", ACTIVATE_CLIENT, FRAME_OK},
	{"ACTIVATECLIENTNUM", ACTIVATE_CLIENT_NUM, KEYGRABBER_OK},
#ifdef MENUS
	{"SHOWMENU", SHOW_MENU, FRAME_MASK|ROOTCLICK_OK},
	{"HIDEALLMENUS", HIDE_ALL_MENUS, FRAME_MASK|ROOTCLICK_OK},
	{"SUBMENU", SUBMENU, ROOTMENU_OK|WINDOWMENU_OK},
	{"DYNAMIC", DYNAMIC_MENU, ROOTMENU_OK|WINDOWMENU_OK},
#endif // MENUS
	{"RESIZE", RESIZE, BUTTONCLICK_OK|CLIENT_OK|FRAME_OK},
	{"MOVE", MOVE, FRAME_OK|CLIENT_OK},
	{"MOVERESIZE", MOVE_RESIZE, KEYGRABBER_OK},
	{"GROUPINGDRAG", GROUPING_DRAG, FRAME_OK|CLIENT_OK},

	{"MOVETOEDGE", MOVE_TO_EDGE, KEYGRABBER_OK},
	{"NEXTFRAME", NEXT_FRAME, KEYGRABBER_OK|ROOTCLICK_OK},
	{"PREVFRAME", PREV_FRAME, KEYGRABBER_OK|ROOTCLICK_OK},

	{"MARKCLIENT", MARK_CLIENT, FRAME_MASK},
	{"ATTACHMARKED", ATTACH_MARKED, FRAME_MASK},
	{"ATTACHCLIENTINNEXTFRAME", ATTACH_CLIENT_IN_NEXT_FRAME, FRAME_MASK},
	{"ATTACHCLIENTINPREVFRAME", ATTACH_CLIENT_IN_PREV_FRAME, FRAME_MASK},
	{"ATTACHFRAMEINNEXTFRAME", ATTACH_FRAME_IN_NEXT_FRAME, FRAME_MASK},
	{"ATTACHFRAMEINPREVFRAME", ATTACH_FRAME_IN_PREV_FRAME, FRAME_MASK},

	{"DETACH", DETACH, FRAME_MASK},
	{"TOGGLETAG", TOGGLE_TAG, FRAME_MASK},
	{"TOGGLETAGBEHIND", TOGGLE_TAG_BEHIND, FRAME_MASK},
	{"UNTAG", UNTAG, FRAME_MASK},
	
	{"NEXTWORKSPACE", NEXT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"RIGHTWORKSPACE", RIGHT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"PREVWORKSPACE", PREV_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"LEFTWORKSPACE", LEFT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"SENDTOWORKSPACE", SEND_TO_WORKSPACE, KEYGRABBER_OK|WINDOWMENU_OK},
	{"GOTOWORKSPACE", GO_TO_WORKSPACE, KEYGRABBER_OK},

	{"EXEC", EXEC, FRAME_MASK|ROOTMENU_OK|ROOTCLICK_OK},
	{"RELOAD", RELOAD, KEYGRABBER_OK|ROOTMENU_OK},
	{"RESTART", RESTART, KEYGRABBER_OK|ROOTMENU_OK},
	{"RESTARTOTHER", RESTART_OTHER, ROOTMENU_OK|KEYGRABBER_OK},
	{"EXIT", EXIT, KEYGRABBER_OK|ROOTMENU_OK},
	{"", NO_ACTION, 0}
};

Config::Config(Display* dpy) :
_dpy(dpy),
_moveresize_edgesnap(0), _moveresize_framesnap(0),
_moveresize_opaquemove(0), _moveresize_opaqueresize(0),
_moveresize_grabwhenresize(true),
_moveresize_ww_frame(0), _moveresize_ww_mouse(0),
_moveresize_ww_frame_wrap(false), _moveresize_ww_mouse_wrap(false),
_screen_workspaces(4), _screen_doubleclicktime(250),
_screen_focusmask(0), _screen_showframelist(true),
_screen_placement_row(false),
_screen_placement_ltr(true), _screen_placement_ttb(true)
{
	load();
}

Config::~Config()
{
}

//! @fn    void load(void)
//! @brief Loads ~/.pekwm/config or if it fails, SYSCONFDIR/config
void
Config::load(void)
{
	BaseConfig cfg;
	bool success;

	success = cfg.load(string(getenv("HOME") + string("/.pekwm/config")));
	if (!success) {
		copyConfigFiles(); // try copying the files
		success = cfg.load(string(SYSCONFDIR "/config"));
	}

	if (!success)
		return; // Where's that config file?

	string s_value;
	string mouse_file; // temporary filepath for mouseconfig

	BaseConfig::CfgSection *cs;

	// Get other config files dests
	if ((cs = cfg.getSection("FILES"))) {
		if (cs->getValue("KEYS", _files_keys))
			Util::expandFileName(_files_keys);
		if (cs->getValue("MOUSE", mouse_file))
			Util::expandFileName(mouse_file);
		if (cs->getValue("MENU", _files_menu))
			Util::expandFileName(_files_menu);
		if (cs->getValue("START", _files_start))
			Util::expandFileName(_files_start);
		if (cs->getValue("AUTOPROPS", _files_autoprops))
			Util::expandFileName(_files_autoprops);
		if (cs->getValue("THEME", _files_theme))
			Util::expandFileName(_files_theme);
	}

	// Parse moving / resizing options
	if ((cs = cfg.getSection("MOVERESIZE"))) {
		cs->getValue("EDGESNAP", _moveresize_edgesnap);
		cs->getValue("FRAMESNAP", _moveresize_framesnap);
		cs->getValue("OPAQUEMOVE", _moveresize_opaquemove);
		cs->getValue("OPAQUERESIZE", _moveresize_opaqueresize);
		cs->getValue("GRABWHENRESIZE", _moveresize_grabwhenresize);

		cs->getValue("WWFRAME", _moveresize_ww_frame);
		cs->getValue("WWFRAMEWRAP", _moveresize_ww_frame_wrap);
		cs->getValue("WWMOUSE", _moveresize_ww_mouse);
		cs->getValue("WWMOUSEWRAP", _moveresize_ww_mouse_wrap);
	}

	// Screen, important stuff such as number of workspaces
	if ((cs = cfg.getSection("SCREEN"))) {
		cs->getValue("WORKSPACES", _screen_workspaces);
		cs->getValue("DOUBLECLICKTIME", _screen_doubleclicktime);
		cs->getValue("SHOWFRAMELIST", _screen_showframelist);

		BaseConfig::CfgSection *cs_n = cs->getSection("FOCUS");
		if (cs_n) {
			bool add;
			_screen_focusmask = 0;

			if ((cs_n->getValue("NEW", add)) && add)
				_screen_focusmask |= FOCUS_NEW;
			if ((cs_n->getValue("ENTER", add)) && add)
				_screen_focusmask |= FOCUS_ENTER;
			if ((cs_n->getValue("LEAVE", add)) && add)
				_screen_focusmask |= FOCUS_LEAVE;
			if ((cs_n->getValue("CLICK", add)) && add)
				_screen_focusmask |= FOCUS_CLICK;
		} else
			_screen_focusmask = FOCUS_CLICK;

		cs_n = cs->getSection("PLACEMENT");
		if (cs_n) {
			if (cs_n->getValue("MODEL", s_value)) {
				_screen_placementmodels.clear();

				vector<string> models;
				if ((Util::splitString(s_value, models, " \t", 3))) {
					vector<string>::iterator it = models.begin();
					for (; it != models.end(); ++it) {
						_screen_placementmodels.push_back(getPlacementModel(*it));
					}
				}
			}

			BaseConfig::CfgSection *cs_s = cs_n->getSection("SMART");
			if (cs_s) {
				cs_s->getValue("ROW", _screen_placement_row);
				cs_s->getValue("LEFTTORIGHT", _screen_placement_ltr);
				cs_s->getValue("TOPTOBOTTOM", _screen_placement_ttb);
			}
		}

		if (!_screen_placementmodels.size())
			_screen_placementmodels.push_back(MOUSE_CENTERED);
	}

#ifdef HARBOUR
	if ((cs = cfg.getSection("HARBOUR"))) {
		cs->getValue("ONTOP", _harbour_ontop);
		cs->getValue("MAXIMIZEOVER", _harbour_maximize_over);

		if (cs->getValue("PLACEMENT", s_value))
			_harbour_placement = getHarbourPlacement(s_value);
		if (_harbour_placement == NO_HARBOUR_PLACEMENT)
			_harbour_placement = TOP;
		if (cs->getValue("ORIENTATION", s_value))
			_harbour_orientation = getOrientatition(s_value);
		if (_harbour_orientation == NO_ORIENTATION)
			_harbour_orientation = TOP_TO_BOTTOM;
	}
#endif // HARBOUR

	// Load the mouse configuration
	loadMouseConfig(mouse_file);
}

//! @fn    ActionType getAction(const string &name, unsigned int mask)
//! @brief
ActionType
Config::getAction(const string &name, unsigned int mask)
{
	if (!name.size())
		return NO_ACTION;

	for (unsigned int i = 0; _actionlist[i].action != NO_ACTION; ++i) {
		if ((_actionlist[i].mask&mask) && (_actionlist[i] == name)) {
			return _actionlist[i].action;
		}
	}

	return NO_ACTION;
}

//! @fn    PlacementModel getPlacementModel(const string &model)
//! @brief
PlacementModel
Config::getPlacementModel(const string &model)
{
	if (!model.size())
		return NO_PLACEMENT;

	for (unsigned int i = 0; _placementlist[i].value != NO_PLACEMENT; ++i) {
		if (_placementlist[i] == model) {
			return PlacementModel(_placementlist[i].value);
		}
	}

	return NO_PLACEMENT;
}

//! @fn    Edge getEdge(const string &edge)
//! @brief
Edge
Config::getEdge(const string &edge)
{
	if (!edge.size())
		return NO_EDGE;

	for (unsigned int i = 0; _edgelist[i].value != NO_EDGE; ++i) {
		if (_edgelist[i] == edge)
			return Edge(_edgelist[i].value);
	}

	return NO_EDGE;
}

//! @fn    Raise getRaise(const string &raise)
//! @brief
Raise
Config::getRaise(const string &raise)
{
	if (!raise.size())
		return NO_RAISE;

	for (unsigned int i = 0; _raiselist[i].value != NO_RAISE; ++i) {
		if (_raiselist[i] == raise)
			return Raise(_raiselist[i].value);
	}

	return NO_RAISE;
}

//! @fn    ApplyOn getApplyOn(const string &apply)
ApplyOn
Config::getApplyOn(const string &apply)
{
	if (!apply.size())
		return APPLY_ON_NONE;

	for (unsigned int i = 0; _applyonlist[i].value != APPLY_ON_NONE; ++i) {
		if (_applyonlist[i] == apply)
			return ApplyOn(_applyonlist[i].value);
	}

	return APPLY_ON_NONE;
}

//! @fn
Skip
Config::getSkip(const string &skip)
{
	if (!skip.size())
		return SKIP_NONE;

	for (unsigned int i = 0; _skiplist[i].value != SKIP_NONE; ++i) {
		if (_skiplist[i] == skip)
			return Skip(_skiplist[i].value);
	}

	return SKIP_NONE;
}

//! @fn    Layer getLayer(const string& layer)
//! @brief Converts layer into Layer
Layer
Config::getLayer(const string& layer)
{
	if (!layer.size())
		return LAYER_NORMAL;

	for (unsigned int i = 0; _layerlist[i].value != LAYER_NONE; ++i) {
		if (_layerlist[i] == layer)
			return  Layer(_layerlist[i].value);
	}

	return LAYER_NORMAL;
}

MoveResizeActionType
Config::getMoveResizeAction(const string &action)
{
	if (!action.size())
		return NO_MOVERESIZE_ACTION;

	for (unsigned int i = 0; _moveresizeactionlist[i].value != NO_MOVERESIZE_ACTION; ++i) {
		if (_moveresizeactionlist[i] == action)
			return MoveResizeActionType(_moveresizeactionlist[i].value);
	}

	return NO_MOVERESIZE_ACTION;
}

#ifdef MENUS
MenuType
Config::getMenuType(const std::string &type)
{
	if (!type.size())
		return NO_MENU_TYPE;

	for (unsigned int i = 0; _menutypelist[i].value != NO_MENU_TYPE; ++i) {
		if (_menutypelist[i] == type)
			return MenuType(_menutypelist[i].value);
	}

	return NO_MENU_TYPE;
}

MenuActionType
Config::getMenuAction(const std::string &action)
{
	if (!action.size())
		return NO_MENU_ACTION;

	for (unsigned int i = 0; _menuactionlist[i].value != NO_MENU_ACTION; ++i) {
		if (_menuactionlist[i] == action)
			return MenuActionType(_menuactionlist[i].value);
	}

	return NO_MENU_ACTION;
}
#endif // MENUS

#ifdef HARBOUR
//! @fn    HarbourPlacement getHarbourPlacement(const string &placement)
HarbourPlacement
Config::getHarbourPlacement(const string &placement)
{
	if (!placement.size())
		return NO_HARBOUR_PLACEMENT;

	for (unsigned int i = 0; _harbour_placementlist[i].value != NO_HARBOUR_PLACEMENT; ++i) {
		if (_harbour_placementlist[i] == placement) {
			return HarbourPlacement(_harbour_placementlist[i].value);
		}
	}

	return NO_HARBOUR_PLACEMENT;
}

//! @fn	   Orientation getOrientatition(const string &orientation);
Orientation
Config::getOrientatition(const std::string &orientation)
{
	if (!orientation.size())
		return NO_ORIENTATION;

	for (unsigned int i = 0; _orientationlist[i].value != NO_ORIENTATION; ++i) {
		if (_orientationlist[i] == orientation)
			return Orientation(_orientationlist[i].value);
	}

	return NO_ORIENTATION;
}
#endif // HARBOUR

//! @fn    bool parseKey(const string& key_string, unsigned int& mod, unsigned int& key)
//! @brief
bool
Config::parseKey(const string& key_string,
								 unsigned int& mod, unsigned int& key)
{
	// used for parsing
	static vector<string> tokens;
	static vector<string>::iterator it;

	// chop the string up separating mods and the end key/button
	tokens.clear();
	if (Util::splitString(key_string, tokens, " \t")) {
		// if the last token isn't an key/button, the action isn't valid
		key =
			XKeysymToKeycode(_dpy,
											 XStringToKeysym(tokens[tokens.size() - 1].c_str()));
		if (key != NoSymbol) {
			tokens.pop_back(); // remove the key/button

			// add the modifier
			mod = 0;
			for (it = tokens.begin(); it != tokens.end(); ++it)
				mod |= getMod(*it);

			return true;
		}
	}

	return false;
}

//! @fn    bool parseButton(const string& button_string, unsigned int& mod, unsigned int& button)
//! @brief
bool
Config::parseButton(const string& button_string,
										unsigned int& mod, unsigned int& button)
{
	// used for parsing
	static vector<string> tokens;
	static vector<string>::iterator it;

	// chop the string up separating mods and the end key/button
	tokens.clear();
	if (Util::splitString(button_string, tokens, " \t")) {
		// if the last token isn't an key/button, the action isn't valid
		button = getMouseButton(tokens[tokens.size() - 1]);
		if (button != 0) {
			tokens.pop_back(); // remove the key/button

			// add the modifier
			mod = 0;
			for (it = tokens.begin(); it != tokens.end(); ++it)
				mod |= getMod(*it);

			return true;
		}
	}

	return false;
}

//! @fn    bool parseAction(const string& action_string, Action& action, unsigned int mask)
//! @brief
bool
Config::parseAction(const string& action_string, Action& action,
										unsigned int mask)
{
	static vector<string> tokens;

	// chop the string up separating the actions
	tokens.clear();
	if (Util::splitString(action_string, tokens, " \t", 2)) {
		action.action = getAction(tokens[0], mask);
		if (action.action != NO_ACTION) {
			if (tokens.size() == 2) { // we got enough tokens for a parameter
				switch (action.action) {
				case EXEC:
				case RESTART_OTHER:
#ifdef MENUS
				case DYNAMIC_MENU:
#endif // MENUS
					action.param_s = tokens[1];
					break;
				case SEND_TO_WORKSPACE:
				case GO_TO_WORKSPACE:
				case ACTIVATE_CLIENT_NUM:
					action.param_i = atoi(tokens[1].c_str()) - 1;
					if (action.param_i < 0)
						action.param_i = 0;
					break;
				case GROUPING_DRAG:
					action.param_i = Util::isTrue(tokens[1]);
					break;
				case MOVE_TO_EDGE:
					action.param_i = getEdge(tokens[1]);
					break;
				case NEXT_FRAME:
				case PREV_FRAME:
					action.param_i = getRaise(tokens[1]);
					break;
#ifdef MENUS
				case SHOW_MENU:
					action.param_i = getMenuType(tokens[1]);
					break;
#endif // MENUS

				default:
					// Do nothing
					break;
				}
			}

			return true;
		}
	}

	return false;
}


//! @fn    bool parseActions(const string& action_string, ActionEvent& ae)
//! @brief 
bool
Config::parseActions(const string& action_string, ActionEvent& ae,
										 unsigned int mask)
{
	static vector<string> tokens;
	static vector<string>::iterator it;
	static Action action;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	tokens.clear();
	if (Util::splitString(action_string, tokens, ";")) {
		for (it = tokens.begin(); it != tokens.end(); ++it) {
			if (parseAction(*it, action, mask))
				ae.action_list.push_back(action);
		}

		return true;
	}

	return false;
}

//! @fn
//! @brief
bool
Config::parseActionEvent(BaseConfig::CfgSection* sect, ActionEvent& ae,
												 unsigned int mask, bool button)
{
	static string value;

	if (!sect->getValue("NAME", value)) // includes mod+key
		return false;

	bool ok;
	if (button)
		ok = parseButton(value, ae.mod, ae.sym);
	else
		ok = parseKey(value, ae.mod, ae.sym);

	if (ok && sect->getValue("ACTIONS", value))
		return parseActions(value, ae, mask);
	return false;
}

//! @fn
//! @brief
bool
Config::parseMoveResizeAction(const std::string& action_string, Action& action)
{
	static vector<string> tokens;

	// chop the string up separating the actions
	tokens.clear();
	if (Util::splitString(action_string, tokens, " \t", 2)) {
		action.action = getMoveResizeAction(tokens[0]);
		if (action.action != NO_MOVERESIZE_ACTION) {
			if (tokens.size() == 2) { // we got enough tokens for a paremeter
				switch (action.action) {
				case MOVE_HORIZONTAL:
				case MOVE_VERTICAL:
				case RESIZE_HORIZONTAL:
				case RESIZE_VERTICAL:
				case MOVE_SNAP:
					action.param_i = atoi(tokens[1].c_str());
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

//! @fn
//! @brief
bool
Config::parseMoveResizeActions(const std::string& action_string, ActionEvent& ae)
{
	static vector<string> tokens;
	static vector<string>::iterator it;
	static Action action;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	tokens.clear();
	if (Util::splitString(action_string, tokens, ";")) {
		for (it = tokens.begin(); it != tokens.end(); ++it) {
			if (parseMoveResizeAction(*it, action))
				ae.action_list.push_back(action);
		}

		return true;
	}

	return false;
}

//! @fn
//! @brief
bool
Config::parseMoveResizeEvent(BaseConfig::CfgSection* sect, ActionEvent& ae)
{
	static string value;

	if (!sect->getValue("NAME", value)) // include mod+key
			return false;

	if (parseKey(value, ae.mod, ae.sym)) {
		if (sect->getValue("ACTIONS", value))
			return parseMoveResizeActions(value, ae);
	}

	return false;
}

#ifdef MENUS
bool
Config::parseMenuAction(const std::string& action_string, Action& action)
{
	static vector<string> tokens;

	// chop the string up separating the actions
	tokens.clear();
	if (Util::splitString(action_string, tokens, " \t", 2)) {
		action.action = getMenuAction(tokens[0]);
		if (action.action != NO_MENU_ACTION)
			return true;
	}

	return false;
}

bool
Config::parseMenuActions(const std::string& actions, ActionEvent& ae)
{ 
	static vector<string> tokens;
	static vector<string>::iterator it;
	static Action action;

	// reset the action event
	ae.action_list.clear();

	// chop the string up separating the actions
	tokens.clear();
	if (Util::splitString(actions, tokens, ";")) {
		for (it = tokens.begin(); it != tokens.end(); ++it) {
			if (parseMenuAction(*it, action))
				ae.action_list.push_back(action);
		}

		return true;
	}

	return false;
}

bool
Config::parseMenuEvent(BaseConfig::CfgSection* sect, ActionEvent& ae)
{
	static string value;

	if (!sect->getValue("NAME", value)) // include mod+key
			return false;

	if (parseKey(value, ae.mod, ae.sym)) {
		if (sect->getValue("ACTIONS", value))
			return parseMenuActions(value, ae);
	}

	return false;
}
#endif // MENUS

//! @fn    unsigned int getMod(const string &mod)
//! @brief Converts the string mod into a usefull X Modifier Mask
//! @param mod String to convert into an X Modifier Mask
//! @return X Modifier Mask if found, else 0
unsigned int
Config::getMod(const string &mod)
{
	if (!mod.size())
		return 0;

	for (unsigned int i = 0; _modlist[i].value; ++i) {
		if (_modlist[i] ==  mod)
			return _modlist[i].value;
	}

	return 0;
}

//! @fn    unsigned int getMouseButton(const string &button)
//! @brief
unsigned int
Config::getMouseButton(const string &button)
{
	if (!button.size())
		return 0;

	unsigned int btn = unsigned(atoi(button.c_str()));
	if (btn > NUM_BUTTONS)
		btn = 0;

	return btn;
}

//! @fn    void copyConfigFiles(void)
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

	bool cp_config, cp_keys, cp_mouse, cp_menu, cp_autoprops, cp_start;
	cp_config = cp_keys = cp_mouse = cp_menu = cp_autoprops = cp_start = false;

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

	} else { // we didn't have a ~/.pekwm directory allready, lets create one
		if (mkdir(cfg_dir.c_str(), 0700)) {
			cerr << "Can't create " << cfg_dir << " directory!" << endl;
			cerr << "Can't copy config files !" << endl;
			return;
		}

		cp_config = cp_keys = cp_mouse = cp_menu = cp_autoprops = cp_start = true;
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
}

//! @fn    void copyTextFile(const string &grom, const string &to)
//! @brief Copies a single text file.
void
Config::copyTextFile(const string &from, const string &to)
{
	if (!from.length() || !to.length())
		return;

	ifstream stream_from(from.c_str());
	if (! stream_from.good())
		return;

	ofstream stream_to(to.c_str());
	if (! stream_to.good())
		return;

	stream_to << stream_from.rdbuf();
}

//! @fn    void loadMouseConfig(const string &file)
//! @brief Parses the CfgSection for mouse actions.
void
Config::loadMouseConfig(const string &file)
{
	if (!file.size())
		return;

	BaseConfig mouse_cfg;

	bool success = mouse_cfg.load(file);
	if (!success)
		success = mouse_cfg.load(string(SYSCONFDIR "/mouse"));

	if (!success)
		return;

	// make sure old actions get unloaded
	_mouse_client.clear();
	_mouse_frame.clear();
	_mouse_root.clear();

	BaseConfig::CfgSection *section;

	if ((section = mouse_cfg.getSection("FRAME")))
		parseButtons(section, &_mouse_frame, FRAME_OK);
	if ((section = mouse_cfg.getSection("CLIENT")))
		parseButtons(section, &_mouse_client, CLIENT_OK);
	if ((section = mouse_cfg.getSection("ROOT")))
		parseButtons(section, &_mouse_root, ROOTCLICK_OK);
}

//! @fn    void parseButtons(BaseConfig::CfgSection *cs, list<Action> *mouse_list, ActionOK action_ok)
//! @brief Parses mouse config section, like FRAME
void
Config::parseButtons(BaseConfig::CfgSection* cs,
										 list<ActionEvent>* mouse_list, ActionOk action_ok)
{
	if (!cs || !mouse_list)
		return;

	string name, value, line;
	BaseConfig::CfgSection *sub;

	ActionEvent ae;
	while ((sub = cs->getNextSection())) {
		if (*sub == "BUTTONPRESS") {
			ae.type = BUTTON_PRESS;
		} else if (*sub == "BUTTONRELEASE") {
			ae.type = BUTTON_RELEASE;
		} else if (*sub == "DOUBLECLICK") {
			ae.type = BUTTON_DOUBLE;
		} else if (*sub == "MOTION") {
			ae.type = BUTTON_MOTION;
			if (sub->getValue("THRESHOLD", value))
				ae.threshold = atoi(value.c_str());
			else
				ae.threshold = 0;
		} else {
			continue; // not a valid type
		}

		if (parseActionEvent(sub, ae, action_ok, true))
			mouse_list->push_back(ae);
	}
}
