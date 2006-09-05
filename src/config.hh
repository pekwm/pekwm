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

#include <list>

enum ActionOk {
	KEYGRABBER_OK = (1<<1),
	FRAME_OK = (1<<2),
	CLIENT_OK = (1<<3),
	ROOTCLICK_OK = (1<<4),
	BUTTONCLICK_OK = (1<<5),
	WINDOWMENU_OK = (1<<6),
	ROOTMENU_OK = (1<<7)
};

class Config
{
public:
	Config();
	~Config();

	void load(void);

	// Files
	inline const std::string &getKeyFile(void) const { return m_files_keys; }
	inline const std::string &getMenuFile(void) const { return m_files_menu; }
	inline const std::string &getStartFile(void) const { return m_files_start; }
	inline const std::string &getAutoPropsFile(void) const {
		return m_files_autoprops; }
	inline const std::string &getThemeFile(void) const { return m_files_theme; }

	// Moveresize
	inline unsigned int getEdgeSnap(void) const { return m_moveresize_edgesnap; }
	inline unsigned int getFrameSnap(void) const { return m_moveresize_framesnap; }
	inline bool getOpaqueMove(void) const { return m_moveresize_opaquemove; }
	inline bool getOpaqueResize(void) const { return m_moveresize_opaqueresize; }
	inline bool getGrabWhenResize(void) const { return m_moveresize_grabwhenresize; }
	inline unsigned int getWorkspaceWarp(void) const { return m_moveresize_workspacewarp; }
	inline bool getWrapWorkspaceWarp(void) const { return m_moveresize_wrapworkspacewarp; }

	// Screen
	inline unsigned int getWorkspaces(void) const { return m_screen_workspaces; }
	inline unsigned int getDoubleClickTime(void) const { return m_screen_doubleclicktime; }
	inline bool getReportOnlyFrames(void) const { return m_screen_reportonlyframes; }
	inline unsigned int getFocusModel(void) const { return m_screen_focusmodel; }
	inline unsigned int getPlacementModel(void) const {
		return m_screen_placementmodel;
	}
	inline unsigned int getFallbackPlacementModel(void) const {
		return m_screen_fallbackplacementmodel;
	}
#ifdef HARBOUR
	inline unsigned int getHarbourPlacement(void) const {
		return m_harbour_placement;
	}
	inline unsigned int getHarbourOrientation(void) const {
		return m_harbour_orientation;
	}
	inline bool getHarbourOntop(void) const { return m_harbour_ontop; }
#endif // HARBOUR

	inline std::list<MouseButtonAction>
	*getMouseFrameList(void) { return &m_mouse_frame; }
	inline std::list<MouseButtonAction>
	*getMouseClientList(void) { return &m_mouse_client; }
	inline std::list<MouseButtonAction>
	*getMouseRootList(void) { return &m_mouse_root; }

	// Search in lists...
	Actions getAction(const std::string &name, unsigned int mask);
	FocusModel getFocusModel(const std::string &model);
	PlacementModel getPlacementModel(const std::string &model);
	Corner getCorner(const std::string &corner);
#ifdef HARBOUR
	HarbourPlacement getHarbourPlacement(const std::string &model);
	Orientation getOrientatition(const std::string &orientation);
#endif // HARBOUR
	unsigned int getMod(const std::string &mod);
	unsigned int getMouseButton(const std::string &button);

private:
	void copyConfigFiles(void);
	void copyTextFile(const std::string &from, const std::string &to);

	void loadMouseConfig(const std::string &file);
	void parseButtons(BaseConfig::CfgSection *cs,
										std::list<MouseButtonAction> *mouse_list,
										ActionOk action_ok);

private:
	// files
	std::string m_files_keys, m_files_menu;
	std::string m_files_start, m_files_autoprops;
	std::string m_files_theme;

	// moveresize
	unsigned int m_moveresize_edgesnap, m_moveresize_framesnap;
	bool m_moveresize_opaquemove, m_moveresize_opaqueresize;
	bool m_moveresize_grabwhenresize;
	unsigned int m_moveresize_workspacewarp;
	bool m_moveresize_wrapworkspacewarp;

	// screen
	unsigned int m_screen_workspaces;
	unsigned int m_screen_doubleclicktime;
	bool m_screen_reportonlyframes;
	unsigned int m_screen_focusmodel;
	unsigned int m_screen_placementmodel, m_screen_fallbackplacementmodel;

	// harbour
#ifdef HARBOUR
	bool m_harbour_ontop;
	unsigned int m_harbour_placement;
	unsigned int m_harbour_orientation;
#endif // HARBOUR

	std::list<MouseButtonAction> m_mouse_frame;
	std::list<MouseButtonAction> m_mouse_client;
	std::list<MouseButtonAction> m_mouse_root;

	struct ActionListItem {
		const char *name;
		Actions action;
		unsigned int mask;

		inline bool operator == (std::string s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static ActionListItem m_actionlist[];

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
	static UnsignedListItem m_cornerlist[];
	static UnsignedListItem m_modlist[];
#ifdef HARBOUR
	static UnsignedListItem m_harbour_placementlist[];
	static UnsignedListItem m_orientationlist[];
#endif // HARBOUR




};

#endif // _CONFIG_HH_
