//
// Config.hh for pekwm
// Copyright (C) 2002-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _CONFIG_HH_
#define _CONFIG_HH_

#include "pekwm.hh"
#include "Action.hh"
#include "CfgParser.hh"
#include "ParseUtil.hh"

#include <list>
#include <map>
#include <utility>

class Config
{
public:
    Config(void);
    ~Config(void);

    static Config* instance(void) { return _instance; }

    void load(const std::string &config_file);

    inline const std::string &getConfigFile(void) const { return _config_file; }

    // Files
    inline const std::string &getKeyFile(void) const { return _files_keys; }
    inline const std::string &getMenuFile(void) const { return _files_menu; }
    inline const std::string &getStartFile(void) const { return _files_start; }
    inline const std::string &getAutoPropsFile(void) const {
        return _files_autoprops;
    }
    inline const std::string &getThemeFile(void) const { return _files_theme; }

    // Moveresize
    inline int getEdgeAttract(void) const { return _moveresize_edgeattract; }
    inline int getEdgeResist(void) const { return _moveresize_edgeresist; }
    inline int getWOAttract(void) const { return _moveresize_woattract; }
    inline int getWOResist(void) const { return _moveresize_woresist; }
    inline bool getOpaqueMove(void) const { return _moveresize_opaquemove; }
    inline bool getOpaqueResize(void) const { return _moveresize_opaqueresize; }

    // Screen
    inline int getWorkspaces(void) const { return _screen_workspaces; }
    inline int getScreenPixmapCacheSize(void) const { return _screen_pixmap_cache_size; }
    inline int getScreenEdgeSize(void) const { return _screen_edge_size; }
    inline int getDoubleClickTime(void) const { return _screen_doubleclicktime; }
    inline const std::string &getTrimTitle(void) const { return _screen_trim_title; }

    inline bool getShowFrameList(void) const { return _screen_showframelist; }
    inline bool isShowStatusWindow(void) const { return _screen_show_status_window; }
    inline bool isShowClientID(void) const { return _screen_show_client_id; }
    inline bool isPlaceNew(void) const { return _screen_place_new; }
    inline bool isFocusNew(void) const { return _screen_focus_new; }
    inline bool isFocusNewChild(void) const { return _screen_focus_new_child; }

    inline std::list<uint>::iterator getPlacementModelBegin(void) { return _screen_placementmodels.begin(); }
    inline std::list<uint>::iterator getPlacementModelEnd(void) { return _screen_placementmodels.end(); }

    inline bool getPlacementRow(void) const { return _screen_placement_row; }
    inline bool getPlacementLtR(void) const { return _screen_placement_ltr; }
    inline bool getPlacementTtB(void) const { return _screen_placement_ttb; }
    inline int getPlacementOffsetX(void) const { return _screen_placement_offset_x; }
    inline int getPlacementOffsetY(void) const { return _screen_placement_offset_y; }

    inline bool getClientUniqueName(void) const { return _screen_client_unique_name; }
    inline const std::string &getClientUniqueNamePre(void) const {
        return _screen_client_unique_name_pre;
    }
    inline const std::string &getClientUniqueNamePost(void) const {
        return _screen_client_unique_name_post;
    }

    inline int getViewportCols(void) const { return _viewport_cols; }
    inline int getViewportRows(void) const { return _viewport_rows; }

    inline bool isMenuSelectOn(uint val) const { return (_menu_select_mask&val); }
    inline bool isMenuEnterOn(uint val) const { return (_menu_enter_mask&val); }
    inline bool isMenuExecOn(uint val) const { return (_menu_exec_mask&val); }

#ifdef HARBOUR
    inline int getHarbourDAMinSide(void) const { return _harbour_da_min_s; }
    inline int getHarbourDAMaxSide(void) const { return _harbour_da_max_s; }
#ifdef HAVE_XINERAMA
    inline int getHarbourHead(void) const { return _harbour_head_nr; }
#endif // HAVE_XINERAMA
    inline bool isHarbourOntop(void) const { return _harbour_ontop; }
    inline bool isHarbourMaximizeOver(void) const { return _harbour_maximize_over; }
    inline uint getHarbourPlacement(void) const { return _harbour_placement; }
    inline uint getHarbourOrientation(void) const { return _harbour_orientation; }
#endif // HARBOUR

    inline std::list<ActionEvent>  *getMouseActionList(MouseActionListName name)
    { return _mouse_action_map[name]; }

    std::list<ActionEvent> *getBorderListFromPosition(uint pos);
    std::list<ActionEvent> *getEdgeListFromPosition(uint pos);

    // map parsing
    ActionType getAction(const std::string &name, uint mask);
    inline Layer getLayer(const std::string &layer) { return ParseUtil::getValue<Layer>(layer, _layer_map); }
    inline Skip getSkip(const std::string &skip) { return ParseUtil::getValue<Skip>(skip, _skip_map); }
    inline CfgDeny getCfgDeny(const std::string &deny) { return ParseUtil::getValue<CfgDeny>(deny, _cfg_deny_map); }

    bool parseKey(const std::string &key_string, uint& mod, uint &key);
    bool parseButton(const std::string &button_string, uint &mod, uint &button);
    bool parseAction(const std::string &action_string, Action &action, uint mask);
    bool parseActionState(Action &action, const std::string &st_action);
    bool parseActions(const std::string &actions, ActionEvent &ae, uint mask);
    bool parseActionEvent(CfgParser::Entry *op_section, ActionEvent &ae,
                          uint mask, bool button);

    bool parseMoveResizeAction(const std::string &action_string, Action &action);
    bool parseMoveResizeActions(const std::string &actions, ActionEvent &ae);
    bool parseMoveResizeEvent(CfgParser::Entry *op_section, ActionEvent &ae);

    bool parseCmdDialogAction(const std::string &val, Action &action);
    bool parseCmdDialogActions(const std::string &actions, ActionEvent &ae);
    bool parseCmdDialogEvent(CfgParser::Entry *op_section, ActionEvent &ae);

    uint getMenuMask(const std::string &mask);
    uint getMenuIconWidth(void) const { return _menu_icon_width; }
    uint getMenuIconHeight(void) const { return _menu_icon_height; }

#ifdef MENUS
    bool parseMenuAction(const std::string& action_string, Action& action);
    bool parseMenuActions(const std::string& actions, ActionEvent& ae);
    bool parseMenuEvent(CfgParser::Entry *op_section, ActionEvent& ae);
#endif // MENUS

    inline uint getMod(const std::string &mod) { return ParseUtil::getValue<uint>(mod, _mod_map); }
    uint getMouseButton(const std::string& button);

private:
    void copyConfigFiles(void);
    void copyTextFile(const std::string &from, const std::string &to);

    void loadFiles(CfgParser::Entry *section);
    void loadMoveReszie(CfgParser::Entry *section);
    void loadScreen(CfgParser::Entry *section);
    void loadMenu(CfgParser::Entry *section);
#ifdef HARBOUR
    void loadHarbour(CfgParser::Entry *section);
#endif // HARBOUR

    void loadMouseConfig(const std::string &file);
    void parseButtons(CfgParser::Entry *op_section,
                      std::list<ActionEvent>* mouse_list, ActionOk action_ok);

private:
    std::string _config_file;

    // files
    std::string _files_keys, _files_menu;
    std::string _files_start, _files_autoprops;
    std::string _files_theme;

    // moveresize
    int _moveresize_edgeattract, _moveresize_edgeresist;
    int _moveresize_woattract, _moveresize_woresist;
    bool _moveresize_opaquemove, _moveresize_opaqueresize;

    // screen
    int _screen_workspaces, _screen_pixmap_cache_size;
    int _screen_edge_size;
    int _screen_doubleclicktime;
    std::string _screen_trim_title;
    bool _screen_showframelist;
    bool _screen_show_status_window;
    bool _screen_show_client_id; //!< Flag to display client ID in title.
    bool _screen_place_new, _screen_focus_new, _screen_focus_new_child;
    bool _screen_placement_row, _screen_placement_ltr, _screen_placement_ttb;
    int _screen_placement_offset_x, _screen_placement_offset_y;
    std::list<uint> _screen_placementmodels;
    bool _screen_client_unique_name;
    std::string _screen_client_unique_name_pre, _screen_client_unique_name_post;

    uint _menu_select_mask, _menu_enter_mask, _menu_exec_mask;
    uint _menu_icon_width, _menu_icon_height;

    // harbour
#ifdef HARBOUR
    int _harbour_da_min_s, _harbour_da_max_s;
    bool _harbour_ontop;
    bool _harbour_maximize_over;
    uint _harbour_placement;
    uint _harbour_orientation;
#ifdef HAVE_XINERAMA
    int _harbour_head_nr;
#endif // HAVE_XINERAMA
#endif // HARBOUR

    // viewport
    int _viewport_cols;
    int _viewport_rows;

    std::map<MouseActionListName, std::list<ActionEvent>* > _mouse_action_map;

    static Config *_instance;

    std::map<ParseUtil::Entry, std::pair<ActionType, uint> > _action_map;
    std::map<ParseUtil::Entry, PlacementModel> _placement_map;
    std::map<ParseUtil::Entry, OrientationType> _edge_map;
    std::map<ParseUtil::Entry, Raise> _raise_map;
    std::map<ParseUtil::Entry, Skip> _skip_map;
    std::map<ParseUtil::Entry, Layer> _layer_map;
    std::map<ParseUtil::Entry, MoveResizeActionType> _moveresize_map;
    std::map<ParseUtil::Entry, CmdDialogAction> _cmddialog_map;
    std::map<ParseUtil::Entry, DirectionType> _direction_map;
    std::map<ParseUtil::Entry, WorkspaceChangeType> _workspace_change_map;
    std::map<ParseUtil::Entry, BorderPosition> _borderpos_map;
    std::map<ParseUtil::Entry, MouseEventType> _mouse_event_map;
    std::map<ParseUtil::Entry, uint> _mod_map;
    std::map<ParseUtil::Entry, ActionStateType> _action_state_map;
    std::map<ParseUtil::Entry, CfgDeny> _cfg_deny_map;
#ifdef MENUS
    std::map<ParseUtil::Entry, ActionType> _menu_action_map;
#endif // MENUS
#ifdef HARBOUR
    std::map<ParseUtil::Entry, HarbourPlacement> _harbour_placement_map;
    std::map<ParseUtil::Entry, Orientation> _harbour_orientation_map;
#endif // HARBOUR
};

#endif // _CONFIG_HH_
