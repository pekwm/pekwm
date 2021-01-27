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

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "Action.hh"
#include "AppCtrl.hh"
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

class WindowManager : public AppCtrl
{
public:
    static WindowManager *start(const std::string &config_file, bool replace);
    virtual ~WindowManager();

    void doEventLoop(void);

    // START - AppCtrl interface.
    virtual void reload(void) override { _reload = true; }
    virtual void restart(void) override { restart(""); }
    virtual void restart(std::string command) override;
    virtual void shutdown(void) { _shutdown = true; }
    // END - AppCtrl interface.

    inline bool shallRestart(void) const { return _restart; }
    inline const std::string &getRestartCommand(void) const {
        return _restart_command;
    }

    // public event handlers used when doing grabbed actions
    void handleKeyEvent(XKeyEvent *ev);
    void handleButtonPressEvent(XButtonEvent *ev);
    void handleButtonReleaseEvent(XButtonEvent *ev);

private:
    WindowManager(void);

    void setupDisplay(Display* dpy);
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

    void handleKeyEventAction(XKeyEvent *ev, ActionEvent *ae, PWinObj *wo,
                              PWinObj *wo_orig);

    void readDesktopNamesHint(void);

    // private methods for the hints
    void initHints(void);

    Client *createClient(Window window, bool is_new);

private:
    bool _shutdown; //!< Set to wheter we want to shutdown.
    bool _reload; //!< Set to wheter we want to reload.
    bool _restart;
    std::string _restart_command;

    EdgeWO *_screen_edges[4];
};
