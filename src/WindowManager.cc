//
// WindowManager.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// windowmanager.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowManager.hh"

#include "WindowObject.hh"
#include "ActionHandler.hh"
#include "AutoProperties.hh"
#include "Config.hh"
#include "Theme.hh"
#include "PekwmFont.hh"
#include "Workspaces.hh"
#include "Util.hh"

#include "Frame.hh"
#include "FrameWidget.hh"
#include "Client.hh"

#ifdef KEYS
#include "KeyGrabber.hh"
#endif // KEYS
#ifdef HARBOUR
#include "Harbour.hh"
#include "DockApp.hh"
#endif // HARBOUR

#include "BaseMenu.hh" // we need this for the "Alt+Tabbing"
#ifdef MENUS
#include "ActionMenu.hh"
#include "FrameListMenu.hh"
#ifdef HARBOUR
#include "HarbourMenu.hh"
#endif // HARBOUR
#endif // MENUS

#include <iostream>
#include <list>
#include <algorithm>
#include <functional>

#include <cstdio> // for BUFSIZ
extern "C" {
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
}

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::map;
using std::mem_fun;

WindowManager *wm;

void
sigHandler(int signal)
{
	switch (signal) {
	case SIGHUP:
		wm->reload();
		break;
	case SIGINT:
	case SIGTERM:
		wm->quitNicely();
		break;
	case SIGCHLD:
		wait(NULL);
		break;
	}
}

int
handleXError(Display *dpy, XErrorEvent *e)
{
	if ((e->error_code == BadAccess) &&
			(e->resourceid == wm->getScreen()->getRoot())) {
		cerr << "pekwm: root window unavailable, can't start!" << endl;
		exit(1);
	}
#ifdef DEBUG
	else {
		char error_buf[256];
		XGetErrorText(dpy, e->error_code, error_buf, 256);

		cerr << "XError: " << error_buf << " id: " << e->resourceid << endl;
	}
#endif // DEBUG

	return 0;
}

WindowManager::WindowManager(string command_line) :
_screen(NULL),
#ifdef KEYS
_keygrabber(NULL),
#endif // KEYS
_config(NULL),
_theme(NULL), _action_handler(NULL),
_autoproperties(NULL), _workspaces(NULL),
#ifdef HARBOUR
_harbour(NULL),
#endif // HARBOUR
#ifdef MENUS
_focused_menu(NULL),
#endif // MENUS
_wm_name("pekwm"), _command_line(command_line),
_startup(false),
_focused_client(NULL), _tagged_frame(NULL), _tag_activate(true),
_status_window(None),
_pekwm_atoms(NULL), _icccm_atoms(NULL), _ewmh_atoms(NULL)
{
	::wm = this;

	struct sigaction act;

	// Set up the signal handlers.
	act.sa_handler = sigHandler;
	act.sa_mask = sigset_t();
	act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;

	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGCHLD, &act, NULL);

	// initialize array values
	for (unsigned int i = 0; i < NUM_BUTTONS; ++i) {
		_last_frame_click[i] = 0;
		_last_root_click[i] = 0;
	}

	setupDisplay();

	scanWindows();
	setupFrameIds();

	execStartFile();

	doEventLoop();
}

//! @fn    void execStartFile(void)
//! @brief Checks if the start file is executable then execs it.
void
WindowManager::execStartFile(void)
{
	string start_file(_config->getStartFile());

	bool exec = Util::isExecutable(start_file);
	if (!exec) {
		start_file = SYSCONFDIR "/start";
		exec = Util::isExecutable(start_file);
	}

	if (exec)
		Util::forkExec(start_file);
}

//! @fn    void cleanup(void);
//! @brief Cleans up, Maps windows etc.
void
WindowManager::cleanup(void)
{
	unsigned int num_wins;
	Window d_win1, d_win2, *wins;
	Client *client;

	// Preserve stacking order when removing the clients from the list.
	XQueryTree(_screen->getDisplay(), _screen->getRoot(),
						 &d_win1, &d_win2, &wins, &num_wins);
	for (unsigned i = 0; i < num_wins; ++i) {
		if((client = findClient(wins[i])) ) {
			XMapWindow(_screen->getDisplay(), client->getWindow());
			delete client;
		}
	}
	XFree(wins);
	_client_list.clear();

#ifdef KEYS
	_keygrabber->ungrabKeys(_screen->getRoot());
#endif // KEYS

	screenEdgeDestroy();

	// destroy status window
	XDestroyWindow(_screen->getDisplay(), _status_window);

	// destroy atom windows
	XDestroyWindow(_screen->getDisplay(), _extended_hints_win);

	XInstallColormap(_screen->getDisplay(), _screen->getColormap());
	XSetInputFocus(_screen->getDisplay(), PointerRoot, RevertToNone, CurrentTime);

#ifdef HARBOUR
	_harbour->removeAllDockApps();
#endif // HARBOUR

	XCloseDisplay(_screen->getDisplay());
}

// The destructor cleans up allocated memory and exits cleanly. Hopefully! =)
WindowManager::~WindowManager()
{
#ifdef HARBOUR
	if (_harbour) {
		delete _harbour;
	}
#endif // HARBOUR

	if (_action_handler)
		delete _action_handler;
	if (_autoproperties)
		delete _autoproperties;

#ifdef MENUS
	map<MenuType, BaseMenu*>::iterator it = _menus.begin();
	for (; it != _menus.end(); ++it)
		delete it->second;
	_menus.clear();
#endif // MENUS

	if (_pekwm_atoms)
		delete _pekwm_atoms;
	if (_icccm_atoms)
		delete _icccm_atoms;
	if (_ewmh_atoms)
		delete _ewmh_atoms;

#ifdef KEYS
	if (_keygrabber)
		delete _keygrabber;
#endif // KEYS

	if (_workspaces)
		delete _workspaces;
	if (_config)
		delete _config;
	if (_theme)
		delete _theme;
	if (_screen)
		delete _screen;
}

//! @fn    void setupDisplay(void)
//! @brief
void
WindowManager::setupDisplay(void)
{
	Display *dpy = XOpenDisplay(NULL);
	if (!dpy) {
		cerr << "Can not open display!" << endl
				 << "Your DISPLAY variable currently is set to: "
				 << getenv("DISPLAY") << endl;
		exit(1);
	}

	XSetErrorHandler(handleXError);

	_screen = new ScreenInfo(dpy);
	_action_handler = new ActionHandler(this);

	// get config, needs to be done _BEFORE_ hints
	_config = new Config(dpy);

	// load colors, fonts
	_theme = new Theme(_screen, _config);

	_autoproperties = new AutoProperties(_config);
	_autoproperties->loadConfig();

	_pekwm_atoms = new PekwmAtoms(_screen->getDisplay());
	_icccm_atoms = new IcccmAtoms(_screen->getDisplay());
	_ewmh_atoms = new EwmhAtoms(_screen->getDisplay());
	initHints();

	_workspaces = new Workspaces(_screen, _config, _ewmh_atoms,
															 _config->getWorkspaces());

#ifdef HARBOUR
	_harbour = new Harbour(_screen, _config, _theme, _workspaces);
#endif // HARBOUR

	XDefineCursor(dpy, _screen->getRoot(),
								_screen->getCursor(ScreenInfo::CURSOR_ARROW));

	XSetWindowAttributes sattr;
	sattr.event_mask =
		SubstructureRedirectMask|SubstructureNotifyMask|
		ColormapChangeMask|FocusChangeMask|
		EnterWindowMask|
		PropertyChangeMask|
		ButtonPressMask|ButtonReleaseMask;

	XChangeWindowAttributes(dpy, _screen->getRoot(), CWEventMask, &sattr);

#ifdef KEYS
	_keygrabber = new KeyGrabber(_screen, _config);
	_keygrabber->load(_config->getKeyFile());
	_keygrabber->grabKeys(_screen->getRoot());
#endif // KEYS

#ifdef MENUS
	_menus[WINDOWMENU_TYPE] = new ActionMenu(this, WINDOWMENU_TYPE, "");
	_menus[ROOTMENU_TYPE] = new ActionMenu(this, ROOTMENU_TYPE, "");
	_menus[GOTOMENU_TYPE] = new FrameListMenu(this, GOTOMENU_TYPE);
	_menus[ICONMENU_TYPE] = new FrameListMenu(this, ICONMENU_TYPE);
	_menus[ATTACH_CLIENT_TYPE] = new FrameListMenu(this, ATTACH_CLIENT_TYPE);
	_menus[ATTACH_FRAME_TYPE] = new FrameListMenu(this, ATTACH_FRAME_TYPE);
	_menus[ATTACH_CLIENT_IN_FRAME_TYPE] =
		new FrameListMenu(this, ATTACH_CLIENT_IN_FRAME_TYPE);
	_menus[ATTACH_FRAME_IN_FRAME_TYPE] =
		new FrameListMenu(this, ATTACH_FRAME_IN_FRAME_TYPE);

	_menus[WINDOWMENU_TYPE]->reload();
	_menus[ROOTMENU_TYPE]->reload();
#endif // MENUS

	// Create screen edge windows
	screenEdgeCreate();
	if (_config->getWWMouse())
		screenEdgeMap();

	// Create status window
	_status_window =
		XCreateSimpleWindow(dpy, _screen->getRoot(), 0, 0, 1, 1,
												_theme->getMenuBorderWidth(),
												_theme->getMenuBorderColor().pixel,
												_theme->getMenuBackground().pixel);
}

//! @fn    void scanWindows(void)
//! @brief Goes through the window and creates Clients/DockApps.
void
WindowManager::scanWindows(void)
{
	if (_startup) // only done once when we start
		return;

	unsigned int num_wins;
	Window d_win1, d_win2, *wins;
	XWindowAttributes attr;

	// Lets create a list of windows on the display
	XQueryTree(_screen->getDisplay(), _screen->getRoot(),
						 &d_win1, &d_win2, &wins, &num_wins);
	list<Window> win_list(wins, wins + num_wins);
	XFree(wins);

	list<Window>::iterator it = win_list.begin();

#ifdef HARBOUR
	// If we have the Harbour on, we filter out all windows with the
	// the IconWindowHint set not pointing to themself, making DockApps
	// work as they are supposed to.
	for (; it != win_list.end(); ++it) {
		if (*it == None)
			continue;

		XWMHints *wm_hints = XGetWMHints(_screen->getDisplay(), *it);
		if (wm_hints) {
			if ((wm_hints->flags&IconWindowHint) &&
					(wm_hints->icon_window != *it)) {
				list<Window>::iterator i_it =
					find(win_list.begin(), win_list.end(), wm_hints->icon_window);
				if (i_it != win_list.end())
					*i_it = None;
			}
			XFree(wm_hints);
		}
	}
#endif // HARBOUR

	Client *client;
	for (it = win_list.begin(); it != win_list.end(); ++it) {
		if (*it == None)
			continue;

		XGetWindowAttributes(_screen->getDisplay(), *it, &attr);
		if (!attr.override_redirect && (attr.map_state != IsUnmapped)) {
#ifdef HARBOUR
			XWMHints *wm_hints = XGetWMHints(_screen->getDisplay(), *it);
			if (wm_hints) {
				if ((wm_hints->flags&StateHint) &&
						(wm_hints->initial_state == WithdrawnState)) {
					_harbour->addDockApp(new DockApp(_screen, _theme, *it));
				} else {
					client = new Client(this, *it);
					if (!client->isAlive())
						delete client;

				}
				XFree(wm_hints);
			} else
#endif // HARBOUR
				{
					client = new Client(this, *it);
					if (!client->isAlive())
						delete client;
				}
		}
	}

	// Try to focus the ontop window, if no window we give root focus
	WindowObject *wo = _workspaces->getTopWO(WindowObject::WO_FRAME);
	if (wo && wo->isMapped())
		wo->giveInputFocus();
	else
		XSetInputFocus(_screen->getDisplay(), _screen->getRoot(),
									 RevertToNone, CurrentTime);

	// We won't be needing these anymore until next restart
	_autoproperties->removeApplyOnStart();

	_startup = true;
}

void
WindowManager::screenEdgeCreate(void)
{
	if (_screen_edge_list.size())
		return;

	_screen_edge_list.push_back(new ScreenInfo::ScreenEdge(_screen->getDisplay(), _screen->getRoot(), SE_LEFT));
	_screen_edge_list.push_back(new ScreenInfo::ScreenEdge(_screen->getDisplay(), _screen->getRoot(), SE_RIGHT));

	// TO-DO: This is going to be unset for now. If we later want that "magic-
	// pixel" around the edge
	// 	_screen_edge_list.push_back(new ScreenInfo::ScreenEdge(_screen->getDisplay(), _screen->getRoot(), SE_TOP));
	// 	_screen_edge_list.push_back(new ScreenInfo::ScreenEdge(_screen->getDisplay(), _screen->getRoot(), SE_BOTTOM));

	screenEdgeResize();
}

void
WindowManager::screenEdgeDestroy(void)
{
	if (!_screen_edge_list.size())
		return;

	list<ScreenInfo::ScreenEdge*>::iterator it = _screen_edge_list.begin();
	for (; it != _screen_edge_list.end(); ++it) {
		_workspaces->remove(*it);
		XDestroyWindow(_screen->getDisplay(), (*it)->getWindow());
		delete *it;
	}
}

void
WindowManager::screenEdgeResize(void)
{
	if (_screen_edge_list.size() != 2)
		return;

	unsigned int ww = _config->getWWMouse();
	if (!ww)
		ww = 1;

	list<ScreenInfo::ScreenEdge*>::iterator it = _screen_edge_list.begin();

	// left edge
	(*it)->move(0, 0);
	(*it)->resize(ww, _screen->getHeight());
	++it;

	// right edge
	(*it)->move(_screen->getWidth() - ww, 0);
	(*it)->resize(ww, _screen->getHeight());
	++it;

	/*
	// top edge
	(*it)->move(ww, 0);
	(*it)->resize(_screen->getWidth() - (ww * 2), ww);
	++it;

	// bottom edge
	(*it)->move(ww, _screen->getHeight() - ww);
	(*it)->resize(_screen->getWidth() - (ww * 2), ww);
	*/
}

void
WindowManager::screenEdgeMap(void)
{
	list<ScreenInfo::ScreenEdge*>::iterator it = _screen_edge_list.begin();
	for (; it != _screen_edge_list.end(); ++it)
		(*it)->mapWindow();
}

void
WindowManager::screenEdgeUnmap(void)
{
	list<ScreenInfo::ScreenEdge*>::iterator it = _screen_edge_list.begin();
	for (; it != _screen_edge_list.end(); ++it)
		(*it)->unmapWindow();
}

ScreenEdgeType
WindowManager::screenEdgeFind(Window window)
{
	list<ScreenInfo::ScreenEdge*>::iterator it = _screen_edge_list.begin();
	for (; it != _screen_edge_list.end(); ++it) {
		if ((*it)->getWindow() == window)
			return (*it)->getEdge();
	}

	return SE_NO;
}


void
WindowManager::quitNicely(void)
{
	cleanup();
	exit(0);
}

void
WindowManager::reload(void)
{
#ifdef HARBOUR
	// I do not want to restack or rearrange if nothing has changed.
	unsigned int old_harbour_placement = _config->getHarbourPlacement();
	bool old_harbour_stacking = _config->getHarbourOntop();
#endif // HARBOUR

	_config->load(); // reload the config
	_autoproperties->loadConfig(); // reload autoprops config

	// update what might have changed in the cfg toouching the hints
	setNumWorkspaces(_config->getWorkspaces());

	// reload the theme
	_theme->setThemeDir(_config->getThemeFile());
	_theme->load();

	// resize the screen edge
	screenEdgeResize();
	// map / unmap as needed
	if (_config->getWWMouse())
		screenEdgeMap();
	else
		screenEdgeUnmap();

	// update the status window theme
	XSetWindowBackground(_screen->getDisplay(), _status_window,
											 _theme->getMenuBackground().pixel);
	XSetWindowBorder(_screen->getDisplay(), _status_window,
									 _theme->getMenuBorderColor().pixel);
	XSetWindowBorderWidth(_screen->getDisplay(), _status_window,
												_theme->getMenuBorderWidth());

	// reload the themes on the frames
	for_each(_frame_list.begin(), _frame_list.end(),
					 mem_fun(&Frame::loadTheme));

	// regrab the buttons on the client windows
	for_each(_client_list.begin(), _client_list.end(),
					 mem_fun(&Client::grabButtons));

#ifdef KEYS
	// reload the keygrabber
	_keygrabber->load(_config->getKeyFile());

	_keygrabber->ungrabKeys(_screen->getRoot());
	_keygrabber->grabKeys(_screen->getRoot());

	// regrab keys
	list<Client*>::iterator c_it = _client_list.begin();
	for (; c_it != _client_list.end(); ++c_it) {
		_keygrabber->ungrabKeys((*c_it)->getWindow());
		_keygrabber->grabKeys((*c_it)->getWindow());
	}
#endif // KEYS

	// load autoprops
	list<Frame*>::iterator f_it = _frame_list.begin();
	for (; f_it != _frame_list.end(); ++f_it)
		(*f_it)->readAutoprops();

#ifdef HARBOUR
	_harbour->loadTheme();
	if (old_harbour_placement != _config->getHarbourPlacement()) {
		_harbour->rearrange();
		_harbour->updateHarbourSize();
	}
	if (old_harbour_stacking != _config->getHarbourOntop())
		_harbour->restack();
#endif // HARBOUR

#ifdef MENUS
	map<MenuType, BaseMenu*>::iterator bm_it = _menus.begin();
	for (; bm_it != _menus.end(); ++bm_it) {
		bm_it->second->loadTheme();
		bm_it->second->reload();
	}
#ifdef HARBOUR
	_harbour->getHarbourMenu()->loadTheme();
	_harbour->getHarbourMenu()->updateMenu();
#endif // HARBOUR
#endif // MENUS
}

//! @fn    void restart(const string &command)
//! @breif Exit pekwm and restart with the command command
void
WindowManager::restart(string command)
{
	if (!command.size())
		command = _command_line;

	cleanup();
	execlp("/bin/sh", "sh" , "-c", command.c_str(), (char*) NULL);
}

// Event handling routins beneath this =====================================

void
WindowManager::doEventLoop(void)
{
	Display *dpy = _screen->getDisplay();
	Client *client = NULL; // used for shape events

	XEvent ev;
	while (true) {
		XNextEvent(dpy, &ev);

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
			handlePropertyEvent(&ev.xproperty);
			break;
		case Expose:
			handleExposeEvent(&ev.xexpose);
			break;

		case KeyPress:
		case KeyRelease:
			handleKeyEvent(&ev.xkey);
			break;

		case ButtonPress:
			handleButtonPressEvent(&ev.xbutton);
			break;
		case ButtonRelease:
			handleButtonReleaseEvent(&ev.xbutton);
			break;

		case MotionNotify:
			handleMotionEvent(&ev.xmotion);
			break;

		case EnterNotify:
			handleEnterNotify(&ev.xcrossing);
			break;
		case LeaveNotify:
			handleLeaveNotify(&ev.xcrossing);
			break;
		case FocusIn:
			handleFocusInEvent(&ev.xfocus);
			break;
		case FocusOut:
			handleFocusOutEvent(&ev.xfocus);
			break;

#ifdef SHAPE
		default:
			if (_screen->hasShape() && (ev.type == _screen->getShapeEvent())) {
				client = findClient(ev.xany.window);
				if (client && client->getFrame() &&
						client->getFrame()->isActiveClient(client)) {
					client->setShaped(client->getFrame()->getFrameWidget()->
														setShape(client->getWindow(), client->isShaped()));
				}
			}
			break;
#endif // SHAPE
		}
	}
}

//! @fn    void handleKeyEvent(XKeyEvent *ev)
//! @brief Handle XKeyEvents
void
WindowManager::handleKeyEvent(XKeyEvent *ev)
{
#ifdef KEYS
	if (ev->type != KeyPress)
		return;

	ActionEvent* ae;

#ifdef MENUS
	if (_focused_menu) {
		ae = _keygrabber->findMenuAction(ev);
		if (ae) {
			list<Action>::iterator it = ae->action_list.begin();
			for (; it != ae->action_list.end(); ++it) {
				switch (it->action) {
				case MENU_NEXT:
					_focused_menu->selectNextItem();
					break;
				case MENU_PREV:
					_focused_menu->selectPrevItem();
					break;
				case MENU_SELECT:
					_focused_menu->execItem(_focused_menu->getCurrItem());
					break;
				case MENU_ENTER_SUBMENU:
					if (_focused_menu->getCurrItem() &&
							_focused_menu->getCurrItem()->submenu) {
						_focused_menu->mapSubmenu(_focused_menu->getCurrItem()->submenu);
						_focused_menu = _focused_menu->getCurrItem()->submenu;
						_focused_menu->selectItem((unsigned int) 0);
					}
					break;
				case MENU_LEAVE_SUBMENU:
					if (_focused_menu->getParent()) {
						_focused_menu = _focused_menu->getParent();
						_focused_menu->unmapSubmenus();
					}
					break;
				case MENU_CLOSE:
					_focused_menu->unmapAll();
					break;
				default:
					break;
				}
			}
		}
	} else
#endif // MENUS
		{
			ae = _keygrabber->findGlobalAction(ev);
			if (ae) {
				ActionPerformed ap(ae, _focused_client);
				ap.type = ev->type;
				ap.event.key = ev;
 
				_action_handler->handleAction(ap);
			}
		}

	// Flush Enter events caused by keygrabbing
	XEvent e;
	while (XCheckTypedEvent(_screen->getDisplay(), EnterNotify, &e));
#endif // KEYS
}

void
WindowManager::handleButtonPressEvent(XButtonEvent *ev)
{
	// Flush Button presses
	while (XCheckTypedEvent(_screen->getDisplay(), ButtonPress, (XEvent *) ev));

	ActionEvent *ae = NULL;
	Client *client = NULL;
#ifdef MENUS
	BaseMenu *bm = NULL;
#endif // MENUS

	// First handle root window actions
	if (ev->window == _screen->getRoot()) {
		ae = findMouseButtonAction(ev, BUTTON_PRESS);
	} else {
		client = findClient(ev->window);
		if (client) {
			ae = client->getFrame()->handleButtonEvent(ev);
			if (ae) {
				if (ev->subwindow)
					client = client->getFrame()->getActiveClient();
				else
					client = client->getFrame()->getClientFromPos(ev->x);
			}

			if ((_config->getFocusMask()&FOCUS_CLICK) && // TO-DO: Always this way?
					(client != _focused_client) && (ev->button == BUTTON1)) {
				client->getFrame()->giveInputFocus();
			}
		}
#ifdef MENUS
		// handle menu clicks
		else if ((bm = BaseMenu::findMenu(ev->window))) {
			bm->handleButtonPressEvent(ev);
		}
#endif // MENUS

#ifdef HARBOUR
		else {
			DockApp *da = _harbour->findDockAppFromFrame(ev->window);
			if (da) {
				_harbour->handleButtonEvent(ev, da);
			}
		}
#endif // HARBOUR
	}

	if (ae) {
		ActionPerformed ap(ae, client);
		ap.type = ev->type;
		ap.event.button = ev;

		_action_handler->handleAction(ap);		
	}
}

void
WindowManager::handleButtonReleaseEvent(XButtonEvent *ev)
{
	// Flush ButtonReleases
	while (XCheckTypedEvent(_screen->getDisplay(), ButtonRelease, (XEvent *) ev));

	ActionEvent *ae = NULL;
	Client *client = NULL;
#ifdef MENUS
	BaseMenu *bm = NULL;
#endif // MENUS

	if (ev->window == _screen->getRoot()) {
		if (ev->button <= NUM_BUTTONS) { // as _last_root_click can be dangerous
			MouseButtonType mb = BUTTON_RELEASE;

			if ((ev->time - _last_root_click[ev->button - 1]) <
					_config->getDoubleClickTime()) {

				_last_root_click[ev->button - 1] = 0;
				mb = BUTTON_DOUBLE;

			} else
				_last_root_click[ev->button - 1] = ev->time;

			ae = findMouseButtonAction(ev, mb);
		}
	} else if ((client = findClient(ev->window))) {
		ae = client->getFrame()->handleButtonEvent(ev);
		client = client->getFrame()->getActiveClient();
		if (ae) {
			if (ev->subwindow)
				client = client->getFrame()->getActiveClient();
			else
				client = client->getFrame()->getClientFromPos(ev->x);
		}
	}
#ifdef MENUS
	else if ((bm = BaseMenu::findMenu(ev->window))) {
		bm->handleButtonReleaseEvent(ev);
	}
#endif // MENUS

#ifdef HARBOUR
	else {
		DockApp *da = _harbour->findDockAppFromFrame(ev->window);
		if (da) {
			_harbour->handleButtonEvent(ev, da);
		}
	}
#endif // HARBOUR

	if (ae) {
		ActionPerformed ap(ae, client);
		ap.type = ev->type;
		ap.event.button = ev;

		_action_handler->handleAction(ap);
	}
}

void
WindowManager::handleConfigureRequestEvent(XConfigureRequestEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		client->getFrame()->handleConfigureRequest(ev, client);

	} else {
#ifdef HARBOUR
		DockApp *da = _harbour->findDockApp(ev->window);
		if (da) {
			_harbour->handleConfigureRequestEvent(ev, da);
		} else
#endif // HARBOUR
			{
				// Since this window isn't yet a client lets delegate
				// the configure request back to the window so it can use it.

				XWindowChanges wc;

				wc.x = ev->x;
				wc.y = ev->y;
				wc.width = ev->width;
				wc.height = ev->height;
				wc.sibling = ev->above;
				wc.stack_mode = ev->detail;

				XConfigureWindow(_screen->getDisplay(), ev->window,
												 ev->value_mask, &wc);
			}
	}
}

void
WindowManager::handleMotionEvent(XMotionEvent *ev)
{
	Client *client = findClient(ev->window);
#ifdef MENUS
	BaseMenu *bm = NULL;
#endif // MENUS

	// First try to operate on a client
	if (client) {
		ActionEvent *ae = client->getFrame()->handleMotionNotifyEvent(ev);
		if (ae) {
			if (ev->subwindow)
				client = client->getFrame()->getActiveClient();
			else
				client = client->getFrame()->getClientFromPos(ev->x);

			ActionPerformed ap(ae, client);
			ap.type = ev->type;
			ap.event.motion = ev;

			_action_handler->handleAction(ap);
		}
	}
	// Else we try to find a menu
#ifdef MENUS
	else if ((bm = BaseMenu::findMenu(ev->window))) {
		bm->handleMotionNotifyEvent(ev);
	}
#endif // MENUS

#ifdef HARBOUR
	else {
		DockApp *da = _harbour->findDockAppFromFrame(ev->window);
		if (da) {
			_harbour->handleMotionNotifyEvent(ev, da);
		}
	}
#endif // HARBOUR
}

void
WindowManager::handleMapRequestEvent(XMapRequestEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		client->handleMapRequest(ev);
	} else {
		XWindowAttributes attr;
		XGetWindowAttributes(_screen->getDisplay(), ev->window, &attr);
		if (!attr.override_redirect) {
			// if we have the harbour enabled, we need to figure out wheter or
			// not this is a dockapp.
#ifdef HARBOUR
			XWMHints *wm_hints = XGetWMHints(_screen->getDisplay(), ev->window);
			if (wm_hints) {
				if ((wm_hints->flags&StateHint) &&
						(wm_hints->initial_state == WithdrawnState)) {
#ifdef DEBUG
					cerr << __FILE__ << "@" << __LINE__ << ": "
							 << "DockApp Mapping: " << ev->window << endl;
#endif // DEBUG
					_harbour->addDockApp(new DockApp(_screen, _theme, ev->window));
				} else {
					client = new Client(this, ev->window);
					if (!client->isAlive())
						delete client;
					else if (_config->getFocusMask()&FOCUS_NEW)
						client->getFrame()->giveInputFocus();
				}
				XFree(wm_hints);
			} else
#endif // HARBOUR
				{
					client = new Client(this, ev->window);
					if (!client->isAlive())
						delete client;
					else if (_config->getFocusMask()&FOCUS_NEW)
						client->getFrame()->giveInputFocus();
				}
		}
	}
}

void
WindowManager::handleUnmapEvent(XUnmapEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		Window frame_win = client->getFrame()->getFrameWidget()->getWindow();
		client->handleUnmapEvent(ev);

		if (!_focused_client)
			findFrameAndFocus(frame_win);
	}
#ifdef HARBOUR
	else {
		DockApp *da = _harbour->findDockApp(ev->window);
		if (da) {
			if (ev->window == ev->event) {
#ifdef DEBUG
				cerr << __FILE__ << "@" << __LINE__ << ": "
						 << "Unmapping DockaApp: " << da << endl;
#endif // DEBUG
				_harbour->removeDockApp(da);
			}
		}
	}
#endif // HARBOUR
}

void
WindowManager::handleDestroyWindowEvent(XDestroyWindowEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		Window frame_win = client->getFrame()->getFrameWidget()->getWindow();
		client->handleDestroyEvent(ev);

		if (!_focused_client)
			findFrameAndFocus(frame_win);
	}
#ifdef HARBOUR
	else {
		DockApp *da = _harbour->findDockApp(ev->window);
		if (da) {
#ifdef DEBUG
			cerr << __FILE__ << "@" << __LINE__ << ": "
					 << "Destroying DockApp: " << da << endl;
#endif // DEBUG
			da->setAlive(false);
			_harbour->removeDockApp(da);
		}
	}
#endif // HARBOUR
}

void
WindowManager::handleEnterNotify(XCrossingEvent *ev)
{
	// Clear event queue
	while (XCheckTypedEvent(_screen->getDisplay(), EnterNotify, (XEvent *) ev));

	if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab))
		return;

	ScreenEdgeType se_type;

#ifdef MENUS
	BaseMenu *menu = BaseMenu::findMenu(ev->window);
	if (menu && (menu != _focused_menu))
		menu->giveInputFocus();
	else
#endif // MENUS
		if (((se_type = screenEdgeFind(ev->window)) != SE_NO) &&
				_config->getWWMouse()) {
			switch (se_type) {
			case SE_LEFT:
				warpWorkspace(-1, _config->getWWMouseWrap(), true,
											_screen->getWidth() - (_config->getWWMouse() * 2),
											ev->y_root);
				break;
			case SE_RIGHT:
				warpWorkspace(1, _config->getWWMouseWrap(), true,
											_config->getWWMouse() * 2,
											ev->y_root);
				break;
			default:
				// do nothing
				break;
			}

		} else if ((_config->getFocusMask()&(FOCUS_ENTER|FOCUS_LEAVE))) {
			Client *client = findClient(ev->window);
			if (client) {
				client = client->getFrame()->getActiveClient();
				if (client != _focused_client) {
					if (_config->getFocusMask()&FOCUS_LEAVE) {
						client->getFrame()->giveInputFocus();
					} else if (client->getLayer() != LAYER_DESKTOP) {
						client->getFrame()->giveInputFocus();
					}
				}
			} else if (_config->getFocusMask()&FOCUS_LEAVE) {
				XSetInputFocus(_screen->getDisplay(), _screen->getRoot(),
											 RevertToNone, CurrentTime);
			}
		}
}

void
WindowManager::handleLeaveNotify(XCrossingEvent *ev)
{
#ifdef MENUS
	BaseMenu *bm;

	if ((bm = BaseMenu::findMenu(ev->window))) {
		bm->handleLeaveEvent(ev);
	}
#endif // MENUS
}

void
WindowManager::handleFocusInEvent(XFocusChangeEvent *ev)
{
	if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab))
		return;

#ifdef MENUS
	_focused_menu = BaseMenu::findMenu(ev->window);
	if (_focused_menu && !_focused_menu->isMapped()) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Hidden menu is getting FocusIn event." << endl;
#endif // DEBUG
		_focused_menu = NULL;
		findFrameAndFocus(None);
	}
#endif // MENUS

	Client *client = findClientFromWindow(ev->window);
	if (client && (client != _focused_client)) {
		_focused_client = client;
		_focused_client->getFrame()->setFocused(true);

		setEwmhActiveWindow(_focused_client->getWindow());
	}
}

void
WindowManager::handleFocusOutEvent(XFocusChangeEvent *ev)
{
	if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab))
		return;

#ifdef MENUS
	if (_focused_menu) {
		_focused_menu = NULL;

		findFrameAndFocus(None);
	} else
#endif // MENUS
		{
			if (_focused_client) {
				_focused_client->getFrame()->setFocused(false);
				_focused_client = NULL;
			}

			Client *client = findClientFromWindow(ev->window);
			if (client) {
				client->getFrame()->setFocused(false);
				_workspaces->setLastFocused(_workspaces->getActive(),
																		 client->getFrame());
				if (client == _focused_client)
					_focused_client = NULL;
			}
		}
}

void
WindowManager::handleClientMessageEvent(XClientMessageEvent *ev)
{
	// Handle root window messages
	if (ev->window == _screen->getRoot()) {
		if (ev->format == 32) { // format 32 events
			if (ev->message_type == _ewmh_atoms->getAtom(NET_CURRENT_DESKTOP)) {
				setWorkspace(ev->data.l[0], true);

			} else if (ev->message_type ==
								 _ewmh_atoms->getAtom(NET_NUMBER_OF_DESKTOPS)) {
				if (ev->data.l[0] > 0) {
					setNumWorkspaces(ev->data.l[0]);
				}
			}
		}
	} else {
		Client *client = findClientFromWindow(ev->window);
		if (client)
			client->getFrame()->handleClientMessage(ev, client);
	}
}

void
WindowManager::handleColormapEvent(XColormapEvent *ev)
{
	Client *client = findClient(ev->window);
	if (client) {
		client = client->getFrame()->getActiveClient();
		client->handleColormapChange(ev);
	}
}

void
WindowManager::handlePropertyEvent(XPropertyEvent *ev)
{
	if (ev->window == _screen->getRoot())
		return;

	Client *client = findClientFromWindow(ev->window);

	if (client)
		client->getFrame()->handlePropertyChange(ev, client);
}

void
WindowManager::handleExposeEvent(XExposeEvent *ev)
{
#ifdef MENUS
	BaseMenu *bm = NULL;
	if ((bm = BaseMenu::findMenu(ev->window))) {
		bm->handleExposeEvent(ev);
	}
#endif // MENUS
}

//! @fn    ActionEvent* findMouseButtonAction(XButtonEvent *ev, MouseButtonType type)
//! @brief Matchess a rootclick action against a XButtonEvent
ActionEvent*
WindowManager::findMouseButtonAction(XButtonEvent *ev, MouseButtonType type)
{
	if (!ev)
		return NULL;

	ev->state &= ~_screen->getNumLock() & ~_screen->getScrollLock() & ~LockMask;
	ev->state &= ~Button1Mask & ~Button2Mask & ~Button3Mask
		& ~Button4Mask & ~Button5Mask;

	list<ActionEvent> *actions = _config->getMouseRootList();
	list<ActionEvent>::iterator it = actions->begin();
	for (; it != actions->end(); ++it) {
		if ((it->sym == ev->button) && (it->mod == ev->state) &&
				((signed)it->type == type)) {
			return &*it;
		}
	}

	return NULL;
}

// Event handling routines stop ============================================


//! @fn    void findFrameAndFocus(Window frame_search)
//! @brief Searches for a Frame to focus, and then gives it input focus.
void
WindowManager::findFrameAndFocus(Window frame_search)
{
	WindowObject *focus = NULL;

	// First, try to find a Frame from frame_search window.
	if (frame_search != None) {
		Frame *frame = findFrameFromWindow(frame_search);
		if (frame && frame->isMapped())
			focus = frame;
	}

	// If that didn't succeed, lets see if we have a last focused object.
	if (!focus) {
		focus = _workspaces->getLastFocused(_workspaces->getActive());

		// We didn't have a last focused, get the top WindowObject
		if (!focus) {
			focus = _workspaces->getTopWO(WindowObject::WO_FRAME);
		}
	}

	if (focus)
		focus->giveInputFocus();
	else
		setEwmhActiveWindow(None);
}

//! @fn    void focusNextPrevFrame(unsigned int button, unsigned int raise, bool next)
//! @brief Tries to find the next/prev frame relative to the focused client
void
WindowManager::focusNextPrevFrame(unsigned int button, unsigned int raise,
																	bool next)
{
	BaseMenu menu(_screen, _theme, _workspaces, "Windows");
	ActionEvent ae; // empty ae, used when inserting

	list<Frame*>::iterator f_it = _frame_list.begin();
	unsigned int num = 0;
	for (unsigned int i = 0; f_it != _frame_list.end(); ++f_it) {
		// if it's not hidden it's either sticky or on this workspace
		if ((*f_it)->isMapped() && (*f_it)->getActiveClient() &&
				!((*f_it)->isSkip(SKIP_FOCUS_TOGGLE))) {
			if ((*f_it)->isActiveClient(_focused_client))
				num = i;

			menu.insert(*(*f_it)->getActiveClient()->getName(), ae,
									(*f_it)->getActiveClient());
			++i;
		}
	}

	if (!menu.size())
		return; // No clients in the list

	if (!_screen->grabKeyboard(_screen->getRoot()))
		return;

	menu.selectItem(num);
	if (_config->getShowFrameList()) {
		menu.updateMenu();

#ifdef XINERAMA
		Geometry head;
		_screen->getHeadInfo(_screen->getCurrHead(), head);
		menu.move(head.x + ((head.width - menu.getWidth()) / 2),
							head.y + ((head.height - menu.getHeight()) / 2));
#else // !XINERAMA
		menu.move((_screen->getWidth() - menu.getWidth()) / 2,
							(_screen->getHeight() - menu.getHeight()) / 2);
#endif // XINERAMA
	}

	// Create a window that'll show us the list of clients
	if (_focused_client)
		_focused_client->getFrame()->setFocused(false);

	if (next)
		menu.selectNextItem();
	else
		menu.selectPrevItem();

	if (_config->getShowFrameList()) {
		menu.mapWindow();
		menu.redraw();
	}

	Client *client = menu.getCurrItem()->client;

	XEvent ev;
	bool cycling = true;
	while (cycling) {
		if (client) {
			client->getFrame()->setFocused(true);
			if (Raise(raise) == ALWAYS_RAISE)
				client->getFrame()->raise();
		}

		XMaskEvent(_screen->getDisplay(), KeyPressMask|KeyReleaseMask, &ev);
		if (ev.type == KeyPress) {
			if (ev.xkey.keycode == button) {
				if (client)
					client->getFrame()->setFocused(false);

				if (next) { // Get the next Client
					menu.selectNextItem(); // highlight next item on the menu
				} else { // Get the previous Client
					menu.selectPrevItem(); // highlight prev item on the menu
				}

				client = menu.getCurrItem()->client;
			} else {
				XPutBackEvent (_screen->getDisplay(), &ev);
				cycling = false;
			}
		} else if (ev.type == KeyRelease) {
			if (IsModifierKey(XKeycodeToKeysym(_screen->getDisplay(),
												ev.xkey.keycode, 0))) {
				cycling = false;
			}
		} else
			XPutBackEvent(_screen->getDisplay(), &ev);
	}

	_screen->ungrabKeyboard();

	if (client)
		client->getFrame()->giveInputFocus();
	if (Raise(raise) == END_RAISE)
		client->getFrame()->raise();
}

//! @fn    Client* findClientFromWindow(Window win)
//! @brief Finds the Client wich holds the window w, only matching the window.
//! @param w Window to use as criteria when searching
Client*
WindowManager::findClientFromWindow(Window win)
{
	if (win == _screen->getRoot())
		return NULL;
	list<Client*>::iterator it = _client_list.begin();
	for(; it != _client_list.end(); ++it) {
		if (win == (*it)->getWindow())
			return (*it);
	}
	return NULL;
}

//! @fn    Frame* findFrameFromWindow(Window win)
//! @brief
Frame*
WindowManager::findFrameFromWindow(Window win)
{
	if (win == _screen->getRoot())
		return NULL;
	list<Frame*>::iterator it = _frame_list.begin();
	for(; it != _frame_list.end(); ++it) {
		if (win == (*it)->getFrameWidget()->getWindow())
			return (*it);
	}
	return NULL;
}

//! @fn    void setWorkspace(unsigned int workspace)
//! @brief Activates Workspace workspace and sets the right hints
//! @param workspace Workspace to activate
void
WindowManager::setWorkspace(unsigned int workspace, bool focus)
{
	if ((workspace == _workspaces->getActive()) ||
			(workspace >= _workspaces->getNumber()))
		return;

	_screen->grabServer();

	// save the focused client
	if (_focused_client) {
		_workspaces->setLastFocused(_workspaces->getActive(),
																 _focused_client->getFrame());
		_focused_client->getFrame()->setFocused(false);
		_focused_client = NULL;
	} else
		_workspaces->setLastFocused(_workspaces->getActive(), NULL);

#ifdef MENUS
	// hide the windowmenu if it's visible
	ActionMenu *menu = (ActionMenu*) getMenu(WINDOWMENU_TYPE);
	if (menu->isMapped())
		menu->unmapAll(); // TO-DO: ADD CHECK TO SEE IF FRAME IS HIDDEN
	menu->setClient(NULL);
#endif // MENUS

	// switch workspace
	_workspaces->hideAll(_workspaces->getActive());
	setAtomLongValue(_screen->getRoot(),
									 _ewmh_atoms->getAtom(NET_CURRENT_DESKTOP), workspace);
	_workspaces->unhideAll(workspace, focus);

	_screen->ungrabServer(true);
}

//! @fn
//! @brief
bool
WindowManager::warpWorkspace(int diff, bool wrap, bool focus, int x, int y)
{
	int ws = _workspaces->getActive() + diff;
	int num_ws = _workspaces->getNumber() - 1;

	if (wrap) {
		if (ws < 0)
			ws = num_ws;
		else if (ws > num_ws)
			ws = 0;
	} else if ((ws < 0) || (ws > num_ws))
		return false;

	// warp pointer
	XWarpPointer(_screen->getDisplay(), None, _screen->getRoot(),
							 0, 0, 0, 0, x, y);

	// set workpsace
	setWorkspace(ws, focus);

	return true;
}

void
WindowManager::setNumWorkspaces(unsigned int num)
{
	if (num < 1)
		return;

	_workspaces->setNumber(num);

	// We don't want to be on a non existing workspace
	if (num <= _workspaces->getActive())
		setWorkspace(num - 1, true);

	// TO-DO: activate the new active workspace once again, to make sure
	// all clients are visible
}

//! @fn    Client* findClient(window win)
//! @brief Finds the Client wich holds the Window w.
//! @param w Window to use as criteria when searching.
//! @return Pointer to the client if found, else NULL
Client*
WindowManager::findClient(Window win)
{
	if ((win == None) || (win == _screen->getRoot()))
		return NULL;

	if (_client_list.size()) {
		list<Client*>::iterator it = _client_list.begin();
		for (; it != _client_list.end(); ++it) {
			if (win == (*it)->getWindow())
				return (*it);
			else if ((*it)->getFrame() &&
							 (*(*it)->getFrame()->getFrameWidget()) == win)
				return (*it);
		}
	}
	return NULL;
}

//! @fn    void findTransientToMapOrUnmap(Window win, bool hide)
//! @brief
void
WindowManager::findTransientsToMapOrUnmap(Window win, bool hide)
{
	if (!_client_list.size())
		return;

	list<Client*>::iterator it = _client_list.begin();
	for (; it != _client_list.end(); ++it) {
		if (((*it)->getTransientWindow() == win) &&
				((*it)->getFrame()->isActiveClient(*it))) {
			if (hide) {
				if (!(*it)->isIconified()) {
					(*it)->getFrame()->iconify();
				}
			} else if ((*it)->isIconified()) {
				if ((*it)->getWorkspace() != _workspaces->getActive()) {
					(*it)->setWorkspace(_workspaces->getActive());
				}

				(*it)->getFrame()->mapWindow();
			}
		}
	}
}

//! @fn    void removeFromClientList(Client *client)
//! @brief
void
WindowManager::removeFromClientList(Client *client)
{
	if (client == _focused_client)
		_focused_client = NULL;

	_client_list.remove(client);
}

//! @fn    void removeFromFrameList(Frame *frame)
//! @brief
void
WindowManager::removeFromFrameList(Frame *frame)
{
	if (_tagged_frame == frame)
		_tagged_frame = NULL;

	_frame_list.remove(frame);
	_frameid_list.push_back(frame->getId());
}

//! @fn    Client* findClient(const ClassHint *class_hint)
//! @brief
Client*
WindowManager::findClient(const ClassHint *class_hint)
{
	if (! _client_list.size())
		return NULL;
	list<Client*>::iterator it = _client_list.begin();
	for (; it != _client_list.end(); ++it) {
		if (*class_hint == *(*it)->getClassHint())
			return *it;
	}
	return NULL;
}


//! @fn    Frame* findGroup(const ClassHint *class_hint, unsigned int max, bool focused)
//! @brief
Frame*
WindowManager::findGroup(const ClassHint *class_hint, AutoProperty* property)
{
	// should we search the focused frame first?
	if (_focused_client && property->group_focused_first &&
			_focused_client->getFrame()->isMapped() && // make sure
			(_focused_client->getFrame()->getNumClients() < property->group_size) &&
			(*class_hint == *_focused_client->getFrame()->getClassHint())) {
		return _focused_client->getFrame();
	}

	// didn't work on the focused, search all frames			
	list<Frame*>::iterator it = _frame_list.begin();
	for (; it != _frame_list.end(); ++it) {
		if ((property->group_global || (*it)->isMapped()) && // Frame is mapped
				((*it)->getNumClients() < property->group_size) && // Any space
				(*(*it)->getClassHint() == *class_hint)) { // the class matches
			return *it;
		}
	}

	// didn't find any, return null
	return NULL;
}

//! @fn    Frame* findTaggedFrame(bool &activate)
//! @brief Sees if there is a tagged frame and if it's visible
Frame*
WindowManager::findTaggedFrame(bool &activate)
{
	if (_tagged_frame && _tagged_frame->isMapped()) {
		activate = _tag_activate;
		return _tagged_frame;
	}
	return NULL;
}

//! @fn    Frame* findFrameFromId(unsigned int id)
//! @brief
Frame*
WindowManager::findFrameFromId(unsigned int id)
{
	list<Frame*>::iterator it = _frame_list.begin();
	for (; it != _frame_list.end(); ++it) {
		if ((*it)->getId() == id)
			return (*it);
	}

	return NULL;
}

//! @fn    unsigned int findUniqueFrameId(void)
//! @brief
unsigned int
WindowManager::findUniqueFrameId(void)
{
	if (!_startup) // always return 0 while scanning windows
		return 0;

	unsigned int id = 0;

	if (_frameid_list.size()) {
		 id = _frameid_list.back();
		 _frameid_list.pop_back();
	} else {
		id = (_frame_list.size() + 1); // this is called before the frame is pushed
	}

	return id;
}

//! @fn    void setupFrameIds(void)
//! @brief
void
WindowManager::setupFrameIds(void)
{
	list<Frame*>::iterator it = _frame_list.begin();
	for (unsigned int i = 1; it != _frame_list.end(); ++i, ++it)
		(*it)->setId(i);
}

//! @fn    void setTaggedFrame(Frame *frame, bool activate)
//! @brief
void
WindowManager::setTaggedFrame(Frame *frame, bool activate)
{
	_tagged_frame = frame;
	_tag_activate = activate;
}

//! @fn    void attachMarked(Frame *frame)
//! @brief Attaches all marked clients to frame
void
WindowManager::attachMarked(Frame *frame)
{
	list<Client*>::iterator it = _client_list.begin();
	for (; it != _client_list.end(); ++it) {
		if ((*it)->isMarked()) {
			if ((*it)->getFrame() != frame) {
				(*it)->getFrame()->removeClient(*it);
				(*it)->setWorkspace(frame->getWorkspace());
				frame->insertClient(*it, false, false);
			}
			(*it)->mark();
		}
	}
}

//! @fn    void attachInNextPrevFrame(Client *client, bool frame, bool next)
//! @brief Attaches the Client/Frame into the Next/Prev Frame
void
WindowManager::attachInNextPrevFrame(Client *client, bool frame, bool next)
{
	if (!client)
		return;

	Frame *new_frame;

	if (next)
		new_frame = getNextFrame(client->getFrame(), true, SKIP_FOCUS_TOGGLE);
	else
		new_frame = getPrevFrame(client->getFrame(), true, SKIP_FOCUS_TOGGLE);

	if (new_frame) { // we found a frame
		if (frame) {
			new_frame->insertFrame(client->getFrame());
		} else {
			client->getFrame()->setFocused(false);
			client->getFrame()->removeClient(client);
			new_frame->insertClient(client);
		}
	}
}

//! @fn    Frame* getNextFrame(Frame* frame, bool mapped, unsigned int mask)
//! @brief Finds the next frame in the list
//!
//! @param frame Frame to search from
//! @param mapped Match only agains mapped frames
//! @param mask Defaults to 0
Frame*
WindowManager::getNextFrame(Frame* frame, bool mapped, unsigned int mask)
{
	if (!frame || (_frame_list.size() < 2))
		return NULL;

	Frame *next_frame = NULL;
	list<Frame*>::iterator f_it = 
		find(_frame_list.begin(), _frame_list.end(), frame);

	if (f_it != _frame_list.end()) {
		list<Frame*>::iterator n_it = f_it;

		if (++n_it == _frame_list.end())
			n_it = _frame_list.begin();

		while (!next_frame && (n_it != f_it)) {
			if (!(*n_it)->isSkip(mask) && (!mapped || (*n_it)->isMapped()))
				next_frame =  (*n_it);

			if (++n_it == _frame_list.end())
				n_it = _frame_list.begin();
		}
	}

	return next_frame;
}

//! @fn    Frame* getPrevFrame(Frame* frame, bool mapped, unsigned int mask)
//! @brief Finds the previous frame in the list
//!
//! @param frame Frame to search from
//! @param mapped Match only agains mapped frames
//! @param mask Defaults to 0
Frame*
WindowManager::getPrevFrame(Frame* frame, bool mapped, unsigned int mask)
{
	if (!frame || (_frame_list.size() < 2))
		return NULL;

	Frame *next_frame = NULL;
	list<Frame*>::iterator f_it = 
		find(_frame_list.begin(), _frame_list.end(), frame);

	if (f_it != _frame_list.end()) {
		list<Frame*>::iterator n_it = f_it;

		if (n_it == _frame_list.begin())
			n_it = --_frame_list.end();
		else
			--n_it;

		while (!next_frame && (n_it != f_it)) {
			if (!(*n_it)->isSkip(mask) && (!mapped || (*n_it)->isMapped()))
				next_frame =  (*n_it);

			if (n_it == _frame_list.begin())
				n_it = --_frame_list.end();
			else
				--n_it;
		}
	}

	return next_frame;
}

#ifdef MENUS
//! @fn    void hideAllMenus(void)
//! @brief
void
WindowManager::hideAllMenus(void)
{
	map<MenuType, BaseMenu*>::iterator it = _menus.begin();
	for (; it != _menus.end(); ++it)
		it->second->unmapAll();
}
#endif // MENUS

//! @fn    inline void showStatusWindow(void) const
//! @brief Shows and raises the status window.
void
WindowManager::showStatusWindow(void) const
{
	XMapRaised(_screen->getDisplay(), _status_window);
}

//! @fn    inline void hideStatusWindow(void) const
//! @brief Hides the status window
void
WindowManager::hideStatusWindow(void) const
{
	XUnmapWindow(_screen->getDisplay(), _status_window);
}

//! @fn    void drawInStatusWindow(const string &text)
//! @brief Draws status info centered on the current head
//!
//! @param text Status text to draw in status window
//! @param x Position to show the window, defaults to -1 which means centered
//! @param y Position to show the window, defaults to -1 which means centered
//! @todo  Status window padding should be possible to configure
void
WindowManager::drawInStatusWindow(const std::string &text, int x, int y)
{
	if (!text.size())
		return;

	unsigned int width = _theme->getMenuFont()->getWidth(text.c_str()) + 2;
	unsigned int height = _theme->getMenuFont()->getHeight(text.c_str()) + 2;

	// x and y defaults to -1 which means 0x0 on the current head
	if ((x == -1) && (y == -1)) {
#ifdef XINERAMA
		Geometry head;
		_screen->getHeadInfo(_screen->getCurrHead(), head);
		x = head.x + ((head.width - width) / 2);
		y = head.y + ((head.height - height) / 2);
#else // !XINERAMA
		x = (_screen->getWidth() - width) / 2;
		y = (_screen->getHeight() - height) / 2;
#endif // XINERAMA
	}

	XMoveResizeWindow(_screen->getDisplay(), _status_window, x, y, width, height);
	XClearWindow(_screen->getDisplay(), _status_window);

	_theme->getMenuFont()->draw(_status_window, 1, 1, text.c_str());
}

// here follows methods for hints and atoms

void
WindowManager::initHints(void)
{
	// Motif hints
	_atom_mwm_hints =
		XInternAtom(_screen->getDisplay(), "_MOTIF_WM_HINTS", False);

	// Setup Extended Net WM hints
	_extended_hints_win =
			XCreateSimpleWindow(_screen->getDisplay(), _screen->getRoot(),
												  -200, -200, 5, 5, 0, 0, 0);

	XSetWindowAttributes pattr;
	pattr.override_redirect = True;

	XChangeWindowAttributes(_screen->getDisplay(), _extended_hints_win,
													CWOverrideRedirect, &pattr);

	setEwmhHintString(_extended_hints_win, _ewmh_atoms->getAtom(NET_WM_NAME),
									  (char*) _wm_name);
	setAtomLongValue(_extended_hints_win,
									 _ewmh_atoms->getAtom(NET_SUPPORTING_WM_CHECK),
									 _extended_hints_win);
	setAtomLongValue(_screen->getRoot(),
									 _ewmh_atoms->getAtom(NET_SUPPORTING_WM_CHECK),
									 _extended_hints_win);

	setEwmhSupported();
	setAtomLongValue(_screen->getRoot(),
									 _ewmh_atoms->getAtom(NET_NUMBER_OF_DESKTOPS),
									 _config->getWorkspaces());
	setEwmhDesktopGeometry();
	setEwmhDesktopViewport();
	setAtomLongValue(_screen->getRoot(),
									 _ewmh_atoms->getAtom(NET_CURRENT_DESKTOP), 0);
}

//! @fn    long getAtomLongValue(Window win, int atom)
//! @brief
long
WindowManager::getAtomLongValue(Window win, int atom)
{
	Atom real_type;
	int real_format;
	unsigned long items_read, items_left;
	long *data = NULL, value = -1;

	int status =
		XGetWindowProperty(_screen->getDisplay(), win, atom, 0L, 1L, False,
											 XA_CARDINAL, &real_type, &real_format,
											 &items_read, &items_left, (unsigned char **) &data);

	if ((status == Success) && items_read) {
		value = *data;
		XFree(data);
	}

	return value;
}

//! @fn    void setAtomLongValue(Window win, int atom, long value)
//! @brief
void
WindowManager::setAtomLongValue(Window win, int atom, long value)
{
	XChangeProperty(_screen->getDisplay(), win, atom, XA_CARDINAL, 32,
									PropModeReplace, (unsigned char *)&value, 1);
}

void
WindowManager::setEwmhHintString(Window w, int a, char* value)
{
	XChangeProperty(_screen->getDisplay(), w, a, XA_STRING, 8,
									PropModeReplace, (unsigned char*) value, strlen (value));
}

bool
WindowManager::getEwmhHintString(Window w, int a, string &name)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, leftover;
	unsigned char *data = NULL;

	int status =
		XGetWindowProperty(_screen->getDisplay(), w, a, 0L, (long)BUFSIZ,
											 False, XA_STRING, &actual_type, &actual_format,
											 &nitems, &leftover, &data);

	if (status != Success)
		return false;

	if ((actual_type == XA_STRING) && (actual_format == 8)) {
		name = (char*) data;
		XFree((char*) data);
	}

	XFree((char*) data);
	return false;
}

void*
WindowManager::getEwmhPropertyData(Window win, Atom prop,
																	 Atom type, int *items)
{
	Atom type_ret;
	int format_ret;
	unsigned long items_ret;
	unsigned long after_ret;
	unsigned char *prop_data;

	prop_data = 0;

	XGetWindowProperty(_screen->getDisplay(), win, prop, 0, 0x7fffffff, False,
										 type, &type_ret, &format_ret, &items_ret,
										 &after_ret, &prop_data);

	if (items) {
		*items = items_ret;
	}

	return prop_data;
}

bool
WindowManager::getEwmhStates(Window win, NetWMStates &win_state)
{
	int num = 0;
	Atom* states;
	states = (Atom*)
		getEwmhPropertyData(win, _ewmh_atoms->getAtom(STATE), XA_ATOM, &num);

	if (states) {
		if (states[0] == _ewmh_atoms->getAtom(STATE_MODAL))
			win_state.modal = true;
		if (states[1] == _ewmh_atoms->getAtom(STATE_STICKY))
			win_state.sticky = true;
		if (states[2] == _ewmh_atoms->getAtom(STATE_MAXIMIZED_VERT))
			win_state.max_vert = true;
		if (states[3] == _ewmh_atoms->getAtom(STATE_MAXIMIZED_HORZ))
			win_state.max_horz = true;
		if (states[4] == _ewmh_atoms->getAtom(STATE_SHADED))
			win_state.shaded = true;
		if (states[5] == _ewmh_atoms->getAtom(STATE_SKIP_TASKBAR))
			win_state.skip_taskbar = true;
		if (states[6] == _ewmh_atoms->getAtom(STATE_SKIP_PAGER))
			win_state.skip_pager = true;
		if (states[7] == _ewmh_atoms->getAtom(STATE_HIDDEN))
			win_state.hidden = true;
		if (states[8] == _ewmh_atoms->getAtom(STATE_FULLSCREEN))
			win_state.fullscreen = true;
		if (states[9] == _ewmh_atoms->getAtom(STATE_ABOVE))
			win_state.above = true;
		if (states[10] == _ewmh_atoms->getAtom(STATE_BELOW))
			win_state.below = true;

		XFree(states);

		return true;
	} else {
		return false;
	}
}

void
WindowManager::setEwmhSupported(void)
{
	Atom *atoms = _ewmh_atoms->getAtomArray();

	XChangeProperty(_screen->getDisplay(), _screen->getRoot(),
									_ewmh_atoms->getAtom(NET_SUPPORTED), XA_CARDINAL, 32,
									PropModeReplace,
									(unsigned char *) atoms, _ewmh_atoms->size());

	delete [] atoms;
}

void
WindowManager::setEwmhDesktopGeometry(void)
{
	CARD32 geometry[] = { _screen->getWidth(), _screen->getHeight() };

	XChangeProperty(_screen->getDisplay(), _screen->getRoot(),
									_ewmh_atoms->getAtom(NET_DESKTOP_GEOMETRY), XA_CARDINAL, 32,
									PropModeReplace, (unsigned char *)geometry, 2);
}

void
WindowManager::setEwmhDesktopViewport(void)
{
	CARD32 viewport[] = { 0, 0 };

	XChangeProperty(_screen->getDisplay(), _screen->getRoot(),
									_ewmh_atoms->getAtom(NET_DESKTOP_VIEWPORT), XA_CARDINAL, 32,
									PropModeReplace, (unsigned char *)viewport, 2);
}

void
WindowManager::setEwmhActiveWindow(Window w)
{
	XChangeProperty(_screen->getDisplay(), _screen->getRoot(),
									_ewmh_atoms->getAtom(NET_ACTIVE_WINDOW), XA_WINDOW, 32,
									PropModeReplace, (unsigned char *) &w, 1);
}

void
WindowManager::setEwmhWorkArea(void)
{
	int work_x, work_y, work_width, work_height;

	work_x = _screen->getStrut()->left;
	work_y = _screen->getStrut()->top;
	work_width = _screen->getWidth() - work_x - _screen->getStrut()->right;
	work_height = _screen->getHeight() - work_y - _screen->getStrut()->bottom;

	CARD32 workarea[] = { work_x, work_y, work_width, work_height };

	XChangeProperty(_screen->getDisplay(), _screen->getRoot(),
									_ewmh_atoms->getAtom(NET_WORKAREA), XA_CARDINAL, 32,
									PropModeReplace, (unsigned char *)workarea, 4);
}
