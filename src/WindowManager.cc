//
// WindowManager.cc for pekwm
// Copyright © 2002-2015 the pekwm development team
//
// windowmanager.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PWinObj.hh"
#include "PDecor.hh"
#include "Frame.hh"
#include "Client.hh"
#include "WindowManager.hh"

#include "ScreenResources.hh"
#include "ActionHandler.hh"
#include "AutoProperties.hh"
#include "Config.hh"
#include "Theme.hh"
#include "PFont.hh"
#include "PTexture.hh"
#include "FontHandler.hh"
#include "TextureHandler.hh"
#include "Workspaces.hh"
#include "Util.hh"
#include "x11.hh"

#include "RegexString.hh"

#include "KeyGrabber.hh"
#include "MenuHandler.hh"
#include "Harbour.hh"
#include "DockApp.hh"
#include "CmdDialog.hh"
#include "SearchDialog.hh"
#include "StatusWindow.hh"
#include "WorkspaceIndicator.hh"

#include <iostream>
#include <functional>
#include <memory>
#include <cassert>

extern "C" {
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif // HAVE_XRANDR
}

using std::cout;
using std::cerr;
using std::endl;
using std::find;
using std::map;
using std::mem_fun;
using std::string;
using std::vector;
using std::wstring;

// Static initializers
WindowManager *WindowManager::_instance = 0;

extern "C" {

    static bool is_signal_hup = false;
    static bool is_signal_int_term = false;
    static bool is_signal_alrm = false;

    /**
      * Signal handler setting signal flags.
      */
    static void
    sigHandler(int signal)
    {
        switch (signal) {
        case SIGHUP:
            is_signal_hup = true;
            break;
        case SIGINT:
        case SIGTERM:
            is_signal_int_term = true;
            break;
        case SIGCHLD:
            wait(0);
            break;
        case SIGALRM:
            // Do nothing, just used to break out of waiting
            is_signal_alrm = true;
            break;
        }
    }
    
} // extern "C"


// WindowManager

/**
 * Create window manager instance and run main routine.
 */
WindowManager*
WindowManager::start(const std::string &config_file, bool replace)
{
    if (_instance) {
        delete _instance;
    }

    // Setup window manager
    _instance = new WindowManager(config_file);

    if (_instance->setupDisplay(replace)) {
        _instance->scanWindows();
        Frame::resetFrameIDs();

        _instance->_root_wo->setEwmhDesktopNames();
        _instance->_root_wo->setEwmhDesktopLayout();
    
        // add all frames to the MRU list
        _instance->_mru.resize(Frame::frame_size());
        copy(Frame::frame_begin(), Frame::frame_end(),
             _instance->_mru.begin());

        _instance->execStartFile();
    } else {
        delete _instance;
        _instance = 0;
    }

    return _instance;
}

/**
 * Destroy current running window manager.
 */
void
WindowManager::destroy(void)
{
    delete _instance;
    _instance = 0;
}

//! @brief Constructor for WindowManager class
WindowManager::WindowManager(const std::string &config_file) :
        _screen_resources(0),
        _keygrabber(0),
        _config(0),
        _font_handler(0), _texture_handler(0),
        _theme(0), _action_handler(0),
        _autoproperties(0),
        _harbour(0),
        _cmd_dialog(0), _search_dialog(0),
        _status_window(0), _workspace_indicator(0),
        _startup(false), _shutdown(false),
        _reload(false), _restart(false),
        _allow_grouping(true), _hint_wo(0), _root_wo(0)
{
    _screen_edges[0] = 0;
    _screen_edges[1] = 0;
    _screen_edges[2] = 0;
    _screen_edges[3] = 0;

    struct sigaction act;

    // Set up the signal handlers.
    act.sa_handler = sigHandler;
    act.sa_mask = sigset_t();
    act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;

    sigaction(SIGTERM, &act, 0);
    sigaction(SIGINT, &act, 0);
    sigaction(SIGHUP, &act, 0);
    sigaction(SIGCHLD, &act, 0);
    sigaction(SIGALRM, &act, 0);

    // construct
    _config = new Config();
    _config->load(config_file);
    _config->loadMouseConfig(_config->getMouseConfigFile());
}

//! @brief WindowManager destructor
WindowManager::~WindowManager(void)
{
    cleanup();

    delete _cmd_dialog;
    delete _search_dialog;
    delete _status_window;
    delete _workspace_indicator;
    MenuHandler::deleteMenus();
    delete _harbour;
    delete _root_wo;
    delete _hint_wo;
    delete _action_handler;
    delete _autoproperties;
    delete _keygrabber;
    delete _config;
    delete _theme;
    delete _font_handler;
    delete _texture_handler;
    delete _screen_resources;

    X11::destruct();
}

//! @brief Checks if the start file is executable then execs it.
void
WindowManager::execStartFile(void)
{
    string start_file(_config->getStartFile());

    bool exec = Util::isExecutable(start_file);
    if (! exec) {
        start_file = SYSCONFDIR "/start";
        exec = Util::isExecutable(start_file);
    }

    if (exec) {
        Util::forkExec(start_file);
    }
}

//! @brief Cleans up, Maps windows etc.
void
WindowManager::cleanup(void)
{
    // update all nonactive clients properties
    vector<Frame*>::const_iterator it_f(Frame::frame_begin());
    for (; it_f != Frame::frame_end(); ++it_f) {
        (*it_f)->updateInactiveChildInfo();
    }

    if (_harbour) {
        _harbour->removeAllDockApps();
    }

    // To preserve stacking order when destroying the frames, we go through
    // the PWinObj list from the Workspaces and put all Frames into our own
    // list, then we delete the frames in order.
    vector<Frame*> frame_list;
    Workspaces::iterator it_w(Workspaces::begin());
    for (; it_w != Workspaces::end(); ++it_w) {
        if ((*it_w)->getType() == PWinObj::WO_FRAME) {
            frame_list.push_back(static_cast<Frame*>(*it_w));
        }
    }

    // Delete all Frames. This reparents the Clients.
    for (it_f = frame_list.begin(); it_f != frame_list.end(); ++it_f) {
        delete *it_f;
    }

    // Delete all Clients.
    while (Client::client_begin() != Client::client_end())
        delete *Client::client_begin();

    if (_keygrabber) {
        _keygrabber->ungrabKeys(X11::getRoot());
    }

    // destroy screen edge
    for (int i=0; i < 4; ++i) {
        Workspaces::remove(_screen_edges[i]);
        delete _screen_edges[i];
        _screen_edges[i]=0;
    }

    XInstallColormap(X11::getDpy(), X11::getColormap());
    X11::setInputFocus(PointerRoot);
}

/**
 * Setup display and claim resources.
 */
bool
WindowManager::setupDisplay(bool replace)
{
    Display *dpy = XOpenDisplay(0);
    if (! dpy) {
        cerr << "Can not open display!" << endl
             << "Your DISPLAY variable currently is set to: "
             << getenv("DISPLAY") << endl;
        exit(1);
    }

    // Setup screen, init atoms and claim the display.
    X11::init(dpy, _config->isHonourRandr());

    try {
        // Create hint window _before_ root window.
        _hint_wo = new HintWO(X11::getRoot(), replace);
    } catch (string &ex) {
        return false;
    }

    // Create root PWinObj
    _root_wo = new RootWO(X11::getRoot());
    PWinObj::setRootPWinObj(_root_wo);

    _font_handler = new FontHandler();
    _texture_handler = new TextureHandler();
    _action_handler = new ActionHandler();

    // load colors, fonts
    _screen_resources = new ScreenResources();
    _theme = new Theme;

    _autoproperties = new AutoProperties();
    _autoproperties->load();

    Workspaces::setSize(_config->getWorkspaces());
    Workspaces::setPerRow(_config->getWorkspacesPerRow());

    _harbour = new Harbour;

    MenuHandler::createMenus(_theme);

    _cmd_dialog = new CmdDialog(_theme);
    _search_dialog = new SearchDialog(_theme);
    _status_window = new StatusWindow(_theme);
    _workspace_indicator = new WorkspaceIndicator(_theme, _timer_action);

    XDefineCursor(dpy, X11::getRoot(),
                  _screen_resources->getCursor(ScreenResources::CURSOR_ARROW));

#ifdef HAVE_XRANDR
    XRRSelectInput(dpy, X11::getRoot(), RRScreenChangeNotifyMask);
#endif // HAVE_XRANDR

    _keygrabber = new KeyGrabber;
    _keygrabber->load(_config->getKeyFile());
    _keygrabber->grabKeys(X11::getRoot());

    // Create screen edge windows
    screenEdgeCreate();
    screenEdgeMapUnmap();

    return true;
}

//! @brief Goes through the window and creates Clients/DockApps.
void
WindowManager::scanWindows(void)
{
    if (_startup) { // only done once when we start
        return;
    }
    
    uint num_wins;
    Window d_win1, d_win2, *wins;

    // Lets create a list of windows on the display
    XQueryTree(X11::getDpy(), X11::getRoot(),
               &d_win1, &d_win2, &wins, &num_wins);
    vector<Window> win_list(wins, wins + num_wins);
    XFree(wins);

    vector<Window>::iterator it(win_list.begin());

    // We filter out all windows with the the IconWindowHint
    // set not pointing to themselves, making DockApps
    // work as they are supposed to.
    for (; it != win_list.end(); ++it) {
        if (*it == None) {
            continue;
        }

        XWMHints *wm_hints = XGetWMHints(X11::getDpy(), *it);
        if (wm_hints) {
            if ((wm_hints->flags&IconWindowHint) &&
                    (wm_hints->icon_window != *it)) {
                vector<Window>::iterator i_it(find(win_list.begin(), win_list.end(), wm_hints->icon_window));
                if (i_it != win_list.end())
                    *i_it = None;
            }
            XFree(wm_hints);
        }
    }

    Client *client;
    for (it = win_list.begin(); it != win_list.end(); ++it) {
        if (*it == None) {
            continue;
        }
        
        XWindowAttributes attr;
        XGetWindowAttributes(X11::getDpy(), *it, &attr);
        if (! attr.override_redirect && attr.map_state != IsUnmapped) {
            XWMHints *wm_hints = XGetWMHints(X11::getDpy(), *it);
            if (wm_hints) {
                if ((wm_hints->flags&StateHint) &&
                        (wm_hints->initial_state == WithdrawnState)) {
                    _harbour->addDockApp(new DockApp(*it));
                } else {
                    client = new Client(*it);
                    if (! client->isAlive()) {
                        delete client;
                    }

                }
                XFree(wm_hints);
            } else {
                client = new Client(*it);
                if (! client->isAlive())
                    delete client;
            }
        }
    }

    // Try to focus the ontop window, if no window we give root focus
    PWinObj *wo = Workspaces::getTopWO(PWinObj::WO_FRAME);
    if (wo && wo->isMapped()) {
        wo->giveInputFocus();
    } else {
        _root_wo->giveInputFocus();
    }

    // Try to find transients for all clients, on restarts ordering might
    // not be correct.
    vector<Client*>::const_iterator it_client = Client::client_begin();
    for (; it_client != Client::client_end(); ++it_client) {
        if ((*it_client)->isTransient() && ! (*it_client)->getTransientForClient()) {
            (*it_client)->findAndRaiseIfTransient();
        }
    }

    // We won't be needing these anymore until next restart
    _autoproperties->removeApplyOnStart();

    _startup = true;
}

//! @brief Creates and places screen edge
void
WindowManager::screenEdgeCreate(void)
{
    bool indent = Config::instance()->getScreenEdgeIndent();

    _screen_edges[0] = new EdgeWO(X11::getRoot(), SCREEN_EDGE_LEFT,
                                  indent && (_config->getScreenEdgeSize(SCREEN_EDGE_LEFT) > 0));
    _screen_edges[1] = new EdgeWO(X11::getRoot(), SCREEN_EDGE_RIGHT,
                                  indent && (_config->getScreenEdgeSize(SCREEN_EDGE_RIGHT) > 0));
    _screen_edges[2] = new EdgeWO(X11::getRoot(), SCREEN_EDGE_TOP,
                                  indent && (_config->getScreenEdgeSize(SCREEN_EDGE_TOP) > 0));
    _screen_edges[3] = new EdgeWO(X11::getRoot(), SCREEN_EDGE_BOTTOM,
                                  indent && (_config->getScreenEdgeSize(SCREEN_EDGE_BOTTOM) > 0));

    // make sure the edge stays ontop
    for (int i=0; i < 4; ++i) {
        Workspaces::insert(_screen_edges[i]);
    }

    screenEdgeResize();
}

//! @brief
void
WindowManager::screenEdgeResize(void)
{
    uint l_size = std::max(_config->getScreenEdgeSize(SCREEN_EDGE_LEFT), 1);
    uint r_size = std::max(_config->getScreenEdgeSize(SCREEN_EDGE_RIGHT), 1);
    uint t_size = std::max(_config->getScreenEdgeSize(SCREEN_EDGE_TOP), 1);
    uint b_size = std::max(_config->getScreenEdgeSize(SCREEN_EDGE_BOTTOM), 1);

    // Left edge
    _screen_edges[0]->moveResize(0, 0, l_size, X11::getHeight());

    // Right edge
    _screen_edges[1]->moveResize(X11::getWidth() - r_size, 0, r_size, X11::getHeight());

    // Top edge
    _screen_edges[2]->moveResize(l_size, 0, X11::getWidth() - l_size - r_size, t_size);

    // Bottom edge
    _screen_edges[3]->moveResize(l_size, X11::getHeight() - b_size, X11::getWidth() - l_size - r_size, b_size);

    for (int i=0; i<4; ++i) {
        _screen_edges[i]->configureStrut(_config->getScreenEdgeIndent()
                        && _config->getScreenEdgeSize(_screen_edges[i]->getEdge()) > 0);
    }

    X11::updateStrut();
}

void
WindowManager::screenEdgeMapUnmap(void)
{
    EdgeWO* edge;
    for (int i=0; i<4; ++i) {
        edge = _screen_edges[i];
        if (_config->getScreenEdgeSize(edge->getEdge()) > 0
            && _config->getEdgeListFromPosition(edge->getEdge())->size() > 0) {
            edge->mapWindow();
        } else {
            edge->unmapWindow();
        }
    }
}

//! @brief Reloads configuration and updates states.
void
WindowManager::doReload(void)
{
    doReloadConfig();
    doReloadTheme();
    doReloadMouse();
    doReloadKeygrabber();
    doReloadAutoproperties();

    MenuHandler::reloadMenus();
    doReloadHarbour();

    _root_wo->setEwmhDesktopNames();
    _root_wo->setEwmhDesktopLayout();

    _reload = false;
}

/**
 * Reload main config file.
 */
void
WindowManager::doReloadConfig(void)
{
    // If any of these changes, re-fetch of all names is required
    bool old_client_unique_name = _config->getClientUniqueName();
    string old_client_unique_name_pre = _config->getClientUniqueNamePre();
    string old_client_unique_name_post = _config->getClientUniqueNamePost();

    // Reload configuration
    if (! _config->load(_config->getConfigFile())) {
        return;
    }

    // Update what might have changed in the cfg touching the hints
    Workspaces::setSize(_config->getWorkspaces());
    Workspaces::setPerRow(_config->getWorkspacesPerRow());
    Workspaces::setNames();

    // Update the ClientUniqueNames if needed
    if ((old_client_unique_name != _config->getClientUniqueName()) ||
        (old_client_unique_name_pre != _config->getClientUniqueNamePre()) ||
        (old_client_unique_name_post != _config->getClientUniqueNamePost())) {
        for_each(Client::client_begin(), Client::client_end(), mem_fun(&Client::readName));
    }

    // Resize the screen edge
    screenEdgeResize();
    screenEdgeMapUnmap();

    X11::updateStrut();
}

/**
 * Reload theme file and update decorations.
 */
void
WindowManager::doReloadTheme(void)
{
    // Reload the theme
    if (! _theme->load(_config->getThemeFile())) {
        return;
    }

    // Reload the themes on all decors
    for_each(PDecor::pdecor_begin(), PDecor::pdecor_end(), mem_fun(&PDecor::loadDecor));
}

/**
 * Reload mouse configuration and re-grab buttons on all windows.
 */
void
WindowManager::doReloadMouse(void)
{
    if (! _config->loadMouseConfig(_config->getMouseConfigFile())) {
        return;
    }

    for_each(Client::client_begin(), Client::client_end(), mem_fun(&Client::grabButtons));
}

/**
 * Reload keygrabber configuration and re-grab keys on all windows.
 */
void
WindowManager::doReloadKeygrabber(bool force)
{
    // Reload the keygrabber
    if (! _keygrabber->load(_config->getKeyFile(), force)) {
        return;
    }

    _keygrabber->ungrabKeys(X11::getRoot());
    _keygrabber->grabKeys(X11::getRoot());

    // Regrab keys and buttons
    vector<Client*>::const_iterator c_it(Client::client_begin());
    for (; c_it != Client::client_end(); ++c_it) {
        (*c_it)->grabButtons();
        _keygrabber->ungrabKeys((*c_it)->getWindow());
        _keygrabber->grabKeys((*c_it)->getWindow());
    }
}

/**
 * Reload autoproperties.
 */
void
WindowManager::doReloadAutoproperties(void)
{
    if (! _autoproperties->load()) {
        return;
    }

    // NOTE: we need to load autoproperties after decor have been updated
    // as otherwise old theme data pointer will be used and sig 11 pekwm.
    vector<Client*>::const_iterator it_c(Client::client_begin());
    for (; it_c != Client::client_end(); ++it_c) {
        (*it_c)->readAutoprops(APPLY_ON_RELOAD);
    }

    vector<Frame*>::const_iterator it_f(Frame::frame_begin());
    for (; it_f != Frame::frame_end(); ++it_f) {
        (*it_f)->readAutoprops(APPLY_ON_RELOAD);
    }
}

/**
 * Reload harbour configuration.
 */
void
WindowManager::doReloadHarbour(void)
{
    _harbour->loadTheme();
    _harbour->rearrange();
    _harbour->restack();
    _harbour->updateHarbourSize();
}

//! @brief Exit pekwm and restart with the command command
void
WindowManager::restart(std::string command)
{
    if (! command.empty()) {
        _restart_command = command;
    }
    _restart = true;
    _shutdown = true;
}

// Event handling routins beneath this =====================================

void
WindowManager::doEventLoop(void)
{
    XEvent ev;
    Timer<ActionPerformed>::timed_event_list events;

    while (! _shutdown && ! is_signal_int_term) {
        // Handle timeouts
        if (is_signal_alrm) {
            is_signal_alrm = false;

            if (_timer_action.getTimedOut(events)) {
                Timer<ActionPerformed>::timed_event_list_it it(events.begin());
                for (; it != events.end(); ++it) {
                    _action_handler->handleAction((*it)->data);
                }
                events.clear();
            }
        }

        // Reload if requested
        if (is_signal_hup || _reload) {
            is_signal_hup = false;
            doReload();
        }

        // Get next event, drop event handling if none was given
        if (X11::getNextEvent(ev)) {
            switch (ev.type) {
            case MapRequest:
                handleMapRequestEvent(&ev.xmaprequest);
                break;
            case UnmapNotify:
                handleUnmapEvent(&ev.xunmap);
                break;
            case DestroyNotify:
                handleDestroyWindowEvent(&ev.xdestroywindow);
                break;
                
            case ConfigureRequest:
                handleConfigureRequestEvent(&ev.xconfigurerequest);
                break;
            case ClientMessage:
                handleClientMessageEvent(&ev.xclient);
                break;
            case ColormapNotify:
                handleColormapEvent(&ev.xcolormap);
                break;
            case PropertyNotify:
                X11::setLastEventTime(ev.xproperty.time);
                handlePropertyEvent(&ev.xproperty);
                break;
            case MappingNotify:
                handleMappingEvent(&ev.xmapping);
                break;
            case Expose:
                handleExposeEvent(&ev.xexpose);
                break;
      
            case KeyPress:
            case KeyRelease:
                X11::setLastEventTime(ev.xkey.time);
                handleKeyEvent(&ev.xkey);
                break;

            case ButtonPress:
                X11::setLastEventTime(ev.xbutton.time);
                handleButtonPressEvent(&ev.xbutton);
                break;
            case ButtonRelease:
                X11::setLastEventTime(ev.xbutton.time);
                handleButtonReleaseEvent(&ev.xbutton);
                break;

            case MotionNotify:
                X11::setLastEventTime(ev.xmotion.time);
                handleMotionEvent(&ev.xmotion);
                break;

            case EnterNotify:
                X11::setLastEventTime(ev.xcrossing.time);
                handleEnterNotify(&ev.xcrossing);
                break;
            case LeaveNotify:
                X11::setLastEventTime(ev.xcrossing.time);
                handleLeaveNotify(&ev.xcrossing);
                break;
            case FocusIn:
                handleFocusInEvent(&ev.xfocus);
                break;
            case FocusOut:
                break;

            case SelectionClear:
                // Another window
                cerr << " *** INFO: Being replaced by another WM" << endl;
                _shutdown = true;
                break;

            default:
#ifdef HAVE_SHAPE
                if (X11::hasExtensionShape() && ev.type == X11::getEventShape()) {
                    XShapeEvent *sev = reinterpret_cast<XShapeEvent*>(&ev);
                    X11::setLastEventTime(sev->time);
                    Client *client = Client::findClient(sev->window);
                    if (client) {
                        client->handleShapeEvent(sev);
                    }
                }
#endif // HAVE_SHAPE
#ifdef HAVE_XRANDR
                if (X11::hasExtensionXRandr()) {
                    if (ev.type == X11::getEventXRandr() + RRScreenChangeNotify) {
                        XRRScreenChangeNotifyEvent *scr_ev =
                                reinterpret_cast<XRRScreenChangeNotifyEvent*>(&ev);

                        if (scr_ev->rotation == RR_Rotate_90 || scr_ev->rotation == RR_Rotate_270) {
                            X11::updateGeometry(scr_ev->height, scr_ev->width);
                        } else {
                            X11::updateGeometry(scr_ev->width, scr_ev->height);
                        }

                        _harbour->updateGeometry();
                        screenEdgeResize();

                        // Make sure windows are visible after resize
                        vector<PDecor*>::const_iterator it(PDecor::pdecor_begin());
                        for (; it != PDecor::pdecor_end(); ++it) {
                            Workspaces::placeWoInsideScreen(*it);
                        }
                    }
                }
#endif // HAVE_XRANDR
                break;
            }
        }
    }
}

//! @brief Handle XKeyEvents
void
WindowManager::handleKeyEvent(XKeyEvent *ev)
{
    ActionEvent *ae = 0;
    PWinObj *wo, *wo_orig;
    wo = wo_orig = PWinObj::getFocusedPWinObj();
    PWinObj::Type type = (wo) ? wo->getType() : PWinObj::WO_SCREEN_ROOT;

    switch (type) {
    case PWinObj::WO_CLIENT:
    case PWinObj::WO_FRAME:
    case PWinObj::WO_SCREEN_ROOT:
    case PWinObj::WO_MENU:
        if (ev->type == KeyPress) {
            ae = _keygrabber->findAction(ev, type);
        }
        break;
    case PWinObj::WO_CMD_DIALOG:
    case PWinObj::WO_SEARCH_DIALOG:
        if (ev->type == KeyPress) {
            ae = wo->handleKeyPress(ev);
        } else {
            ae = wo->handleKeyRelease(ev);
        }
        wo = static_cast<InputDialog*>(wo)->getWORef();
        break;
    default:
        if (wo) {
            if (ev->type == KeyPress) {
                ae = wo->handleKeyPress(ev);
            } else {
                ae = wo->handleKeyRelease(ev);
            }
        }
        break;
    }

    if (ae) {
        // HACK: Always close CmdDialog and SearchDialog before actions
        if (wo_orig
            && (wo_orig->getType() == PWinObj::WO_CMD_DIALOG
                || wo_orig->getType() == PWinObj::WO_SEARCH_DIALOG)) {
            ::Action close_action;
            ActionEvent close_ae;
            
            close_ae.action_list.push_back(close_action);
            close_ae.action_list.back().setAction(ACTION_CLOSE);

            ActionPerformed close_ap(wo_orig, close_ae);
            _action_handler->handleAction(close_ap);
        }

        ActionPerformed ap(wo, *ae);
        ap.type = ev->type;
        ap.event.key = ev;
        _action_handler->handleAction(ap);
    }

    // flush Enter events caused by keygrabbing
    XEvent e;
    while (X11::checkTypedEvent(EnterNotify, &e)) {
        if (! e.xcrossing.send_event) {
            X11::setLastEventTime(e.xcrossing.time);
        }
    }
}

//! @brief
void
WindowManager::handleButtonPressEvent(XButtonEvent *ev)
{
    // Clear event queue
    while (X11::checkTypedEvent(ButtonPress, (XEvent *) ev)) {
        if (! ev->send_event) {
            X11::setLastEventTime(ev->time);
        }
    }

    ActionEvent *ae = 0;
    PWinObj *wo = 0;

    wo = PWinObj::findPWinObj(ev->window);
    if (wo) {
        ae = wo->handleButtonPress(ev);

        if (wo->getType() == PWinObj::WO_FRAME) {
            // this is done so that clicking the titlebar executes action on
            // the client clicked on, doesn't apply when subwindow is set (meaning
            // a titlebar button beeing pressed)
            if ((ev->subwindow == None)
                    && (ev->window == static_cast<Frame*>(wo)->getTitleWindow())) {
                wo = static_cast<Frame*>(wo)->getChildFromPos(ev->x);
            } else {
                wo = static_cast<Frame*>(wo)->getActiveChild();
            }
        }
    }

    if (ae) {
        ActionPerformed ap(wo, *ae);
        ap.type = ev->type;
        ap.event.button = ev;

        _action_handler->handleAction(ap);
    } else {
        DockApp *da = _harbour->findDockAppFromFrame(ev->window);
        if (da) {
            _harbour->handleButtonEvent(ev, da);
        }
    }
}

//! @brief
void
WindowManager::handleButtonReleaseEvent(XButtonEvent *ev)
{
    // Flush ButtonReleases
    while (X11::checkTypedEvent(ButtonRelease, (XEvent *) ev)) {
        if (! ev->send_event) {
            X11::setLastEventTime(ev->time);
        }
    }

    ActionEvent *ae = 0;
    PWinObj *wo = PWinObj::findPWinObj(ev->window);
    if (wo == _root_wo && ev->subwindow != None) {
        wo = PWinObj::findPWinObj(ev->subwindow);
    }

    if (wo) {
        // Kludge for the case that wo is freed by handleButtonRelease(.).
        PWinObj::Type wotype = wo->getType();
        ae = wo->handleButtonRelease(ev);

        if (wotype == PWinObj::WO_FRAME) {
            // this is done so that clicking the titlebar executes action on
            // the client clicked on, doesn't apply when subwindow is set (meaning
            // a titlebar button beeing pressed)
            if ((ev->subwindow == None)
                && (ev->window == static_cast<Frame*>(wo)->getTitleWindow())) {
                wo = static_cast<Frame*>(wo)->getChildFromPos(ev->x);
            } else {
                wo = static_cast<Frame*>(wo)->getActiveChild();
            }
        }

        if (ae) {
            ActionPerformed ap(wo, *ae);
            ap.type = ev->type;
            ap.event.button = ev;

            _action_handler->handleAction(ap);
        }
    } else {
        DockApp *da = _harbour->findDockAppFromFrame(ev->window);
        if (da) {
            _harbour->handleButtonEvent(ev, da);
        }
    }
}

void
WindowManager::handleConfigureRequestEvent(XConfigureRequestEvent *ev)
{
    Client *client = Client::findClientFromWindow(ev->window);

    if (client) {
        ((Frame*) client->getParent())->handleConfigureRequest(ev, client);

    } else {
        DockApp *da = _harbour->findDockApp(ev->window);
        if (da) {
            _harbour->handleConfigureRequestEvent(ev, da);
        } else {
            // Since this window isn't yet a client lets delegate
            // the configure request back to the window so it can use it.

            XWindowChanges wc;

            wc.x = ev->x;
            wc.y = ev->y;
            wc.width = ev->width;
            wc.height = ev->height;
            wc.sibling = ev->above;
            wc.stack_mode = ev->detail;

            XConfigureWindow(X11::getDpy(), ev->window, ev->value_mask, &wc);
        }
    }
}

/**
 * Handle motion event, match on event window expect when event window
 * is root and subwindow is set then also match on menus.
 */
void
WindowManager::handleMotionEvent(XMotionEvent *ev)
{
    ActionEvent *ae = 0;
    PWinObj *wo = PWinObj::findPWinObj(ev->window);
    if (wo == _root_wo && ev->subwindow != None) {
        wo = PWinObj::findPWinObj(ev->subwindow);
    }

    if (wo) {
        if (wo->getType() == PWinObj::WO_CLIENT) {
            ae = wo->getParent()->handleMotionEvent(ev);

        } else if (wo->getType() == PWinObj::WO_FRAME) {
            ae = wo->handleMotionEvent(ev);

            // this is done so that clicking the titlebar executes action on
            // the client clicked on
            if (ev->subwindow != None) {
                wo = static_cast<Frame*>(wo)->getActiveChild();
            } else {
                wo = static_cast<Frame*>(wo)->getChildFromPos(ev->x);
            }

        } else {
            ae = wo->handleMotionEvent(ev);
        }

        if (ae) {
            ActionPerformed ap(wo, *ae);
            ap.type = ev->type;
            ap.event.motion = ev;

            _action_handler->handleAction(ap);
        }
    } else {
        DockApp *da = _harbour->findDockAppFromFrame(ev->window);
        if (da) {
            _harbour->handleMotionNotifyEvent(ev, da);
        }
    }
}

//! @brief
void
WindowManager::handleMapRequestEvent(XMapRequestEvent *ev)
{
    PWinObj *wo = PWinObj::findPWinObj(ev->window);

    if (wo) {
        wo->handleMapRequest(ev);
    } else {
        XWindowAttributes attr;
        XGetWindowAttributes(X11::getDpy(), ev->window, &attr);
        if (! attr.override_redirect) {
            // We need to figure out whether or not this is a dockapp.
            XWMHints *wm_hints = XGetWMHints(X11::getDpy(), ev->window);
            if (wm_hints) {
                if ((wm_hints->flags&StateHint) &&
                        (wm_hints->initial_state == WithdrawnState)) {
                    _harbour->addDockApp(new DockApp(ev->window));
                    XFree(wm_hints);
                    return;
                }
                XFree(wm_hints);
            }
            Client *client = new Client(ev->window, true);
            if (! client->isAlive()) {
                delete client;
            }
        }
    }
}

//! @brief
void
WindowManager::handleUnmapEvent(XUnmapEvent *ev)
{
    PWinObj *wo = PWinObj::findPWinObj(ev->window);
    PWinObj *wo_search = 0;

    PWinObj::Type wo_type = PWinObj::WO_NO_TYPE;

    if (wo) {
        wo_type = wo->getType();
        if (wo_type == PWinObj::WO_CLIENT) {
            wo_search = wo->getParent();
        }

        wo->handleUnmapEvent(ev);

        if (wo == PWinObj::getFocusedPWinObj()) {
            PWinObj::setFocusedPWinObj(0);
        }
    } else {
        DockApp *da = _harbour->findDockApp(ev->window);
        if (da) {
            if (ev->window == ev->event) {
                _harbour->removeDockApp(da);
            }
        }
    }

    if (wo_type != PWinObj::WO_MENU 
        && wo_type != PWinObj::WO_CMD_DIALOG
        && wo_type != PWinObj::WO_SEARCH_DIALOG
        && ! PWinObj::getFocusedPWinObj()) {
        findWOAndFocus(wo_search);
    }
    Workspaces::layoutIfTiling();
}

void
WindowManager::handleDestroyWindowEvent(XDestroyWindowEvent *ev)
{
    Client *client = Client::findClientFromWindow(ev->window);

    if (client) {
        PWinObj *wo_search = client->getParent();
        client->handleDestroyEvent(ev);

        if (! PWinObj::getFocusedPWinObj()) {
            findWOAndFocus(wo_search);
        }
    } else {
        DockApp *da = _harbour->findDockApp(ev->window);
        if (da) {
            da->setAlive(false);
            _harbour->removeDockApp(da);
        }
    }
}

void
WindowManager::handleEnterNotify(XCrossingEvent *ev)
{
    // Clear event queue
    while (X11::checkTypedEvent(EnterNotify, (XEvent *) ev)) {
        if (! ev->send_event) {
            X11::setLastEventTime(ev->time);
        }
    }

    if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab)) {
        return;
    }

    PWinObj *wo = PWinObj::findPWinObj(ev->window);

    if (wo) {

        if (wo->getType() == PWinObj::WO_CLIENT) {
            wo = wo->getParent();
        }

        ActionEvent *ae = wo->handleEnterEvent(ev);

        if (ae) {
            ActionPerformed ap(wo, *ae);
            ap.type = ev->type;
            ap.event.crossing = ev;

            _action_handler->handleAction(ap);
        }
    }
}

void
WindowManager::handleLeaveNotify(XCrossingEvent *ev)
{
    // Clear event queue
    while (X11::checkTypedEvent(LeaveNotify, (XEvent *) ev)) {
        if (! ev->send_event) {
            X11::setLastEventTime(ev->time);
        }
    }

    if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab)) {
        return;
    }

    PWinObj *wo = PWinObj::findPWinObj(ev->window);

    if (wo) {
        ActionEvent *ae = wo->handleLeaveEvent(ev);

        if (ae) {
            ActionPerformed ap(wo, *ae);
            ap.type = ev->type;
            ap.event.crossing = ev;

            _action_handler->handleAction(ap);
        }
    }
}

//! @brief Handles FocusIn Events.
void
WindowManager::handleFocusInEvent(XFocusChangeEvent *ev)
{
    if (ev->mode == NotifyGrab
        || ev->mode == NotifyUngrab
        || ev->detail == NotifyVirtual) {
        return;
    }

    PWinObj *wo = PWinObj::findPWinObj(ev->window);
    if (wo) {
        // To simplify logic, changing client in frame wouldn't update the
        // focused window because wo != focused_wo woule be true.
        if (wo->getType() == PWinObj::WO_FRAME) {
            wo = static_cast<Frame*>(wo)->getActiveChild();
        }

        if (! wo->isFocusable() || ! wo->isMapped()) {
            findWOAndFocus(0);

        } else if (wo != PWinObj::getFocusedPWinObj()) {
            // If it's a valid FocusIn event with accepatable target lets flush
            // all EnterNotify and LeaveNotify as they can interfere with
            // focusing if Sloppy or Follow like focus model is used.
            XEvent e_flush;
            while (X11::checkTypedEvent(EnterNotify, &e_flush)) {
                if (! e_flush.xcrossing.send_event) {
                    X11::setLastEventTime(e_flush.xcrossing.time);
                }
            }
            while (X11::checkTypedEvent(LeaveNotify, &e_flush)) {
                if (! e_flush.xcrossing.send_event) {
                    X11::setLastEventTime(e_flush.xcrossing.time);
                }
            }

            PWinObj *focused_wo = PWinObj::getFocusedPWinObj(); // convenience

            // unfocus last window
            if (focused_wo) {
                if (focused_wo->getType() == PWinObj::WO_CLIENT) {
                    focused_wo->getParent()->setFocused(false);
                } else {
                    focused_wo->setFocused(false);
                }
                Workspaces::setLastFocused(Workspaces::getActive(), focused_wo);
            }

            PWinObj::setFocusedPWinObj(wo);

            if (wo->getType() == PWinObj::WO_CLIENT) {
                wo->getParent()->setFocused(true);
                _root_wo->setEwmhActiveWindow(wo->getWindow());

                // update the MRU list (except for skip focus windows, see #297)
                if (! static_cast<Client*>(wo)->isSkip(SKIP_FOCUS_TOGGLE)) {
                    addToMRUFront(static_cast<Frame*>(wo->getParent()));
                }
            } else {
                wo->setFocused(true);
                _root_wo->setEwmhActiveWindow(None);
            }
        }
    }
}

/**
 * Handle XClientMessageEvent events.
 *
 * @param ev Event to handle.
 */
void
WindowManager::handleClientMessageEvent(XClientMessageEvent *ev)
{
    if (ev->window == X11::getRoot()) {
        // root window messages

        if (ev->format == 32) {

            if (ev->message_type == X11::getAtom(NET_CURRENT_DESKTOP)) {
                Workspaces::setWorkspace(ev->data.l[0], true);

            } else if (ev->message_type ==
                       X11::getAtom(NET_NUMBER_OF_DESKTOPS)) {
                if (ev->data.l[0] > 0) {
                    Workspaces::setSize(ev->data.l[0]);
                }
            }
        }

    } else {
        // client messages

        Client *client = Client::findClientFromWindow(ev->window);
        if (client) {
            static_cast<Frame*>(client->getParent())->handleClientMessage(ev, client);
        }
    }
}

void
WindowManager::handleColormapEvent(XColormapEvent *ev)
{
    Client *client = Client::findClient(ev->window);
    if (client) {
        client =
            static_cast<Client*>(((Frame*) client->getParent())->getActiveChild());
        client->handleColormapChange(ev);
    }
}

//! @brief Handles XPropertyEvents
void
WindowManager::handlePropertyEvent(XPropertyEvent *ev)
{
    if (ev->window == X11::getRoot()) {
        if (ev->atom == X11::getAtom(NET_DESKTOP_NAMES)) {
            _root_wo->readEwmhDesktopNames();
            Workspaces::setNames();
        }

        return;
    }

    Client *client = Client::findClientFromWindow(ev->window);

    if (client) {
        ((Frame*) client->getParent())->handlePropertyChange(ev, client);
    }
}

//! @brief Handles XMappingEvent
void
WindowManager::handleMappingEvent(XMappingEvent *ev)
{
    if (ev->request == MappingKeyboard || ev->request == MappingModifier) {
        XRefreshKeyboardMapping(ev);
        X11::setLockKeys();
        InputDialog::reloadKeysymMap();
        doReloadKeygrabber(true);
    }
}

void
WindowManager::handleExposeEvent(XExposeEvent *ev)
{
    ActionEvent *ae = 0;

    PWinObj *wo = PWinObj::findPWinObj(ev->window);
    if (wo) {
        ae = wo->handleExposeEvent(ev);
    }

    if (ae) {
        ActionPerformed ap(wo, *ae);
        ap.type = ev->type;
        ap.event.expose = ev;

        _action_handler->handleAction(ap);
    }
}

// Event handling routines stop ============================================

//! @brief Searches for a PWinObj to focus, and then gives it input focus
void
WindowManager::findWOAndFocus(PWinObj *search)
{
    PWinObj *focus = 0;

    if (PWinObj::windowObjectExists(search) &&
            (search->isMapped()) && (search->isFocusable()))  {
        focus = search;
    }

    // search window object didn't exist, go through the MRU list
    if (! focus) {
        vector<Frame *>::const_iterator f_it = _mru.begin();
        for (; ! focus && f_it != _mru.end(); ++f_it) {
            if ((*f_it)->isMapped() && (*f_it)->isFocusable()) {
                focus = *f_it;
            }
        }
    }

    if (focus) {
        focus->giveInputFocus();
    }  else if (! PWinObj::getFocusedPWinObj()) {
        _root_wo->giveInputFocus();
        _root_wo->setEwmhActiveWindow(None);
    }
}

//! @brief Raises the client and all window having transient relationship with it
//! @param client Client part of the famliy
//! @param raise If true, raise frames, else lowers them
void
WindowManager::familyRaiseLower(Client *client, bool raise)
{
    Client *parent;
    Window win_search;
    if (! client->getTransientForClient()) {
        parent = client;
        win_search = client->getWindow();
    } else {
        parent = client->getTransientForClient();
        win_search = client->getTransientForClientWindow();
    }

    vector<Client*> client_list;
    Client::findFamilyFromWindow(client_list, win_search);

    if (parent) { // make sure parent gets underneath the children
        if (raise) {
            client_list.insert(client_list.begin(), parent);
        } else {
            client_list.push_back(parent);
        }
    }

    Frame *frame;
    vector<Client*>::const_iterator it(client_list.begin());
    for (; it != client_list.end(); ++it) {
        frame = dynamic_cast<Frame*>((*it)->getParent());
        if (frame) {
            if (raise) {
                frame->raise();
            } else {
                frame->lower();
            }
        }
    }
}

/**
 * Match Frame against autoproperty.
 */
bool
WindowManager::findGroupMatchProperty(Frame *frame, AutoProperty *property)
{
    if ((property->group_global
         || (property->isMask(AP_WORKSPACE)
             ? (frame->getWorkspace() == property->workspace
                && ! frame->isIconified())
             : frame->isMapped()))
        && (property->group_size == 0
            || signed(frame->size()) < property->group_size)
        && (((frame->getClassHint()->group.size() > 0)
             ? (frame->getClassHint()->group == property->group_name) : false)
            || AutoProperties::matchAutoClass(*frame->getClassHint(),
                                              static_cast<Property*>(property)))) {
        return true;
    }
    return false;
}

/**
 * Tries to find a Frame to autogroup with. This is called recursively
 * if workspace specific matching is on to avoid conflicts with the
 * global property.
 */
Frame*
WindowManager::findGroup(AutoProperty *property)
{
    if (! _allow_grouping) {
        return 0;
    }

    Frame *frame = 0;
    if (property->group_global && property->isMask(AP_WORKSPACE)) {
        bool group_global_orig = property->group_global;
        property->group_global= false;
        frame = findGroup(property);
        property->group_global= group_global_orig;
    }

    if (! frame) {
        frame = findGroupMatch(property);
    }

    return frame;
}

/**
 * Do matching against Frames searching for a suitable Frame for
 * grouping.
 */
Frame*
WindowManager::findGroupMatch(AutoProperty *property)
{
    Frame *frame = 0;

    // Matching against the focused window first if requested
    if (property->group_focused_first
        && PWinObj::isFocusedPWinObj(PWinObj::WO_CLIENT)) {
        Frame *fo_frame =
            static_cast<Frame*>(PWinObj::getFocusedPWinObj()->getParent());
        if (findGroupMatchProperty(fo_frame, property)) {
            frame = fo_frame;
        }
    }

    // Moving on to the rest of the frames.
    if (! frame) {
        vector<Frame*>::const_iterator it(Frame::frame_begin());
        for (; it != Frame::frame_end(); ++it) {
            if (findGroupMatchProperty(*it, property)) {
                frame = *it;
                break;
            }
        }
    }

    return frame;
}

//! @brief Attaches all marked clients to frame
void
WindowManager::attachMarked(Frame *frame)
{
    vector<Client*>::const_iterator it(Client::client_begin());
    for (; it != Client::client_end(); ++it) {
        if ((*it)->isMarked()) {
            if ((*it)->getParent() != frame) {
                ((Frame*) (*it)->getParent())->removeChild(*it);
                (*it)->setWorkspace(frame->getWorkspace());
                frame->addChild(*it);
            }
            (*it)->setStateMarked(STATE_UNSET);
        }
    }
}

//! @brief Attaches the Client/Frame into the Next/Prev Frame
void
WindowManager::attachInNextPrevFrame(Client *client, bool frame, bool next)
{
    if (! client)
        return;

    Frame *new_frame;

    if (next) {
        new_frame =
            getNextFrame((Frame*) client->getParent(), true, SKIP_FOCUS_TOGGLE);
    } else {
        new_frame =
            getPrevFrame((Frame*) client->getParent(), true, SKIP_FOCUS_TOGGLE);
    }

    if (new_frame) { // we found a frame
        if (frame) {
            new_frame->addDecor((Frame*) client->getParent());
        } else {
            client->getParent()->setFocused(false);
            ((Frame*) client->getParent())->removeChild(client);
            new_frame->addChild(client);
            new_frame->activateChild(client);
            new_frame->giveInputFocus();
        }
    }
}

//! @brief Finds the next frame in the list
//!
//! @param frame Frame to search from
//! @param mapped Match only agains mapped frames
//! @param mask Defaults to 0
Frame*
WindowManager::getNextFrame(Frame* frame, bool mapped, uint mask)
{
    if (! frame || (Frame::frame_size() < 2)) {
        return 0;
    }

    Frame *next_frame = 0;
    vector<Frame*>::const_iterator f_it(find(Frame::frame_begin(), Frame::frame_end(), frame));

    if (f_it != Frame::frame_end()) {
        vector<Frame*>::const_iterator n_it(f_it);

        if (++n_it == Frame::frame_end()) {
            n_it = Frame::frame_begin();
        }

        while (! next_frame && (n_it != f_it)) {
            if (! (*n_it)->isSkip(mask) && (! mapped || (*n_it)->isMapped()))
                next_frame =  (*n_it);

            if (++n_it == Frame::frame_end()) {
                n_it = Frame::frame_begin();
            }
        }
    }

    return next_frame;
}

//! @brief Finds the previous frame in the list
//!
//! @param frame Frame to search from
//! @param mapped Match only agains mapped frames
//! @param mask Defaults to 0
Frame*
WindowManager::getPrevFrame(Frame* frame, bool mapped, uint mask)
{
    if (! frame || (Frame::frame_size() < 2)) {
        return 0;
    }

    Frame *next_frame = 0;
    vector<Frame*>::const_iterator f_it(find(Frame::frame_begin(), Frame::frame_end(), frame));

    if (f_it != Frame::frame_end()) {
        vector<Frame*>::const_iterator n_it(f_it);

        if (n_it == Frame::frame_begin()) {
            n_it = --Frame::frame_end();
        } else {
            --n_it;
        }

        while (! next_frame && (n_it != f_it)) {
            if (! (*n_it)->isSkip(mask) && (! mapped || (*n_it)->isMapped())) {
                next_frame =  (*n_it);
            }

            if (n_it == Frame::frame_begin()) {
                n_it = --Frame::frame_end();
            } else {
                --n_it;
            }
        }
    }

    return next_frame;
}
