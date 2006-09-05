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
using std::vector;
using std::ifstream;
using std::ofstream;

Config::UnsignedListItem Config::m_mousebuttonlist[] = {
	{"Button1", Button1},
	{"Button2", Button2},
	{"Button3", Button3},
	{"Button4", Button4},
	{"Button5", Button5},
	{"", 0}
};

Config::UnsignedListItem Config::m_focuslist[] = {
	{"Follow", FOCUS_FOLLOW},
	{"Sloppy", FOCUS_SLOPPY},
	{"Click", FOCUS_CLICK},
	{"", NO_FOCUS}
};

Config::UnsignedListItem Config::m_placementlist[] = {
	{"Smart", SMART},
	{"MouseCentered", MOUSE_CENTERED},
	{"MouseTopLeft", MOUSE_TOP_LEFT},
	{"", NO_PLACEMENT}
};

Config::ModListItem Config::m_modlist[] = {
	{"Shift", ShiftMask},
	{"Ctrl", ControlMask},
	{"Alt", Mod1Mask},
	//	{"Mod2", Mod2Mask}, // Num Lock
	//	{"Mod3", Mod3Mask}, // Num Lock on Solaris
	{"Mod4", Mod4Mask}, // Meta / Win
	//	{"Mod5", Mod5Mask}, // Scroll Lock
	{"None", 0},
	{"", -1}
};



const int FRAME_MASK =
  FRAMECLICK_OK|CLIENTCLICK_OK|WINDOWMENU_OK|KEYGRABBER_OK|BUTTONCLICK_OK;

// I know, this looks rather horrid as the comments are longer than
// 80 chars, but it's for auto-generation of documentation.

Config::ActionListItem Config::m_actionlist[] = {
	{"Maximize", MAXIMIZE, FRAME_MASK}, // Maximize the window
	{"MaximizeVertical", MAXIMIZE_VERTICAL, FRAME_MASK}, // Maximize the window vertically
	{"MaximizeHorizontal", MAXIMIZE_HORIZONTAL, FRAME_MASK}, // Maximize the window horizontally
	{"Shade", SHADE, FRAME_MASK}, // Shades the window (only view the titlebar)
	{"Iconify", ICONIFY, FRAME_MASK}, // Iconifies the window (hides it)
	{"IconifyGroup", ICONIFY_GROUP, FRAME_MASK}, // Iconifies all windows in the group, making the frame hidden too
	{"Stick", STICK, FRAME_MASK},  // Makes window 'Sticky' (present on all workspaces)
	{"Raise", RAISE, FRAME_MASK}, // Raises window (puts window 'on top')
	{"Lower", LOWER, FRAME_MASK}, // Lowers window (puts window 'on bottom'
	{"AlwaysOnTop", ALWAYS_ON_TOP, FRAME_MASK}, // Makes window always stay above others
	{"AlwaysBelow", ALWAYS_BELOW, FRAME_MASK}, // Makes window always stay below others
	{"Close", CLOSE, FRAME_MASK}, // Closes the window
	{"NextInFrame", NEXT_IN_FRAME, FRAME_MASK}, // Activates next window in group (frame)
	{"PrevInFrame", PREV_IN_FRAME, FRAME_MASK}, // Activates previous window in group (frame)
	{"MoveClientNext", MOVE_CLIENT_NEXT, FRAME_MASK},
	{"MoveClientPrev", MOVE_CLIENT_PREV, FRAME_MASK},
	{"ActivateClient", ACTIVATE_CLIENT, FRAMECLICK_OK}, // Activates the client under the mouse on the titlebar
	{"ActivateClientNum", ACTIVATE_CLIENT_NUM, KEYGRABBER_OK}, // Activate the client in the frame numbered X
#ifdef MENUS
	{"ShowWindowMenu", SHOW_WINDOWMENU,
	 BUTTONCLICK_OK|FRAMECLICK_OK|CLIENTCLICK_OK}, // Show the window menu
	{"ShowRootMenu", SHOW_ROOTMENU, FRAME_MASK|ROOTCLICK_OK}, // Show the root menu
	{"ShowIconMenu", SHOW_ICONMENU, FRAME_MASK|ROOTCLICK_OK}, // Show the icon menu
	{"HideAllMenus", HIDE_ALL_MENUS, FRAME_MASK|ROOTCLICK_OK}, // Hide all menus
#endif // MENUS
	{"Resize", RESIZE, BUTTONCLICK_OK|CLIENTMOTION_OK}, // Pops the mouse to resize move on the current frame

	{"Move", MOVE, FRAMEMOTION_OK|CLIENTMOTION_OK}, // Move the client
	{"GroupingDrag", GROUPING_DRAG, FRAMEMOTION_OK|CLIENTMOTION_OK},

	{"NudgeHorizontal", NUDGE_HORIZONTAL, KEYGRABBER_OK}, // Moves window X pixels horizontally
	{"NudgeVertical", NUDGE_VERTICAL, KEYGRABBER_OK}, // Moves window Y pixels vertically
	{"ResizeHorizontal", RESIZE_HORIZONTAL, KEYGRABBER_OK}, // Resize the frame width X num pixels
	{"ResizeVertical", RESIZE_VERTICAL, KEYGRABBER_OK}, // Resize the frame height X num pixels
	{"NextFrame", NEXT_FRAME, KEYGRABBER_OK|ROOTCLICK_OK},

	{"NextWorkspace", NEXT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK}, // Activate the next (right) workspace (wraps)
	{"RightWorkspace", RIGHT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK}, // Activate the next (right) workspace (no wrap)
	{"PrevWorkspace", PREV_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK}, // Activate the previous (left) workspace (wrap)
	{"LeftWorkspace", LEFT_WORKSPACE, KEYGRABBER_OK|ROOTCLICK_OK}, // Activate the previous (left) workspace (no wrap)

	{"SendToWorkspace", SEND_TO_WORKSPACE, KEYGRABBER_OK}, // Send frame to workspace X
	{"GoToWorkspace", GO_TO_WORKSPACE, KEYGRABBER_OK}, // Go to workspace X
	{"Exec", EXEC, KEYGRABBER_OK|ROOTMENU_OK|ROOTCLICK_OK}, // Executes a command
	{"Reload", RELOAD, KEYGRABBER_OK|ROOTMENU_OK}, // Rereads configuration files
	{"Restart", RESTART, KEYGRABBER_OK|ROOTMENU_OK}, // Restarts pekwm
	{"RestartOther", RESTART_OTHER, ROOTMENU_OK|KEYGRABBER_OK}, // Quits pekwm and starts another application
	{"Exit", EXIT, KEYGRABBER_OK|ROOTMENU_OK}, // Exits pekwm

#ifdef MENUS
	{"Submenu", SUBMENU, ROOTMENU_OK},
	{"SubmenuEnd", SUBMENU_END, ROOTMENU_OK},
#endif // MENUS
	{"NoAction", NO_ACTION, 0} // Do nothing
};

Config::Config() :
m_num_workspaces(4), m_edge_snap(10),
m_focus_model(FOCUS_SLOPPY),
m_placement_model(NO_PLACEMENT),
m_fallback_placement_model(NO_PLACEMENT),
m_is_wire_move(true), m_is_edge_snap(true),
m_grab_when_resize(true),
m_report_only_frames(false),
m_double_click_time(250)
{
	loadConfig();
}

Config::~Config()
{
}

void
Config::loadConfig(void)
{
	string cfg_file = getenv("HOME") + string("/.pekwm/config");

	BaseConfig cfg(cfg_file, "*", ";");
	if (! cfg.loadConfig()) {
		copyConfigFiles(); // try copying the files

		if (! cfg.loadConfig()) {
			cfg_file = DATADIR "/config";
			cfg.setFile(cfg_file);
			cfg.loadConfig();
		}
	}

	if (cfg.isLoaded()) {
		string s_value;

		cfg.getValue("numworkspaces", m_num_workspaces);
		cfg.getValue("edgesnapwidth", m_edge_snap);
		cfg.getValue("wiremove", m_is_wire_move);
		cfg.getValue("edgesnap", m_is_edge_snap);
		cfg.getValue("grabwhenresize", m_grab_when_resize);
		cfg.getValue("doubleclicktime", m_double_click_time);
		cfg.getValue("reportonlyframes", m_report_only_frames);

		if (cfg.getValue("keyfile", m_key_file)) {
			Util::expandFileName(m_key_file);
		} else {
			m_key_file = DATADIR "/keys";
		}

		if (cfg.getValue("menufile", m_menu_file)) {
			Util::expandFileName(m_menu_file);
		} else {
			m_menu_file = DATADIR "/menu";
		}

		if (cfg.getValue("themedir", m_theme_dir)) {
			Util::expandFileName(m_theme_dir);
		} else {
			m_theme_dir = DATADIR "/themes/default";
		}

		if (cfg.getValue("autopropsfile", m_autoprops_file)) {
			Util::expandFileName(m_autoprops_file);
		} else {
			m_autoprops_file = DATADIR "/autoprops";
		}

		if (cfg.getValue("startfile", m_start_file)) {
			Util::expandFileName(m_start_file);
		} else {
			m_start_file = DATADIR "/start";
		}

		if (cfg.getValue("focusmodel", s_value)) {
			m_focus_model = getFocusModel(s_value);
			if (m_focus_model == NO_FOCUS) {
				m_focus_model = FOCUS_SLOPPY;
			}
		} else {
			m_focus_model = FOCUS_SLOPPY;
		}

		if (cfg.getValue("placementmodel", s_value)) {
			vector<string> models;
			if ((Util::splitString(s_value, models, " \t", 2)) == 2) {
				m_placement_model = getPlacementModel(models[0]);
				m_fallback_placement_model = getPlacementModel(models[1]);
			} else {
				m_placement_model = getPlacementModel(s_value);
			}
		}

		if (m_placement_model == NO_PLACEMENT)
			m_placement_model = MOUSE_CENTERED;

		parseMouseConfig(&cfg);
	}
}

bool
Config::copyConfigFiles(void)
{	
	string cfg_dir = getenv("HOME") + string("/.pekwm");

	string cfg_file = cfg_dir + string("/config");
	string keys_file = cfg_dir + string("/keys");
	string menu_file = cfg_dir + string("/menu");
	string autoprops_file = cfg_dir + string("/autoprops");
	string start_file = cfg_dir + string("/start");

	bool cp_config, cp_keys, cp_menu, cp_autoprops, cp_start;
	cp_config = cp_keys = cp_menu = cp_autoprops = cp_start = false;

	struct stat stat_buf;
	// check and see if we allready have a ~/.pekwm/ directory
	if (stat(cfg_dir.c_str(), &stat_buf) == 0) {
		// is it a dir or file?
		if (!S_ISDIR(stat_buf.st_mode)) {
			cerr << cfg_dir << " allready exists and isn't a directory" << endl
					 << "Can't copy config files !" << endl;

			return false;
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
				return false;
			}
		}

		// we apperently could write and exec that dir, now see if we have any
		// files in it
		if (stat(cfg_file.c_str(), &stat_buf))
			cp_config = true;
		if (stat(keys_file.c_str(), &stat_buf))
			cp_keys = true;
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
			return false;	
		}		

		cp_config = cp_keys = cp_menu = cp_autoprops = cp_start = true;
	}

	if (cp_config)
		copyTextFile(DATADIR "/config", cfg_file);
	if (cp_keys)
		copyTextFile(DATADIR "/keys", keys_file);
	if (cp_menu)
		copyTextFile(DATADIR "/menu", menu_file);
	if (cp_autoprops)
		copyTextFile(DATADIR "/autoprops", autoprops_file);
	if (cp_start)
		copyTextFile(DATADIR "/start", start_file);

	return true;
}


void Config::copyTextFile(const string &from, const string &to)
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

//! @fn    void parseMouseConfig(BaseConfig *cfg)
//! @brief Parses the config cfg for mouse button actions.
void
Config::parseMouseConfig(BaseConfig *cfg)
{
	if (!cfg)
		return;

	// make sure old actions get unloaded
	m_root_click.clear();
	m_frame_click.clear();
	m_client_click.clear();

	vector<string> split_line;
	vector<string>::iterator token;

	string line;

	MouseButtonType type;
	while (true) {
		line.erase();
		if (cfg->getValue("frame.click", line))
			type = BUTTON_FRAME_SINGLE;
		else if (cfg->getValue("frame.click.double", line))
			type = BUTTON_FRAME_DOUBLE;
		else if (cfg->getValue("frame.motion", line))
			type = BUTTON_FRAME_MOTION;
		else if (cfg->getValue("client.click", line))
			type = BUTTON_CLIENT_SINGLE;
		else if (cfg->getValue("client.motion", line))
			type = BUTTON_CLIENT_MOTION;
		else if (cfg->getValue("root.click", line))
			type = BUTTON_ROOT_SINGLE;
		else
			break; // no more buttons left

		if (split_line.size())
			split_line.clear();

		unsigned int num = Util::splitString(line, split_line, ":", 3);
		if (num > 1) {
			token = split_line.begin();
		} else {
			continue; // not a valid line
		}

		MouseButtonAction click;
		click.type = type;

		vector<string> buttons;
		if (Util::splitString(*token, buttons, " \t")) {
			bool got_button = false;

			// First, we try to get the modifier and buttons used
			vector<string>::iterator it = buttons.begin();
			for (; it != buttons.end(); ++it) {
				int mod;
				if ((mod = getMod(*it)) != -1) {
					click.mod |= mod;
				} else {
					click.button = getMouseButton(*it);
					if (click.button >= Button1) {
						got_button = true;
					}
					break;
				}
			}

			if (!got_button) {
				continue; // not an valid button
			}

		} else if ((click.button = getMouseButton(*token)) < Button1) {
			continue; // not an valid button
		}

		// the action resides in next token, also different actions are
		// valid based on what type it is.
		++token;

		switch (type) {
		case BUTTON_FRAME_SINGLE:
		case BUTTON_FRAME_DOUBLE:
			click.action = getAction(*token, FRAMECLICK_OK);
			break;
		case BUTTON_FRAME_MOTION:
			click.action = getAction(*token, FRAMEMOTION_OK);
			break;
		case BUTTON_CLIENT_SINGLE:
			click.action = getAction(*token, CLIENTCLICK_OK);
			break;
		case BUTTON_CLIENT_MOTION:
			click.action = getAction(*token, CLIENTMOTION_OK);
			break;
		case BUTTON_ROOT_SINGLE:
			click.action = getAction(*token, ROOTCLICK_OK);
			break;
		default:
			click.action = NO_ACTION;
			break;
		}

		if (click.action == NO_ACTION) {
			continue; // not an valid action
		}

		// for the actions that have parameters
		switch (click.action) {
		case EXEC:
			if (num > 2)
				click.s_param = *++token;
			break;
		default:
			// Do nothing
			break;
		}

		switch (click.type) {
		case BUTTON_FRAME_SINGLE:
		case BUTTON_FRAME_DOUBLE:
		case BUTTON_FRAME_MOTION:
			m_frame_click.push_back(click);
			break;
		case BUTTON_CLIENT_SINGLE:
		case BUTTON_CLIENT_MOTION:
			m_client_click.push_back(click);
			break;
		case BUTTON_ROOT_SINGLE:
			m_root_click.push_back(click);			
			break;
		default:
			// do nothing
			break;
		}
	}
}

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

int
Config::getMod(const string &mod)
{
	if (! mod.size())
		return 0;
		
	for (unsigned int i = 0; m_modlist[i].mask != -1; ++i) {
		if (m_modlist[i] ==  mod) {
			return m_modlist[i].mask;
		}
	}
	
	return -1;
}

Actions
Config::getAction(const string &name, int mask)
{
	for (unsigned int i = 0; m_actionlist[i].action != NO_ACTION; ++i) {
		if ((m_actionlist[i].mask&mask) && (m_actionlist[i] == name)) {
			return m_actionlist[i].action;
		}
	}

	return NO_ACTION;
}
