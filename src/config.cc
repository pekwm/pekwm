//
// config.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
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

#include "config.hh"
#include "util.hh"

#include <iostream>
#include <fstream>

#include <cstdlib>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::vector;
using std::ifstream;
using std::ofstream;

Config::UnsignedListItem Config::m_mousebuttonlist[] = {
	{"BUTTON1", Button1},
	{"BUTTON2", Button2},
	{"BUTTON3", Button3},
	{"BUTTON4", Button4},
	{"BUTTON5", Button5},
	{"", 0}
};

Config::UnsignedListItem Config::m_focuslist[] = {
	{"FOLLOW", FOCUS_FOLLOW},
	{"SLOPPY", FOCUS_SLOPPY},
	{"CLICK", FOCUS_CLICK},
	{"", NO_FOCUS}
};

Config::UnsignedListItem Config::m_placementlist[] = {
	{"SMART", SMART},
	{"MOUSECENTERED", MOUSE_CENTERED},
	{"MOUSETOPLEFT", MOUSE_TOP_LEFT},
	{"", NO_PLACEMENT}

};

Config::UnsignedListItem Config::m_cornerlist[] = {
	{"TOPLEFT", TOP_LEFT},
	{"TOPRIGHT", TOP_RIGHT},
	{"BOTTOMLEFT", BOTTOM_LEFT},
	{"BOTTOMRIGHT", BOTTOM_RIGHT},
	{"", NO_CORNER}
};

#ifdef HARBOUR
Config::UnsignedListItem Config::m_harbour_placementlist[] = {
	{"TOP", TOP},
	{"LEFT", LEFT},
	{"RIGHT", RIGHT},
	{"BOTTOM", BOTTOM},
	{"", NO_HARBOUR_PLACEMENT}
};

Config::UnsignedListItem Config::m_orientationlist[] = {
	{"TOPTOBOTTOM", TOP_TO_BOTTOM},
	{"LEFTTORIGHT", LEFT_TO_RIGHT},
	{"BOTTOMTOTOP", BOTTOM_TO_TOP},
	{"RIGHTTOLEFT", RIGHT_TO_LEFT},
	{"", NO_ORIENTATION}
};
#endif // HARBOUR

Config::UnsignedListItem Config::m_modlist[] = {
	{"SHIFT", ShiftMask},
	{"CTRL", ControlMask},
	{"ALT", Mod1Mask},
	//	{"MOD2", Mod2Mask}, // Num Lock
	//	{"MOD3", Mod3Mask}, // Num Lock on Solaris
	{"MOD4", Mod4Mask}, // Meta / Win
	//	{"MOD5", Mod5Mask}, // Scroll Lock
	{"", 0},
};

const int FRAME_MASK =
  FRAME_OK|CLIENT_OK|WINDOWMENU_OK|KEYGRABBER_OK|BUTTONCLICK_OK;

Config::ActionListItem Config::m_actionlist[] = {
	{"MAXIMIZE", MAXIMIZE, FRAME_MASK},
	{"MAXIMIZEVERTICAL", MAXIMIZE_VERTICAL, FRAME_MASK},
	{"MAXIMIZEHORIZONTAL", MAXIMIZE_HORIZONTAL, FRAME_MASK},
	{"SHADE", SHADE, FRAME_MASK},
	{"ICONIFY", ICONIFY, FRAME_MASK},
	{"ICONIFYGROUP", ICONIFY_GROUP, FRAME_MASK},
	{"STICK", STICK, FRAME_MASK},
	{"CLOSE", CLOSE, FRAME_MASK},
	{"KILL", KILL, FRAME_MASK},
	{"RAISE", RAISE, FRAME_MASK},
	{"LOWER", LOWER, FRAME_MASK},
	{"ALWAYSONTOP", ALWAYS_ON_TOP, FRAME_MASK},
	{"ALWAYSBELOW", ALWAYS_BELOW, FRAME_MASK},
	{"TOGGLEBORDER", TOGGLE_BORDER, KEYGRABBER_OK},
	{"TOGGLETITLEBAR", TOGGLE_TITLEBAR, KEYGRABBER_OK},
	{"TOGGLEDECOR", TOGGLE_DECOR, KEYGRABBER_OK},
	{"NEXTINFRAME", NEXT_IN_FRAME, FRAME_MASK},
	{"PREVINFRAME", PREV_IN_FRAME, FRAME_MASK},
	{"MOVECLIENTNEXT", MOVE_CLIENT_NEXT, FRAME_MASK},
	{"MOVECLIENTPREV", MOVE_CLIENT_PREV, FRAME_MASK},
	{"ACTIVATECLIENT", ACTIVATE_CLIENT, FRAME_OK},
	{"ACTIVATECLIENTNUM", ACTIVATE_CLIENT_NUM, KEYGRABBER_OK},
#ifdef MENUS
	{"SHOWWINDOWMENU", SHOW_WINDOWMENU, BUTTONCLICK_OK|FRAME_OK|CLIENT_OK},
	{"SHOWROOTMENU", SHOW_ROOTMENU, FRAME_MASK|ROOTCLICK_OK},
	{"SHOWICONMENU", SHOW_ICONMENU, FRAME_MASK|ROOTCLICK_OK},
	{"HIDEALLMENUS", HIDE_ALL_MENUS, FRAME_MASK|ROOTCLICK_OK},
	{"SUBMENU", SUBMENU, ROOTMENU_OK},
#endif // MENUS
	{"RESIZE", RESIZE, BUTTONCLICK_OK|CLIENT_OK|FRAME_OK},
	{"MOVE", MOVE, FRAME_OK|CLIENT_OK},
	{"GROUPINGDRAG", GROUPING_DRAG, FRAME_OK|CLIENT_OK},

	{"NUDGEHORIZONTAL", NUDGE_HORIZONTAL, KEYGRABBER_OK},
	{"NUDGEVERTICAL", NUDGE_VERTICAL, KEYGRABBER_OK},
	{"RESIZEHORIZONTAL", RESIZE_HORIZONTAL, KEYGRABBER_OK},
	{"RESIZEVERTICAL", RESIZE_VERTICAL, KEYGRABBER_OK},
	{"MOVETOCORNER", MOVE_TO_CORNER, KEYGRABBER_OK},
	{"NEXTFRAME", NEXT_FRAME, KEYGRABBER_OK|ROOTCLICK_OK},

	{"NEXTWORKSPACE", NEXT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"RIGHTWORKSPACE", RIGHT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"PREVWORKSPACE", PREV_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"LEFTWORKSPACE", LEFT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK},
	{"SENDTOWORKSPACE", SEND_TO_WORKSPACE, KEYGRABBER_OK},
	{"GOTOWORKSPACE", GO_TO_WORKSPACE, KEYGRABBER_OK},

	{"EXEC", EXEC, KEYGRABBER_OK|ROOTMENU_OK|ROOTCLICK_OK},
	{"RELOAD", RELOAD, KEYGRABBER_OK|ROOTMENU_OK},
	{"RESTART", RESTART, KEYGRABBER_OK|ROOTMENU_OK},
	{"RESTARTOTHER", RESTART_OTHER, ROOTMENU_OK|KEYGRABBER_OK},
	{"EXIT", EXIT, KEYGRABBER_OK|ROOTMENU_OK},
	{"", NO_ACTION, 0}
};

Config::Config() :
m_moveresize_edgesnap(0), m_moveresize_framesnap(0),
m_moveresize_opaquemove(0), m_moveresize_opaqueresize(0),
m_moveresize_grabwhenresize(true), m_moveresize_workspacewarp(0),
m_moveresize_wrapworkspacewarp(false),
m_screen_workspaces(4), m_screen_doubleclicktime(250),
m_screen_reportonlyframes(false), m_screen_focusmodel(FOCUS_SLOPPY),
m_screen_placementmodel(MOUSE_CENTERED),
m_screen_fallbackplacementmodel(NO_PLACEMENT)
{
	load();
}

Config::~Config()
{
}

//! @fn    void load(void)
//! @brief Loads ~/.pekwm/config or if it fails, DATADIR/config
void
Config::load(void)
{
	BaseConfig cfg;
	bool success;

	success = cfg.load(string(getenv("HOME") + string("/.pekwm/config")));
	if (!success) {
		copyConfigFiles(); // try copying the files
		success = cfg.load(string(DATADIR "/config"));
	}

	if (!success)
		return; // Where's that config file?

	string s_value;
	string mouse_file; // temporary filepath for mouseconfig

	BaseConfig::CfgSection *cs;

	// Get other config files dests
	if ((cs = cfg.getSection("FILES"))) {
		if (cs->getValue("KEYS", m_files_keys))
			Util::expandFileName(m_files_keys);
		else
			m_files_keys = DATADIR "/keys";

		if (cs->getValue("MOUSE", mouse_file))
			Util::expandFileName(mouse_file);
		else
			mouse_file = DATADIR "/mouse";

		if (cs->getValue("MENU", m_files_menu))
			Util::expandFileName(m_files_menu);
		else
			m_files_menu = DATADIR "/menu";

		if (cs->getValue("START", m_files_start))
			Util::expandFileName(m_files_start);
		else
			m_files_start = DATADIR "/start";

		if (cs->getValue("AUTOPROPS", m_files_autoprops))
			Util::expandFileName(m_files_autoprops);
		else
			m_files_autoprops = DATADIR "/autoprops";

		if (cs->getValue("THEME", m_files_theme))
			Util::expandFileName(m_files_theme);
		else
			m_files_theme = DATADIR "/themes/default";
	}

	// Parse moving / resizing options
	if ((cs = cfg.getSection("MOVERESIZE"))) {
		cs->getValue("EDGESNAP", m_moveresize_edgesnap);
		cs->getValue("FRAMESNAP", m_moveresize_framesnap);
		cs->getValue("OPAQUEMOVE", m_moveresize_opaquemove);
		cs->getValue("OPAQUERESIZE", m_moveresize_opaqueresize);
		cs->getValue("GRABWHENRESIZE", m_moveresize_grabwhenresize);
		cs->getValue("WORKSPACEWARP", m_moveresize_workspacewarp);
		cs->getValue("WRAPWORKSPACEWARP", m_moveresize_wrapworkspacewarp);
	}

	// Screen, important stuff such as number of workspaces
	if ((cs = cfg.getSection("SCREEN"))) {
		cs->getValue("WORKSPACES", m_screen_workspaces);
		cs->getValue("DOUBLECLICKTIME", m_screen_doubleclicktime);
		cs->getValue("REPORTONLYFRAMES", m_screen_reportonlyframes);

		if (cs->getValue("FOCUSMODEL", s_value)) {
			m_screen_focusmodel = getFocusModel(s_value);
			if (m_screen_focusmodel == NO_FOCUS) {
				m_screen_focusmodel = FOCUS_SLOPPY;
			}
		} else {
			m_screen_focusmodel = FOCUS_SLOPPY;
		}


		if (cs->getValue("PLACEMENTMODEL", s_value)) {
			vector<string> models;
			if ((Util::splitString(s_value, models, " \t", 2)) == 2) {
				m_screen_placementmodel = getPlacementModel(models[0]);
				m_screen_fallbackplacementmodel = getPlacementModel(models[1]);
			} else {
				m_screen_placementmodel = getPlacementModel(s_value);
			}
		}

		if (m_screen_placementmodel == NO_PLACEMENT)
			m_screen_placementmodel = MOUSE_CENTERED;
	}

#ifdef HARBOUR
	if ((cs = cfg.getSection("HARBOUR"))) {
		cs->getValue("ONTOP", m_harbour_ontop);

		if (cs->getValue("PLACEMENT", s_value))
			m_harbour_placement = getHarbourPlacement(s_value);
		if (m_harbour_placement == NO_HARBOUR_PLACEMENT)
			m_harbour_placement = TOP;
		if (cs->getValue("ORIENTATION", s_value))
			m_harbour_orientation = getOrientatition(s_value);
		if (m_harbour_orientation == NO_ORIENTATION)
			m_harbour_orientation = TOP_TO_BOTTOM;
	}
#endif // HARBOUR

	// Load the mouse configuration
	loadMouseConfig(mouse_file);

}

//! @fn    Actions getAction(const string &name, unsigned int mask)
//! @brief
Actions
Config::getAction(const string &name, unsigned int mask)
{
	if (!name.size())
		return NO_ACTION;

	for (unsigned int i = 0; m_actionlist[i].action != NO_ACTION; ++i) {
		if ((m_actionlist[i].mask&mask) && (m_actionlist[i] == name)) {
			return m_actionlist[i].action;
		}
	}

	return NO_ACTION;
}

//! @fn    FocusModel getFocusModel(const string &model)
//! @brief
FocusModel
Config::getFocusModel(const string &model)
{
	if (!model.size())
		return NO_FOCUS;

	for (unsigned int i = 0; m_focuslist[i].value != NO_FOCUS; ++i) {
		if (m_focuslist[i] == model) {
			return (FocusModel) m_focuslist[i].value;
		}
	}

	return NO_FOCUS;
}

//! @fn    PlacementModel getPlacementModel(const string &model)
//! @brief
PlacementModel
Config::getPlacementModel(const string &model)
{
	if (!model.size())
		return NO_PLACEMENT;

	for (unsigned int i = 0; m_placementlist[i].value != NO_PLACEMENT; ++i) {
		if (m_placementlist[i] == model) {
			return (PlacementModel) m_placementlist[i].value;
		}
	}

	return NO_PLACEMENT;
}

//! @fn    Corner getCorner(const string &corner)
//! @brief
Corner
Config::getCorner(const string &corner)
{
	if (!corner.size())
		return NO_CORNER;

	for (unsigned int i = 0; m_cornerlist[i].value != NO_CORNER; ++i) {
		if (m_cornerlist[i] == corner) {
			return (Corner) m_cornerlist[i].value;
		}
	}

	return NO_CORNER;
}

#ifdef HARBOUR
//! @fn    HarbourPlacement getHarbourPlacement(const string &placement)
HarbourPlacement
Config::getHarbourPlacement(const string &placement)
{
	if (!placement.size())
		return NO_HARBOUR_PLACEMENT;

	for (unsigned int i = 0; m_harbour_placementlist[i].value != NO_HARBOUR_PLACEMENT; ++i) {
		if (m_harbour_placementlist[i] == placement) {
			return (HarbourPlacement) m_harbour_placementlist[i].value;
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

	for (unsigned int i = 0; m_orientationlist[i].value != NO_ORIENTATION; ++i) {
		if (m_orientationlist[i] == orientation)
			return (Orientation) m_orientationlist[i].value;
	}

	return NO_ORIENTATION;
}
#endif // HARBOUR

//! @fn    unsigned int getMod(const string &mod)
//! @brief Converts the string mod into a usefull X Modifier Mask
//! @param mod String to convert into an X Modifier Mask
//! @return X Modifier Mask if found, else 0
unsigned int
Config::getMod(const string &mod)
{
	if (!mod.size())
		return 0;

	for (unsigned int i = 0; m_modlist[i].value; ++i) {
		if (m_modlist[i] ==  mod) {
			return m_modlist[i].value;
		}
	}

	return 0;
}

//! @fn    unsigned int getMouseButton(const string &button)
//! @brief
unsigned int
Config::getMouseButton(const string &button)
{
	if (! button.size())
		return 0;

	for (unsigned int i = 0; m_mousebuttonlist[i].value; ++i) {
		if (m_mousebuttonlist[i] ==  button) {
			return m_mousebuttonlist[i].value;
		}
	}

	return 0;
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
	string autoprops_file = cfg_dir + string("/autoprops");
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
		copyTextFile(DATADIR "/config", cfg_file);
	if (cp_keys)
		copyTextFile(DATADIR "/keys", keys_file);
	if (cp_mouse)
		copyTextFile(DATADIR "/mouse", mouse_file);
	if (cp_menu)
		copyTextFile(DATADIR "/menu", menu_file);
	if (cp_autoprops)
		copyTextFile(DATADIR "/autoprops", autoprops_file);
	if (cp_start)
		copyTextFile(DATADIR "/start", start_file);
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
		success = mouse_cfg.load(string(DATADIR "/mouse"));

	if (!success)
		return;

	// make sure old actions get unloaded
	m_mouse_client.clear();
	m_mouse_frame.clear();
	m_mouse_root.clear();

	BaseConfig::CfgSection *section;

	if ((section = mouse_cfg.getSection("FRAME")))
		parseButtons(section, &m_mouse_frame, FRAME_OK);
	if ((section = mouse_cfg.getSection("CLIENT")))
		parseButtons(section, &m_mouse_client, CLIENT_OK);
	if ((section = mouse_cfg.getSection("ROOT")))
		parseButtons(section, &m_mouse_root, ROOTCLICK_OK);
}

//! @fn    void parseButtons(BaseConfig::CfgSection *cs, list<MouseButtonAction> *mouse_list, ActionOK action_ok)
//! @brief Parses mouse config section, like FRAME
void
Config::parseButtons(BaseConfig::CfgSection *cs,
										 list<MouseButtonAction> *mouse_list,
										 ActionOk action_ok)
{
	if (!cs || !mouse_list)
		return;

	string name, value;

	string line;
	vector<string> values, split_line;
	vector<string>::iterator it;

	BaseConfig::CfgSection *action;

	while ((action = cs->getNextSection())) {
		MouseButtonAction click;

		if (*action == "CLICK") {
			click.type = BUTTON_SINGLE;
		} else if (*action == "DOUBLECLICK") {
			click.type = BUTTON_DOUBLE;
		} else if (*action == "MOTION") {
			click.type = BUTTON_MOTION;
		} else {
			continue; // not a valid type
		}

		// Get the button for the action
		if (!action->getValue("BUTTON", value))
			continue; // we _need_ a button

		click.button = getMouseButton(value);
		if (click.button < Button1)
			continue; // we _need_ a button


		// Get action
		if (!action->getValue("ACTION", value))
			continue; // we need action
		click.action = getAction(value, action_ok);
		if (click.action == NO_ACTION)
			continue; // we need action

		if (click.action == EXEC) {
			if (!action->getValue("PARAM", value))
				continue; // we should have a param for execs
			click.s_param = value;
		}

		// Get modifier
		if (action->getValue("MOD", value)) {
			values.clear();

			click.mod = 0;
			if (Util::splitString(value, values, " \t")) {
				for (it = values.begin(); it != values.end(); ++it) {
					click.mod |= getMod(*it);
				}
			}
		}

		mouse_list->push_back(click);
	}
}
