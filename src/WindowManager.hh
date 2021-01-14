//
// WindowManager.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// windowmanager.hh for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _WINDOWMANAGER_HH_
#define _WINDOWMANAGER_HH_

#include "config.h"

#include "pekwm.hh"
#include "Action.hh"
#include "ManagerWindows.hh"

#include <algorithm>
#include <map>

class ActionHandler;
class Config;
class FontHandler;
class TextureHandler;
class Workspaces;

class ScreenResources;
class PWinObj;
class PDecor;
class Frame;
class Client;
class ClassHint;

class AutoProperty; // for findGroup
class CmdDialog;
class SearchDialog;
class StatusWindow;

class KeyGrabber;
class Harbour;

class WindowManager
{
public:
    static WindowManager *start(const std::string &config_file, bool replace);
    static void destroy(void);
    inline static WindowManager *instance(void) { return _instance; }

    void doEventLoop(void);

    //! @brief Sets reload status, will reload from main loop.
    void reload(void) { _reload = true; }
    void restart(std::string command = "");
    //! @brief Sets shutdown status, will shutdown from main loop.
    void shutdown(void) { _shutdown = true; }

    // get "base" classes
    inline Config *getConfig(void) const { return _config; }
    inline ActionHandler *getActionHandler(void)
    const { return _action_handler; }
    inline KeyGrabber *getKeyGrabber(void) const { return _keygrabber; }
    inline Harbour *getHarbour(void) const { return _harbour; }

    inline bool shallRestart(void) const { return _restart; }
    inline const std::string &getRestartCommand(void) const { return _restart_command; }

    void familyRaiseLower(Client *client, bool raise);

    void attachMarked(Frame *frame);
    void attachInNextPrevFrame(Client* client, bool frame, bool next);

    Frame* getNextFrame(Frame* frame, bool mapped, uint mask = 0);
    Frame* getPrevFrame(Frame* frame, bool mapped, uint mask = 0);

    void findWOAndFocus(PWinObj *wo);

    inline CmdDialog *getCmdDialog(void) { return _cmd_dialog; }
    inline SearchDialog *getSearchDialog(void) { return _search_dialog; }
    inline StatusWindow *getStatusWindow(void) { return _status_window; }

    // public event handlers used when doing grabbed actions
    void handleKeyEvent(XKeyEvent *ev);
    void handleButtonPressEvent(XButtonEvent *ev);
    void handleButtonReleaseEvent(XButtonEvent *ev);

private:
    WindowManager(const std::string &config_file);
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
    void doReloadHarbour(void);

    void cleanup(void);

    // screen edge related
    void screenEdgeCreate(void);
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

    void handleKeyEventAction(XKeyEvent *ev, ActionEvent *ae, PWinObj *wo, PWinObj *wo_orig);

    void readDesktopNamesHint(void);

    // private methods for the hints
    void initHints(void);

    Client *createClient(Window window, bool is_new);

private:
    KeyGrabber *_keygrabber;
    Config *_config;
    ActionHandler *_action_handler;
    Harbour *_harbour;
    CmdDialog *_cmd_dialog;
    SearchDialog *_search_dialog;
    StatusWindow *_status_window;

    bool _shutdown; //!< Set to wheter we want to shutdown.
    bool _reload; //!< Set to wheter we want to reload.
    bool _restart;
    std::string _restart_command;

    EdgeWO *_screen_edges[4];
    HintWO *_hint_wo; /**< Hint window object, communicates EWMH hints. */
    RootWO *_root_wo; /**< Root window object, wrapper for root window. */

    // pointer for singleton pattern
    static WindowManager *_instance;
};

#endif // _WINDOWMANAGER_HH_
