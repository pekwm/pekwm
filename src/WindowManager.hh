//
// WindowManager.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me{@}pekdon{.}net>
//
// windowmanager.hh for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef _WINDOWMANAGER_HH_
#define _WINDOWMANAGER_HH_

#include "pekwm.hh"
#include "Action.hh"
#include "Atoms.hh"
#include "PScreen.hh"
#include "Timer.hh"
#include "ManagerWindows.hh"

#include <list>
#include <map>

#ifdef HAVE_XRANDR
extern "C" {
#include <X11/extensions/Xrandr.h>
}
#endif // HAVE_XRANDR

class ActionHandler;
class AutoProperties;
class Config;
class ColorHandler;
class FontHandler;
class TextureHandler;
class Theme;
class Workspaces;

class PScreen;
class ScreenResources;
class PWinObj;
class PDecor;
class Frame;
class Client;
class ClassHint;
class PMenu; // used together with Focus actions

class AutoProperty; // for findGroup
class CmdDialog;
class SearchDialog;
class StatusWindow;
class WorkspaceIndicator;

class KeyGrabber;
#ifdef HARBOUR
class Harbour;
#endif // HARBOUR

class WindowManager
{
public:
    static WindowManager *start(const std::string &command_line,
                                const std::string &config_file, bool replace);
    static void destroy(void);
    inline static WindowManager *instance(void) { return _instance; }

    void doEventLoop(void);

    //! @brief Sets reload status, will reload from main loop.
    void reload(void) { _reload = true; }
    void restart(std::string command = "");
    //! @brief Sets shutdown status, will shutdown from main loop.
    void shutdown(void) { _shutdown = true; }
    /**< Return shutdown flag, set to tru to shutdown window manager. */
    bool *getShutdownFlag(void) { return &_shutdown; }

    // get "base" classes
    inline PScreen *getScreen(void) const { return _screen; }
    inline Config *getConfig(void) const { return _config; }
    inline Theme *getTheme(void) const { return _theme; }
    inline ActionHandler *getActionHandler(void)
    const { return _action_handler; }
    inline AutoProperties* getAutoProperties(void) const {
        return _autoproperties;
    }
    inline Workspaces *getWorkspaces(void) const { return _workspaces; }
    inline KeyGrabber *getKeyGrabber(void) const { return _keygrabber; }
#ifdef HARBOUR
    inline Harbour *getHarbour(void) const { return _harbour; }
#endif //HARBOUR

    inline const std::string &getRestartCommand(void) const { return _restart_command; }
    inline bool isStartup(void) const { return _startup; }

    // list iterators
    inline std::list<PWinObj*>::iterator mru_begin(void) { return _mru_list.begin(); }
    inline std::list<PWinObj*>::reverse_iterator mru_rbegin(void) { return _mru_list.rbegin(); }
    inline std::list<PWinObj*>::iterator mru_end(void) { return _mru_list.end(); }
    inline std::list<PWinObj*>::reverse_iterator mru_rend(void) { return _mru_list.rend(); }

    // menus
#ifdef MENUS
    inline PMenu* getMenu(const std::string &name) {
        std::map<std::string, PMenu*>::iterator it = _menu_map.find(name);
        return (it != _menu_map.end()) ? it->second : 0;
    }
#endif // MENUS

    // adds
    inline void addToFrameList(Frame *frame) {
        if (frame) {
            _mru_list.remove(frame);
            _mru_list.push_front(frame);
        }
    }

    // removes
    void removeFromFrameList(Frame *frame);

    PWinObj *findPWinObj(Window win);

    // If a window unmaps and has transients lets unmap them too!
    // true = hide | false = unhide
    void findFamily(std::list<Client*> &client_list, Window win);
    void findTransientsToMapOrUnmap(Window win, bool hide);
    void familyRaiseLower(Client *client, bool raise);

    Frame* findGroup(AutoProperty *property);

    inline bool isAllowGrouping(void) const { return _allow_grouping; }
    inline void setStateGlobalGrouping(StateAction sa) {
        if (ActionUtil::needToggle(sa, _allow_grouping)) {
            _allow_grouping = !_allow_grouping;
        }
    }

    void attachMarked(Frame *frame);
    void attachInNextPrevFrame(Client* client, bool frame, bool next);

    Frame* getNextFrame(Frame* frame, bool mapped, uint mask = 0);
    Frame* getPrevFrame(Frame* frame, bool mapped, uint mask = 0);

    void findWOAndFocus(PWinObj *wo);

#ifdef MENUS
    void hideAllMenus(void);
#endif // MENUS

    inline CmdDialog *getCmdDialog(void) { return _cmd_dialog; }
    inline SearchDialog *getSearchDialog(void) { return _search_dialog; }
    inline StatusWindow *getStatusWindow(void) { return _status_window; }
    WorkspaceIndicator *getWorkspaceIndicator(void) { return _workspace_indicator; }

    // Extended Window Manager hints function prototypes
    void setEwmhSupported(void);
    void setEwmhActiveWindow(Window w);
    void setDesktopNames(void);

    // public event handlers used when doing grabbed actions
    void handleKeyEvent(XKeyEvent *ev);
    void handleButtonPressEvent(XButtonEvent *ev);
    void handleButtonReleaseEvent(XButtonEvent *ev);

private:
    WindowManager(const std::string &command_line,
                  const std::string &config_file);
    WindowManager(const WindowManager &); // not implemented to ensure singleton
    ~WindowManager(void);
    
    bool setupDisplay(bool replace);
    void scanWindows(void);
    void execStartFile(void);

    void doReload(void);
    void doReloadConfig(void);
    void doReloadTheme(void);
    void doReloadMouse(void);
    void doReloadKeygrabber(bool force=false);
    void doReloadAutoproperties(void);
#ifdef HARBOUR
    void doReloadHarbour(void);
#endif // HARBOUR

    void cleanup(void);

    // screen edge related
    void screenEdgeCreate(void);
    void screenEdgeDestroy(void);
    void screenEdgeResize(void);
    void screenEdgeMapUnmap(void);

    void handleMapRequestEvent(XMapRequestEvent *ev);
    void handleUnmapEvent(XUnmapEvent *ev);
    void handleDestroyWindowEvent(XDestroyWindowEvent *ev);

    void handleConfigureRequestEvent(XConfigureRequestEvent *ev);
    void handleClientMessageEvent(XClientMessageEvent *ev);

    void handleColormapEvent(XColormapEvent *ev);
    void handlePropertyEvent(XPropertyEvent *ev);
    void handleMappingEvent(XMappingEvent *ev);
    void handleExposeEvent(XExposeEvent *ev);

    void handleMotionEvent(XMotionEvent *ev);

    void handleEnterNotify(XCrossingEvent *ev);
    void handleLeaveNotify(XCrossingEvent *ev);
    void handleFocusInEvent(XFocusChangeEvent *ev);
    void handleFocusOutEvent(XFocusChangeEvent *ev);

  void handleShapeEvent(XAnyEvent *ev);

#ifdef HAVE_XRANDR
    void handleXRandrEvent(XRRNotifyEvent *ev);
    void handleXRandrScreenChangeEvent(XRRScreenChangeNotifyEvent *ev);
    void handleXRandrCrtcChangeEvent(XRRCrtcChangeNotifyEvent *ev);
#endif // HAVE_XRANDR

#ifdef MENUS
    void createMenus(void);
    void doReloadMenus(void);
    void updateMenusStandalone(CfgParser::Entry *cfg_root);
    void deleteMenus(void);
#endif // MENUS

    void readDesktopNamesHint(void);

    // private methods for the hints
    void initHints(void);

    bool findGroupMatchProperty(Frame *frame, AutoProperty *property);
    Frame *findGroupMatch(AutoProperty *property);

private:
    PScreen *_screen;
    ScreenResources *_screen_resources;
    KeyGrabber *_keygrabber;
    Config *_config;
    ColorHandler *_color_handler;
    FontHandler *_font_handler;
    TextureHandler *_texture_handler;
    Theme *_theme;
    ActionHandler *_action_handler;
    AutoProperties *_autoproperties;
    Workspaces *_workspaces;
#ifdef HARBOUR
    Harbour *_harbour;
#endif // HARBOUR
    CmdDialog *_cmd_dialog;
    SearchDialog *_search_dialog;
    StatusWindow *_status_window;

  WorkspaceIndicator *_workspace_indicator; //!< Window popping up when switching workspace

  Timer<ActionPerformed> _timer_action;

#ifdef MENUS
    std::map <std::string, time_t> _menu_state; /**< Map of file mtime for all files touched by a configuration. */    
    std::map<std::string, PMenu*> _menu_map;

    static const char *MENU_NAMES_RESERVED[];
    static const unsigned int MENU_NAMES_RESERVED_COUNT;
#endif // MENUS

    std::string _command_line, _restart_command;
    bool _startup; //!< Indicates startup status.
    bool _shutdown; //!< Set to wheter we want to shutdown.
    bool _reload; //!< Set to wheter we want to reload.

    std::list<PWinObj*> _mru_list;

    bool _allow_grouping; //<! Flag turning grouping on/off.

    std::list<EdgeWO*> _screen_edge_list;
    HintWO *_hint_wo; /**< Hint window object, communicates EWMH hints. */
    RootWO *_root_wo; /**< Root window object, wrapper for root window. */

    // pointer for singleton pattern
    static WindowManager *_instance;
};

#endif // _WINDOWMANAGER_HH_
