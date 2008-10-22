//
// Config.hh for pekwm
// Copyright © 2002-2008 Claes Nasten <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _CONFIG_HH_
#define _CONFIG_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "Action.hh"
#include "CfgParser.hh"
#include "ParseUtil.hh"

#include <list>
#include <vector>
#include <map>
#include <utility>

class Config
{
public:
    Config(void);
    ~Config(void);

    static Config* instance(void) { return _instance; }

    bool load(const std::string &config_file);
    bool loadMouseConfig(const std::string &mouse_file);

    inline const std::string &getConfigFile(void) const { return _config_file; }

    // Files
    const std::string &getKeyFile(void) const { return _files_keys; }
    const std::string &getMenuFile(void) const { return _files_menu; }
    const std::string &getStartFile(void) const { return _files_start; }
    const std::string &getAutoPropsFile(void) const { return _files_autoprops; }
    const std::string &getThemeFile(void) const { return _files_theme; }
    const std::string &getMouseConfigFile(void) const { return _files_mouse; }

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
    inline int getWorkspacesPerRow(void) const { return _screen_workspaces_per_row; }
    void getDesktopNamesUTF8(uchar **names, uint *length) const;
    const std::wstring &getWorkspaceName(uint num) const {
        return (num >= _screen_workspace_names.size())?_screen_workspace_name_default:
                                                       _screen_workspace_names[num];
    }
    void setDesktopNamesUTF8(char *names, uint length);

    inline int getScreenEdgeSize(EdgeType edge) const { return _screen_edge_sizes[edge]; }
    inline bool getScreenEdgeIndent(void) const { return _screen_edge_indent; }
    inline int getDoubleClickTime(void) const { return _screen_doubleclicktime; }
    inline const std::wstring &getTrimTitle(void) const { return _screen_trim_title; }

    inline bool isFullscreenAbove(void) const { return _screen_fullscreen_above; }
    inline bool isFullscreenDetect(void) const { return _screen_fullscreen_detect; }

    inline bool getShowFrameList(void) const { return _screen_showframelist; }
    inline bool isShowStatusWindow(void) const { return _screen_show_status_window; }
    inline bool isShowClientID(void) const { return _screen_show_client_id; }
    int getShowWorkspaceIndicator(void) const { return _screen_show_workspace_indicator; }
    int getWorkspaceIndicatorScale(void) const { return _screen_workspace_indicator_scale; }
    inline bool isPlaceNew(void) const { return _screen_place_new; }
    inline bool isFocusNew(void) const { return _screen_focus_new; }
    inline bool isFocusNewChild(void) const { return _screen_focus_new_child; }
    inline bool isHonourRandr(void) const { return _screen_honour_randr; }

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

    inline bool isMenuSelectOn(uint val) const { return (_menu_select_mask&val); }
    inline bool isMenuEnterOn(uint val) const { return (_menu_enter_mask&val); }
    inline bool isMenuExecOn(uint val) const { return (_menu_exec_mask&val); }

    bool isCmdDialogHistoryUnique(void) const { return _cmd_dialog_history_unique; }
    int getCmdDialogHistorySize(void) const { return _cmd_dialog_history_size; }
    const std::string &getCmdDialogHistoryFile(void) const { return _cmd_dialog_history_file; }
    int getCmdDialogHistorySaveInterval(void) const { return _cmd_dialog_history_save_interval; }

#ifdef HARBOUR
    inline int getHarbourDAMinSide(void) const { return _harbour_da_min_s; }
    inline int getHarbourDAMaxSide(void) const { return _harbour_da_max_s; }
    inline int getHarbourHead(void) const { return _harbour_head_nr; }
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
    bool parseActionEvent(CfgParser::Entry *section, ActionEvent &ae, uint mask, bool button);

    bool parseMoveResizeAction(const std::string &action_string, Action &action);
    bool parseMoveResizeActions(const std::string &actions, ActionEvent &ae);
    bool parseMoveResizeEvent(CfgParser::Entry *section, ActionEvent &ae);

    bool parseInputDialogAction(const std::string &val, Action &action);
    bool parseInputDialogActions(const std::string &actions, ActionEvent &ae);
    bool parseInputDialogEvent(CfgParser::Entry *section, ActionEvent &ae);

    uint getMenuMask(const std::string &mask);
    uint getMenuIconWidth(void) const { return _menu_icon_width; }
    uint getMenuIconHeight(void) const { return _menu_icon_height; }

#ifdef MENUS
    bool parseMenuAction(const std::string& action_string, Action& action);
    bool parseMenuActions(const std::string& actions, ActionEvent& ae);
    bool parseMenuEvent(CfgParser::Entry *section, ActionEvent& ae);
#endif // MENUS

    inline uint getMod(const std::string &mod) { return ParseUtil::getValue<uint>(mod, _mod_map); }
    uint getMouseButton(const std::string& button);

private:
    void copyConfigFiles(void);

    void loadFiles(CfgParser::Entry *section);
    void loadMoveResize(CfgParser::Entry *section);
    void loadScreen(CfgParser::Entry *section);
    void loadMenu(CfgParser::Entry *section);
    void loadCmdDialog(CfgParser::Entry *section);
#ifdef HARBOUR
    void loadHarbour(CfgParser::Entry *section);
#endif // HARBOUR

    void parseButtons(CfgParser::Entry *section, std::list<ActionEvent>* mouse_list, ActionOk action_ok);

    int parseWorkspaceNumber(const std::string &workspace);

private:
    std::string _config_file; /**< Path to config file last loaded. */
    time_t _config_mtime; /**< Mtime of config file last loaded. */
    std::string _mouse_file; /**< Path to mouse config file last loaded. */
    time_t _mouse_mtime; /**< Mtime of mouse config file last loaded. */

    // files
    std::string _files_keys, _files_menu;
    std::string _files_start, _files_autoprops;
    std::string _files_theme, _files_mouse;

    // moveresize
    int _moveresize_edgeattract, _moveresize_edgeresist;
    int _moveresize_woattract, _moveresize_woresist;
    bool _moveresize_opaquemove, _moveresize_opaqueresize;

    // screen
    int _screen_workspaces, _screen_pixmap_cache_size;
    int _screen_workspaces_per_row;
    std::vector<std::wstring> _screen_workspace_names;
    std::wstring _screen_workspace_name_default;
    std::vector<int> _screen_edge_sizes;
    bool _screen_edge_indent;
    int _screen_doubleclicktime;
    std::wstring _screen_trim_title;
    bool _screen_fullscreen_above; //!< Flag to make fullscreen go above all windows. */
    bool _screen_fullscreen_detect; /**< Flag to make configure request fullscreen detection. */
    bool _screen_showframelist;
    bool _screen_show_status_window;
    bool _screen_show_client_id; //!< Flag to display client ID in title.
    int _screen_show_workspace_indicator; //!< Display workspace indicator for N seconds.
    int _screen_workspace_indicator_scale; //!< Scale of the workspace indicator head
    bool _screen_place_new, _screen_focus_new, _screen_focus_new_child;
    bool _screen_honour_randr; /**< Boolean flag if randr information should be honoured. */
    bool _screen_placement_row, _screen_placement_ltr, _screen_placement_ttb;
    int _screen_placement_offset_x, _screen_placement_offset_y;
    std::list<uint> _screen_placementmodels;
    bool _screen_client_unique_name;
    std::string _screen_client_unique_name_pre, _screen_client_unique_name_post;

    uint _menu_select_mask, _menu_enter_mask, _menu_exec_mask;
    uint _menu_icon_width, _menu_icon_height;

    bool _cmd_dialog_history_unique; /**< Boolean flag, when true entries in the CmdDialog history are unique. */
    int _cmd_dialog_history_size; /**< Number of entries in the history before the last entries are dropped. */
    std::string _cmd_dialog_history_file; /**< Path to cmd dialog history file. */
    int _cmd_dialog_history_save_interval; /**< Save history file each Nth CmdDialog exec. */

    // harbour
#ifdef HARBOUR
    int _harbour_da_min_s, _harbour_da_max_s;
    bool _harbour_ontop;
    bool _harbour_maximize_over;
    uint _harbour_placement;
    uint _harbour_orientation;
    int _harbour_head_nr;
#endif // HARBOUR

    std::map<MouseActionListName, std::list<ActionEvent>* > _mouse_action_map;

    static Config *_instance;

    std::map<ParseUtil::Entry, std::pair<ActionType, uint> > _action_map;
    std::map<ParseUtil::Entry, PlacementModel> _placement_map;
    std::map<ParseUtil::Entry, OrientationType> _edge_map;
    std::map<ParseUtil::Entry, Raise> _raise_map;
    std::map<ParseUtil::Entry, Skip> _skip_map;
    std::map<ParseUtil::Entry, Layer> _layer_map;
    std::map<ParseUtil::Entry, MoveResizeActionType> _moveresize_map;
    std::map<ParseUtil::Entry, InputDialogAction> _inputdialog_map;
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
