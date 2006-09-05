//
// config.hh for pekwm
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

#ifndef _CONFIG_HH_
#define _CONFIG_HH_

#include "pekwm.hh"
#include "baseconfig.hh"

enum ActionOk {
	KEYGRABBER_OK = (1<<1),
	FRAMECLICK_OK = (1<<2),
	FRAMEMOTION_OK = (1<<3),
	CLIENTCLICK_OK = (1<<4),
	CLIENTMOTION_OK = (1<<5),
	BUTTONCLICK_OK = (1<<6),
	ROOTCLICK_OK = (1<<7),
	WINDOWMENU_OK = (1<<8),
	ROOTMENU_OK = (1<<9)
};


class Config
{
public:
	Config();
	~Config();

	void loadConfig(void); // parses the config file 

	inline const std::string &getKeyFile(void) const { return m_key_file; }
	inline const std::string &getMenuFile(void) const { return m_menu_file; }
	inline const std::string &getThemeDir(void) const { return m_theme_dir; }
	inline const std::string &getAutoPropsFile(void) const {
		return m_autoprops_file; }
	inline const std::string &getStartFile(void) const { return m_start_file; }

	inline unsigned int getFocusModel(void) const { return m_focus_model; }
	inline unsigned int getNumWorkspaces(void) const { return m_num_workspaces; }

	inline unsigned int getPlacementModel(void) const {
		return m_placement_model;
	}
	inline unsigned int getFallbackPlacementModel(void) const {
		return m_fallback_placement_model;
	}

	inline unsigned int getEdgeSnap(void) const { return m_edge_snap; }
	inline unsigned int getDoubleClickTime(void) const
		{ return m_double_click_time; }

	inline bool isWireMove(void) const { return m_is_wire_move; }
	inline bool isEdgeSnap(void) const { return m_is_edge_snap; }
	inline bool grabWhenResize(void) const { return m_grab_when_resize; }
	inline bool getReportOnlyFrames(void) const { return m_report_only_frames; }

	inline std::vector<MouseButtonAction>
	*getRootClickList(void) { return &m_root_click; }
	inline std::vector<MouseButtonAction>
	*getFrameClickList(void) { return &m_frame_click; }
	inline std::vector<MouseButtonAction>
	*getClientClickList(void) { return &m_client_click; }

	Actions getAction(const std::string &name, int mask);
private:
	bool copyConfigFiles(void);
	void copyTextFile(const std::string &from, const std::string &to);

	void parseMouseConfig(BaseConfig *cfg);

	unsigned int getMouseButton(const std::string &button);
	FocusModel getFocusModel(const std::string &model);
	PlacementModel getPlacementModel(const std::string &model);
	int getMod(const std::string &mod);


private:
	std::vector<MouseButtonAction> m_root_click;
	std::vector<MouseButtonAction> m_frame_click;
	std::vector<MouseButtonAction> m_client_click;

	unsigned int m_num_workspaces;
	unsigned int m_edge_snap;
	unsigned int m_focus_model;
	unsigned int m_placement_model;
	unsigned int m_fallback_placement_model;

	bool m_is_wire_move;
	bool m_is_edge_snap;
	bool m_grab_when_resize;
	bool m_report_only_frames;
	std::string m_key_file, m_menu_file, m_theme_dir;
	std::string m_autoprops_file, m_start_file;

	unsigned int m_double_click_time;

	struct UnsignedListItem {
		const char *name;
		unsigned int value;
		inline bool operator == (const std::string &s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};

	static UnsignedListItem m_mousebuttonlist[];
	static UnsignedListItem m_focuslist[];
	static UnsignedListItem m_placementlist[];

	struct ModListItem {
		const char *name;
		int mask;

		inline bool operator == (const std::string &s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static ModListItem m_modlist[];

	struct ActionListItem {
		const char *name;
		Actions action;
		int mask;

		inline bool operator == (std::string s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static ActionListItem m_actionlist[];
};

#endif // _CONFIG_HH_
