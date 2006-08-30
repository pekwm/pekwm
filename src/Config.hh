//
// Config.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _CONFIG_HH_
#define _CONFIG_HH_

#include "pekwm.hh"
#include "Action.hh"
#include "BaseConfig.hh"

#include <list>

class Config
{
public:
	Config(Display* dpy);
	~Config();

	void load(void);

	// Files
	inline const std::string &getKeyFile(void) const { return _files_keys; }
	inline const std::string &getMenuFile(void) const { return _files_menu; }
	inline const std::string &getStartFile(void) const { return _files_start; }
	inline const std::string &getAutoPropsFile(void) const {
		return _files_autoprops; }
	inline const std::string &getThemeFile(void) const { return _files_theme; }

	// Moveresize
	inline unsigned int getEdgeSnap(void) const { return _moveresize_edgesnap; }
	inline unsigned int getFrameSnap(void) const { return _moveresize_framesnap; }
	inline bool getOpaqueMove(void) const { return _moveresize_opaquemove; }
	inline bool getOpaqueResize(void) const { return _moveresize_opaqueresize; }
	inline bool getGrabWhenResize(void) const { return _moveresize_grabwhenresize; }
	inline unsigned int getWWFrame(void) const { return _moveresize_ww_frame; }
	inline bool getWWFrameWrap(void) const { return _moveresize_ww_frame_wrap; }
	inline unsigned int getWWMouse(void) const { return _moveresize_ww_mouse; }
	inline bool getWWMouseWrap(void) const { return _moveresize_ww_mouse_wrap; }

	// Screen
	inline unsigned int getWorkspaces(void) const { return _screen_workspaces; }
	inline unsigned int getDoubleClickTime(void) const { return _screen_doubleclicktime; }
	inline unsigned int getFocusMask(void) const { return _screen_focusmask; }
	inline bool getShowFrameList(void) const { return _screen_showframelist; }

	inline bool getPlacementRow(void) const { return _screen_placement_row; }
	inline bool getPlacementLtR(void) const { return _screen_placement_ltr; }
	inline bool getPlacementTtB(void) const { return _screen_placement_ttb; }
	inline std::list<unsigned int> *getPlacementModelList(void) {
		return &_screen_placementmodels;
	}
#ifdef HARBOUR
	inline bool getHarbourOntop(void) const { return _harbour_ontop; }
	inline bool getHarbourMaximizeOver(void) const {
		return _harbour_maximize_over;
	}
	inline unsigned int getHarbourPlacement(void) const {
		return _harbour_placement;
	}
	inline unsigned int getHarbourOrientation(void) const {
		return _harbour_orientation;
	}
#endif // HARBOUR

	inline std::list<ActionEvent>
	*getMouseFrameList(void) { return &_mouse_frame; }
	inline std::list<ActionEvent>
	*getMouseClientList(void) { return &_mouse_client; }
	inline std::list<ActionEvent>
	*getMouseRootList(void) { return &_mouse_root; }

	// Search in lists...
	ActionType getAction(const std::string& name, unsigned int mask);
	PlacementModel getPlacementModel(const std::string& model);
	Edge getEdge(const std::string& edge);
	Raise getRaise(const std::string& raise);
	ApplyOn getApplyOn(const std::string& apply);
	Skip getSkip(const std::string& skip);
	Layer getLayer(const std::string& layer);
	MoveResizeActionType getMoveResizeAction(const std::string &action);
#ifdef MENUS
	MenuType getMenuType(const std::string& type);
	MenuActionType getMenuAction(const std::string& action);
#endif // MENUS
#ifdef HARBOUR
	HarbourPlacement getHarbourPlacement(const std::string &model);
	Orientation getOrientatition(const std::string &orientation);
#endif // HARBOUR

	bool parseKey(const std::string& key_string,
								unsigned int& mod, unsigned int& key);
	bool parseButton(const std::string& button_string,
									 unsigned int& mod, unsigned int& button);
	bool parseAction(const std::string& action_string, Action& action,
									 unsigned int mask);
	bool parseActions(const std::string& actions, ActionEvent& ae,
										unsigned int mask);
	bool parseActionEvent(BaseConfig::CfgSection* sect, ActionEvent& ae,
												unsigned int mask, bool button);

	bool parseMoveResizeAction(const std::string& action_string, Action& action);
	bool parseMoveResizeActions(const std::string& actions, ActionEvent& ae);
	bool parseMoveResizeEvent(BaseConfig::CfgSection* sect, ActionEvent& ae);

#ifdef MENUS
	bool parseMenuAction(const std::string& action_string, Action& action);
	bool parseMenuActions(const std::string& actions, ActionEvent& ae);
	bool parseMenuEvent(BaseConfig::CfgSection* sect, ActionEvent& ae);
#endif // MENUS

	unsigned int getMod(const std::string& mod);
	unsigned int getMouseButton(const std::string& button);

private:
	void copyConfigFiles(void);
	void copyTextFile(const std::string &from, const std::string &to);

	void loadMouseConfig(const std::string &file);
	void parseButtons(BaseConfig::CfgSection *cs,
										std::list<ActionEvent>* mouse_list, ActionOk action_ok);

private:
	Display *_dpy;

	// files
	std::string _files_keys, _files_menu;
	std::string _files_start, _files_autoprops;
	std::string _files_theme;

	// moveresize
	unsigned int _moveresize_edgesnap, _moveresize_framesnap;
	bool _moveresize_opaquemove, _moveresize_opaqueresize;
	bool _moveresize_grabwhenresize;
	unsigned int _moveresize_ww_frame, _moveresize_ww_mouse;
	bool _moveresize_ww_frame_wrap, _moveresize_ww_mouse_wrap;

	// screen
	unsigned int _screen_workspaces;
	unsigned int _screen_doubleclicktime;
	unsigned int _screen_focusmask;
	bool _screen_showframelist;
	bool _screen_placement_row, _screen_placement_ltr, _screen_placement_ttb;
	std::list<unsigned int> _screen_placementmodels;

	// harbour
#ifdef HARBOUR
	bool _harbour_ontop;
	bool _harbour_maximize_over;
	unsigned int _harbour_placement;
	unsigned int _harbour_orientation;
#endif // HARBOUR

	std::list<ActionEvent> _mouse_frame;
	std::list<ActionEvent> _mouse_client;
	std::list<ActionEvent> _mouse_root;

	struct ActionListItem {
		const char *name;
		ActionType action;
		unsigned int mask;

		inline bool operator == (std::string s) {
			return (strcasecmp(name, s.c_str()) ? false : true);
		}
	};
	static ActionListItem _actionlist[];

	struct UnsignedListItem {
		const char *name;
		unsigned int value;
		inline bool operator == (const std::string& s) {
			return !strcasecmp(name, s.c_str());
		}
	};

	static UnsignedListItem _placementlist[];
	static UnsignedListItem _edgelist[];
	static UnsignedListItem _raiselist[];
	static UnsignedListItem _modlist[];
	static UnsignedListItem _applyonlist[];
	static UnsignedListItem _skiplist[];
	static UnsignedListItem _layerlist[];
	static UnsignedListItem _moveresizeactionlist[];
#ifdef MENUS
	static UnsignedListItem _menutypelist[];
	static UnsignedListItem _menuactionlist[];
#endif // MENUS
#ifdef HARBOUR
	static UnsignedListItem _harbour_placementlist[];
	static UnsignedListItem _orientationlist[];
#endif // HARBOUR
};

#endif // _CONFIG_HH_
