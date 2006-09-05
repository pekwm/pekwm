//
// windowmanager.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// windowmanager.cc for aewm++
// Copyright (C) 2000 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
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

#include "pekwm.hh"
#include "windowmanager.hh"
#include "config.hh"
#ifdef KEYS
#include "keys.hh"
#endif // KEYS
#include "frame.hh"
#include "util.hh"

#include <iostream>
#include <list>
#include <algorithm>
#include <functional>

#include <cstdio> // for BUFSIZ
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <X11/Xatom.h>
#include <X11/cursorfont.h>

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::vector;
using std::list;
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
m_screen(NULL),
#ifdef KEYS
m_keys(NULL),
#endif // KEYS
m_config(NULL),
m_theme(NULL), m_action_handler(NULL),
m_autoprops(NULL), m_workspaces(NULL),
#ifdef HARBOUR
m_harbour(NULL),
#endif // HARBOUR
dpy(NULL), root(None),
#ifdef MENUS
m_windowmenu(NULL), m_iconmenu(NULL), m_rootmenu(NULL),
#endif // MENUS
m_focused_client(NULL), m_wm_name("pekwm"),
m_active_workspace(0), m_command_line(command_line),
m_status_window(None),
m_master_strut(NULL),
m_icccm_atoms(NULL), m_ewmh_atoms(NULL)
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

	setupDisplay();
	scanWindows();

#ifdef MENUS
	updateIconMenu();
#endif // MENUS

	execStartScript();

	doEventLoop();
}

void
WindowManager::execStartScript(void)
{
	struct stat stat_buf;

	string start_file = m_config->getStartFile();
	bool exec = false;

	if (start_file.size()) {
		if (!stat(start_file.c_str(), &stat_buf)) {
			exec = Util::isExecutable(start_file);
		} else {
			start_file = DATADIR "/start";
			if (!stat(start_file.c_str(), &stat_buf)) {
				exec = Util::isExecutable(start_file);
			}
		}
	} else {
		start_file = DATADIR "/start";
		if (!stat(start_file.c_str(), &stat_buf)) {
			exec = Util::isExecutable(start_file);
		}
	}

	if (exec) {
		Util::forkExec(start_file);
	}
}

//! @fn    void cleanup(void);
//! @brief Cleans up, Maps windows etc.
void
WindowManager::cleanup(void)
{
	unsigned int num_wins;
	Window dummy_win1, dummy_win2, *wins;
	Client *client;

	// Preserve stacking order when removing the clients from the list.
	XQueryTree(dpy, root, &dummy_win1, &dummy_win2, &wins, &num_wins);
	for (unsigned i = 0; i < num_wins; ++i) {
		if((client = findClient(wins[i])) ) {
			XMapWindow(dpy, client->getWindow());
			delete client;
		}
	}
	XFree(wins);
	m_client_list.clear();

#ifdef KEYS
	m_keys->ungrabKeys(root);
#endif // KEYS

	// destroy status window
	XDestroyWindow(dpy, m_status_window);

	// destroy atom windows
	XDestroyWindow(dpy, m_extended_hints_win);

	for (unsigned int i = 0; i < NUM_CURSORS; ++i)
		XFreeCursor(dpy, m_cursors[i]);

	XInstallColormap(dpy, m_screen->getColormap());
	XSetInputFocus(dpy, PointerRoot, RevertToNone, CurrentTime);

#ifdef HARBOUR
	m_harbour->removeAllDockApps();
#endif // HARBOUR

	XCloseDisplay(dpy);
}

// The destructor cleans up allocated memory and exits cleanly. Hopefully! =)
WindowManager::~WindowManager()
{
#ifdef HARBOUR
	if (m_harbour) {
		delete m_harbour;
	}
#endif // HARBOUR

	if (m_action_handler)
		delete m_action_handler;
	if (m_autoprops)
		delete m_autoprops;

#ifdef MENUS
	if (m_windowmenu)
		delete m_windowmenu;
	if (m_iconmenu)
		delete m_iconmenu;
	if (m_rootmenu)
		delete m_rootmenu;
#endif // MENUS

	if (m_master_strut)
		delete m_master_strut;

	if (m_icccm_atoms)
		delete m_icccm_atoms;
	if (m_ewmh_atoms)
		delete m_ewmh_atoms;

#ifdef KEYS
	if (m_keys)
		delete m_keys;
#endif // KEYS

	if (m_workspaces)
		delete m_workspaces;
	if (m_config)
		delete m_config;
	if (m_theme)
		delete m_theme;
	if (m_screen)
		delete m_screen;
}

//! @fn    void setupDisplay(void)
//! @brief
void
WindowManager::setupDisplay(void)
{
	dpy = XOpenDisplay(NULL);
	if (!dpy) {
		cerr << "Can not open display!" << endl
				 << "Your DISPLAY variable currently is set to: "
				 << getenv("DISPLAY") << endl;
		exit(1);
	}

	XSetErrorHandler(handleXError);

	m_screen = new ScreenInfo(dpy);
	root = m_screen->getRoot();

	m_action_handler = new ActionHandler(this);

	// get config, needs to be done _BEFORE_ hints
	m_config = new Config;

	// load colors, fonts
	m_theme = new Theme(m_config, m_screen);

	m_autoprops = new AutoProps(m_config);
	m_autoprops->loadConfig();

	m_master_strut = new Strut;

	m_icccm_atoms = new IcccmAtoms(m_screen->getDisplay());
	m_ewmh_atoms = new EwmhAtoms(m_screen->getDisplay());
	initHints();

#ifdef HARBOUR
	m_harbour = new Harbour(m_screen, m_config, m_theme);
	m_workspaces = new Workspaces(m_screen, m_config, m_ewmh_atoms,
																m_config->getWorkspaces(), m_harbour);
#else // !HARBOUR
	m_workspaces = new Workspaces(m_screen, m_config, m_ewmh_atoms,
																m_config->getWorkspaces());
#endif // HARBOUR

#ifdef SHAPE
	int dummy_error;
	m_has_shape = XShapeQueryExtension(dpy, &m_shape_event, &dummy_error);
#endif

	// Create resize cursors
	m_cursors[TOP_LEFT_CURSOR] = XCreateFontCursor(dpy, XC_top_left_corner);
	m_cursors[TOP_CURSOR] = XCreateFontCursor(dpy, XC_top_side);
	m_cursors[TOP_RIGHT_CURSOR] = XCreateFontCursor(dpy, XC_top_right_corner);
	m_cursors[LEFT_CURSOR] = XCreateFontCursor(dpy, XC_left_side);
	m_cursors[RIGHT_CURSOR] = XCreateFontCursor(dpy, XC_right_side);
	m_cursors[BOTTOM_LEFT_CURSOR] =
		XCreateFontCursor(dpy, XC_bottom_left_corner);
	m_cursors[BOTTOM_CURSOR] = XCreateFontCursor(dpy, XC_bottom_side);
	m_cursors[BOTTOM_RIGHT_CURSOR] =
		XCreateFontCursor(dpy, XC_bottom_right_corner);
	// Create other cursors
	m_cursors[ARROW_CURSOR] = XCreateFontCursor(dpy, XC_left_ptr);
	m_cursors[MOVE_CURSOR] =  XCreateFontCursor(dpy, XC_fleur);
	m_cursors[RESIZE_CURSOR] = XCreateFontCursor(dpy, XC_plus);

	XDefineCursor(dpy, root, m_cursors[ARROW_CURSOR]);

	XSetWindowAttributes sattr;
	sattr.event_mask =
		SubstructureRedirectMask|SubstructureNotifyMask|
		ColormapChangeMask|FocusChangeMask|
		EnterWindowMask|
		PropertyChangeMask|
		ButtonPressMask|ButtonReleaseMask;

	XChangeWindowAttributes(dpy, root, CWEventMask, &sattr);

#ifdef KEYS
	m_keys = new Keys(m_config, m_screen);
	m_keys->grabKeys(root);
#endif // KEYS

#ifdef MENUS
	m_iconmenu = new IconMenu(this, m_screen, m_theme);
	m_windowmenu = new WindowMenu(this);
	m_rootmenu = new RootMenu(this);
#endif // MENUS

	m_status_window =
		XCreateSimpleWindow(dpy, root, 0, 0, 1, 1,
												m_theme->getMenuBorderWidth(),
												m_theme->getMenuBorderColor().pixel,
												m_theme->getMenuBackground().pixel);
}

//! @fn    void scanWindows(void)
//! @brief Goes through the window and creates Clients/DockApps.
void
WindowManager::scanWindows(void)
{
	unsigned int num_wins;
	Window dummy_win1, dummy_win2, *wins;
	XWindowAttributes attr;

	// Lets create a list of windows on the display
	XQueryTree(dpy, root, &dummy_win1, &dummy_win2, &wins, &num_wins);
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

		XWMHints *wm_hints = XGetWMHints(dpy, *it);
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

	for (it = win_list.begin(); it != win_list.end(); ++it) {
		if (*it == None)
			continue;

		XGetWindowAttributes(dpy, *it, &attr);
		if (!attr.override_redirect && (attr.map_state != IsUnmapped)) {
#ifdef HARBOUR
			XWMHints *wm_hints = XGetWMHints(dpy, *it);
			if (wm_hints) {
				if ((wm_hints->flags&StateHint) &&
						(wm_hints->initial_state == WithdrawnState)) {
					m_harbour->addDockApp(new DockApp(m_screen, m_theme, *it));
				} else {
					new Client(this, *it);
				}
				XFree(wm_hints);
			} else
#endif // HARBOUR
				{
					// The client will put itself in the client list
					new Client(this, *it);
				}
		}
	}

	// Try to focus the ontop window, if no window we give root focus
	Client *client = m_workspaces->getTopClient(m_active_workspace);
	if (client && !client->isIconified())
		client->giveInputFocus();
	else
		XSetInputFocus(dpy, m_screen->getRoot(), RevertToNone, CurrentTime);
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
	unsigned int old_harbour_placement = m_config->getHarbourPlacement();
	bool old_harbour_stacking = m_config->getHarbourOntop();
#endif // HARBOUR

	m_config->load(); // reload the config
	m_autoprops->loadConfig(); // reload autoprops config

	// update what might have changed in the cfg toouching the hints
	setNumWorkspaces(m_config->getWorkspaces());
	m_workspaces->updateClientList(); // Reflect reportOnlyFrames

	// reload the theme
	m_theme->setThemeDir(m_config->getThemeFile());
	m_theme->load();

	// update the status window theme
	XSetWindowBackground(dpy, m_status_window,
											 m_theme->getMenuBackground().pixel);
	XSetWindowBorder(dpy, m_status_window, m_theme->getMenuBorderColor().pixel);
	XSetWindowBorderWidth(dpy, m_status_window, m_theme->getMenuBorderWidth());

	// reload the themes on the frames
	for_each(m_frame_list.begin(), m_frame_list.end(),
					 mem_fun(&Frame::loadTheme));

#ifdef KEYS
 // reload the keygrabber
	m_keys->loadKeys();

	m_keys->ungrabKeys(m_screen->getRoot());
	m_keys->grabKeys(m_screen->getRoot());
#endif // KEYS

	//regrab keys AND load autoprops
	vector<Client*>::iterator c_it = m_client_list.begin();
	for (; c_it != m_client_list.end(); ++c_it) {
#ifdef KEYS
		m_keys->ungrabKeys((*c_it)->getWindow());
		m_keys->grabKeys((*c_it)->getWindow());
#endif // KEYS
		(*c_it)->loadAutoProps(AutoProps::APPLY_ON_RELOAD);
	}

#ifdef HARBOUR
	m_harbour->loadTheme();
	if (old_harbour_placement != m_config->getHarbourPlacement())
		m_harbour->rearrange();
	if (old_harbour_stacking != m_config->getHarbourOntop())
		m_harbour->restack();
#endif // HARBOUR

#ifdef MENUS
	// fix the menus
	m_windowmenu->loadTheme();
	m_iconmenu->loadTheme();
	m_rootmenu->loadTheme();

	// reload menus
	m_windowmenu->updateMenu();
	m_iconmenu->updateMenu();
	m_rootmenu->updateRootMenu();
#ifdef HARBOUR
	m_harbour->getHarbourMenu()->loadTheme();
	m_harbour->getHarbourMenu()->updateMenu();
#endif // HARBOUR

#endif // MENUS
}

//! @fn    void restart(const string &command)
//! @breif Exit pekwm and restart with the command command
void
WindowManager::restart(string command)
{
	if (!command.size())
		command = m_command_line;

	cleanup();
	execlp("/bin/sh", "sh" , "-c", command.c_str(), 0);
}

// Event handling routins beneath this =====================================

void
WindowManager::doEventLoop(void)
{
	Display *dpy = m_screen->getDisplay();
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

#ifdef SHAPE
		default:
			if ((client = findClient(ev.xany.window))) {
				if (m_has_shape && (ev.type == m_shape_event)) {
					client->handleShapeChange((XShapeEvent *) &ev);
				}
			}
			break;
#endif
		}
	}
}

//! @fn    void handleKeyEvent(XKeyEvent *ev)
//! @brief Handle XKeyEvents
void
WindowManager::handleKeyEvent(XKeyEvent *ev)
{
#ifdef KEYS
	if (ev->type == KeyPress) {
		Action *action = m_keys->getActionFromKeyEvent(ev);
		if (action)
			m_action_handler->handleAction(action, getFocusedClient());

	}
#endif // KEYS
}

void
WindowManager::handleButtonPressEvent(XButtonEvent *ev)
{
	Action *action = NULL;
#ifdef MENUS
	BaseMenu *mu = NULL;
#endif // MENUS

	// First handle root window actions
	if (ev->window == m_screen->getRoot()) {
		if ((action = findMouseButtonAction(ev->button))) {
			m_action_handler->handleAction(action, NULL);
		}
	} else {
		Client *client = findClient(ev->window);
		if (client) {
			// We do this to make the action go to the correct client
			client = client->getFrame()->getActiveClient();

			// handle window focusing, always focus by clicking on the
			// window with Button1.
			// TO-DO: Make this configurable?
			if ((client != m_focused_client) && (ev->button == Button1))
				client->giveInputFocus();

			action = client->getFrame()->handleButtonEvent(ev);
			if (action)
				m_action_handler->handleAction(action, client);
		}
#ifdef MENUS
		// handle menu clicks
		else if ((mu = m_windowmenu->findMenu(ev->window)) ||
						 (mu = m_iconmenu->findMenu(ev->window)) ||
						 (mu = m_rootmenu->findMenu(ev->window))
#ifdef HARBOUR
						 || (mu = m_harbour->getHarbourMenu()->findMenu(ev->window))
#endif // HARBOUR
						 ) {

			mu->handleButtonPressEvent(ev);
		}
#endif // MENUS

#ifdef HARBOUR
		else {
			DockApp *da = m_harbour->findDockAppFromFrame(ev->window);
			if (da) {
				m_harbour->handleButtonEvent(ev, da);
			}
		}
#endif // HARBOUR
	}
}

void
WindowManager::handleButtonReleaseEvent(XButtonEvent *ev)
{
#ifdef MENUS
	BaseMenu *mu = NULL;
#endif // MENUS

	Client *client = findClient(ev->window);
	if (client) {
		Action* action = client->getFrame()->handleButtonEvent(ev);
		if (action)
			m_action_handler->handleAction(action, client);
	}
#ifdef MENUS
	else if ((mu = m_windowmenu->findMenu(ev->window)) ||
					 (mu = m_iconmenu->findMenu(ev->window)) ||
					 (mu = m_rootmenu->findMenu(ev->window))
#ifdef HARBOUR
					 || (mu = m_harbour->getHarbourMenu()->findMenu(ev->window))
#endif // HARBOUR
					 ) {
		mu->handleButtonReleaseEvent(ev);
	}
#endif // MENUS

#ifdef HARBOUR
	else {
		DockApp *da = m_harbour->findDockAppFromFrame(ev->window);
		if (da) {
			m_harbour->handleButtonEvent(ev, da);
		}
	}
#endif // HARBOUR
}

void
WindowManager::handleConfigureRequestEvent(XConfigureRequestEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		client->handleConfigureRequest(ev);

	} else {
#ifdef HARBOUR
		DockApp *da = m_harbour->findDockApp(ev->window);
		if (da) {
			m_harbour->handleConfigureRequestEvent(ev, da);
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

				XConfigureWindow(m_screen->getDisplay(), ev->window,
												 ev->value_mask, &wc);
			}
	}
}

void
WindowManager::handleMotionEvent(XMotionEvent *ev)
{
	Client *client = findClient(ev->window);
#ifdef MENUS
	BaseMenu *mu = NULL;
#endif // MENUS

	// First try to operate on a client
	if (client) {
		Action *action = client->getFrame()->handleMotionNotifyEvent(ev);
		if (action)
			m_action_handler->handleAction(action, client);
	}
	// Else we try to find a menu
#ifdef MENUS
	else if ((mu = m_windowmenu->findMenu(ev->window)) ||
					 (mu = m_iconmenu->findMenu(ev->window)) ||
					 (mu = m_rootmenu->findMenu(ev->window))
#ifdef HARBOUR
					 || (mu = m_harbour->getHarbourMenu()->findMenu(ev->window))
#endif // HARBOUR
					 ) {
		mu->handleMotionNotifyEvent(ev);
	}
#endif // MENUS

#ifdef HARBOUR
	else {
		DockApp *da = m_harbour->findDockAppFromFrame(ev->window);
		if (da) {
			m_harbour->handleMotionNotifyEvent(ev, da);
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
		XGetWindowAttributes(dpy, ev->window, &attr);
		if (!attr.override_redirect) {
			// if we have the harbour enabled, we need to figure out wheter or
			// not this is a dockapp.
#ifdef HARBOUR
			XWMHints *wm_hints = XGetWMHints(dpy, ev->window);
			if (wm_hints) {
				if ((wm_hints->flags&StateHint) &&
						(wm_hints->initial_state == WithdrawnState)) {
#ifdef DEBUG
					cerr << __FILE__ << "@" << __LINE__ << ": "
							 << "DockApp Mapping: " << ev->window << endl;
#endif // DEBUG
					m_harbour->addDockApp(new DockApp(m_screen, m_theme, ev->window));
				} else {
					client = new Client(this, ev->window);
					client->giveInputFocus();
				}
				XFree(wm_hints);
			} else
#endif // HARBOUR
				{
					client = new Client(this, ev->window);
					client->giveInputFocus();
				}
		}
	}
}

void
WindowManager::handleUnmapEvent(XUnmapEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		Window frame_win = client->getFrame()->getFrameWindow();
		client->handleUnmapEvent(ev);

		if (!m_focused_client)
			findClientAndFocus(frame_win);
	}
#ifdef HARBOUR
	else {
		DockApp *da = m_harbour->findDockApp(ev->window);
		if (da) {
			if (ev->window == ev->event) {
#ifdef DEBUG
				cerr << __FILE__ << "@" << __LINE__ << ": "
						 << "Unmapping DockaApp: " << da << endl;
#endif // DEBUG
				m_harbour->removeDockApp(da);
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
		Window frame_win = client->getFrame()->getFrameWindow();
		client->handleDestroyEvent(ev);

		if (!m_focused_client)
			findClientAndFocus(frame_win);
	}
#ifdef HARBOUR
	else {
		DockApp *da = m_harbour->findDockApp(ev->window);
		if (da) {
#ifdef DEBUG
			cerr << __FILE__ << "@" << __LINE__ << ": "
					 << "Destroying DockApp: " << da << endl;
#endif // DEBUG
			da->setAlive(false);
			m_harbour->removeDockApp(da);
		}
	}
#endif // HARBOUR
}

void
WindowManager::handleEnterNotify(XCrossingEvent *ev)
{
	if (m_config->getFocusModel() == FOCUS_CLICK)
		return;

	// We ignore the NotifyUngrab, it's caused be the keygrabber.
	// The NotifyInferior is caused when clicking on the window.
	if ((ev->mode == NotifyUngrab) || (ev->detail == NotifyInferior))
		return;

	Client *client = findClient(ev->window);
	if (client) {
		client = client->getFrame()->getActiveClient();
		if (client != m_focused_client) {
			client->giveInputFocus();
		}
	} else if (m_config->getFocusModel() == FOCUS_FOLLOW)
		XSetInputFocus(dpy, m_screen->getRoot(), RevertToPointerRoot, CurrentTime);
}

void
WindowManager::handleLeaveNotify(XCrossingEvent *ev)
{
#ifdef MENUS
	BaseMenu *mu;

	if ((mu = m_windowmenu->findMenu(ev->window)) ||
			(mu = m_iconmenu->findMenu(ev->window)) ||
			(mu = m_rootmenu->findMenu(ev->window))
#ifdef HARBOUR
			|| (mu = m_harbour->getHarbourMenu()->findMenu(ev->window))
#endif // HARBOUR
			) {
		mu->handleLeaveEvent(ev);
	}
#endif // MENUS
}

//! @fn    void handleFocusInEvent(XFocusChangeEvent *ev)
//! @brief Handles focusing of windows.
void
WindowManager::handleFocusInEvent(XFocusChangeEvent *ev)
{
	// We ignore the NotifyUngrab, it's caused be the keygrabber.
	// The NotifyPointer happens when hiding/unhiding clients.
	if ((ev->mode == NotifyUngrab) || (ev->detail == NotifyPointer))
		return;

	Client *client = findClientFromWindow(ev->window);
	if (client && (client != m_focused_client)) {
		if (m_focused_client) {
			m_focused_client->setFocus(false);
			m_workspaces->setLastFocusedClient(m_focused_client, m_active_workspace);
		}

		m_focused_client = client;
		m_focused_client->setFocus(true);
	}
}

void
WindowManager::handleClientMessageEvent(XClientMessageEvent *ev)
{
	// Handle root window messages
	if (ev->window == m_screen->getRoot()) {
		if (ev->format == 32) { // format 32 events
			if (ev->message_type == m_ewmh_atoms->net_current_desktop) {
				setWorkspace(ev->data.l[0], true);

			} else if (ev->message_type == m_ewmh_atoms->net_number_of_desktops) {
				if (ev->data.l[0] > 0) {
					setNumWorkspaces(ev->data.l[0]);
				}
			}
		}
	} else {
		Client *client = findClientFromWindow(ev->window);
		if (client) {
			client->handleClientMessage(ev);
		}
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
	if (ev->window == m_screen->getRoot())
		return;

	Client *client = findClientFromWindow(ev->window);

	if (client) {
		client->handlePropertyChange(ev);
	}
}

void
WindowManager::handleExposeEvent(XExposeEvent *ev)
{
#ifdef MENUS
	BaseMenu *mu = NULL;
	if ((mu = m_windowmenu->findMenu(ev->window)) ||
			(mu = m_iconmenu->findMenu(ev->window)) ||
			(mu = m_rootmenu->findMenu(ev->window))
#ifdef HARBOUR
			|| (mu = m_harbour->getHarbourMenu()->findMenu(ev->window))
#endif // HARBOUR
			) {

		mu->handleExposeEvent(ev);
	}
#endif // MENUS
}

Action*
WindowManager::findMouseButtonAction(unsigned int button)
{
	list<MouseButtonAction> *actions = m_config->getMouseRootList();
	list<MouseButtonAction>::iterator it = actions->begin();
	for (; it != actions->end(); ++it) {
		if ((it->button == button) && (it->type == BUTTON_SINGLE)) {
			return &*it;
		}
	}

	return NULL;
}

// Event handling routines stop ============================================


//! @fn    void findClientAndFocus(Window win)
//! @brief Searches for a Client to focus, and then gives it input focus.
void
WindowManager::findClientAndFocus(Window win)
{
	Client *client = NULL;

	if (win != None) {
		client = findClient(win);
		if (client && !client->isHidden())
			client = client->getFrame()->getActiveClient();
		else
			client = NULL;
	}

	if (!client) {
		client = m_workspaces->getLastFocusedClient(m_active_workspace);

		if (client) {
			vector<Client*>::iterator it =
				find(m_client_list.begin(), m_client_list.end(), client);

			if (it == m_client_list.end() || client->isHidden() ||
					!client->isSticky() &&
					(client->onWorkspace() != signed(m_active_workspace))) {
				client = NULL;
			}
		}

		if (!client) {
			client = m_workspaces->getTopClient(m_active_workspace);
			if (client && client->isHidden()) {
				client = NULL;
			}
		}
	}

	if (client)
		client->giveInputFocus();
}

//! @fn    void getMousePosition(int *x, int *y)
//! @brief Sets x and y to the current mouse position.
//! @param x Pointer to int where to store x position
//! @param y Pointer to int where to store y position
void
WindowManager::getMousePosition(int *x, int *y)
{
	Window m_root, m_win;
	int win_x = 0, win_y = 0;
	unsigned int mask;

	XQueryPointer(dpy, root, &m_root, &m_win, x, y, &win_x, &win_y, &mask);
}

//! @fn    void focusNextFrame(void)
//! @brief Tries to find the "next" frame relative to the focused client.
//! @todo Include support for sticky clients
void
WindowManager::focusNextFrame(void)
{
	list<Client*> *clients = m_workspaces->getClientList(m_active_workspace);
	if (clients->size() < 2)
		return; // no clients to play with

	Client *client_to_focus = NULL;

	if (!client_to_focus && m_focused_client) {
		list<Client*>::iterator it =
			find(clients->begin(), clients->end(), m_focused_client);

		if (it != clients->end()) {
			Frame* client_frame = (*it)->getFrame();
			list<Client*>::iterator n_it;

			while (++n_it != it) {
				if (n_it == clients->end()) {
					n_it = clients->begin();
					--n_it;
				} else {
					if (((*n_it)->getFrame() != client_frame) &&
							((*n_it)->getFrame()->getActiveClient() == (*n_it)) &&
							((*n_it)->getLayer() == WIN_LAYER_NORMAL) &&
							!(*n_it)->isIconified()) {
						client_to_focus = *n_it;
						break;
					}
				}
			}
		} else {
			client_to_focus = clients->back();
		}
		// if we don't have any focused client, let's focus the one at the top
	} else {
		client_to_focus = clients->back();
	}

	if (client_to_focus) {
		client_to_focus->getFrame()->raise();
		client_to_focus->giveInputFocus();
	}
}

//! @fn    Client* findClientFromWindow(Window w)
//! @brief Finds the Client wich holds the window w, only matching the window.
//! @param w Window to use as criteria when searching
Client*
WindowManager::findClientFromWindow(Window w)
{
	if ((w == root) || (!m_client_list.size()))
		return NULL;

	vector<Client*>::iterator it = m_client_list.begin();
	for(; it != m_client_list.end(); ++it) {
		if (w == (*it)->getWindow()) {
			return (*it);
		}
	}

	return NULL;
}

//! @fn    void setWorkspace(unsigned int workspace)
//! @brief Activates Workspace workspace and sets the right hints
//! @param workspace Workspace to activate
void
WindowManager::setWorkspace(unsigned int workspace, bool focus)
{
	if ((workspace == m_active_workspace) ||
			(workspace >= m_workspaces->getNumWorkspaces()))
		return;

	XGrabServer(dpy);

	m_workspaces->setLastFocusedClient(m_focused_client, m_active_workspace);
	if (m_focused_client) {
		m_focused_client->setFocus(false);
		m_focused_client = NULL;
	}

	setExtendedWMHint(root, m_ewmh_atoms->net_current_desktop, workspace);
	m_workspaces->activate(workspace, focus);

	m_active_workspace = workspace;

	XSync(dpy, false);
	XUngrabServer(dpy);
}

void
WindowManager::setNumWorkspaces(unsigned int num)
{
	if (num < 1)
		return;

	m_workspaces->setNumWorkspaces(num);

	// We don't want to be on a non existing workspace
	if (num <= m_active_workspace)
		setWorkspace(num - 1, true);

	// TO-DO: activate the new active workspace once again, to make sure
	// all clients are visible
#ifdef MENUS
	m_windowmenu->updateWindowMenu();
#endif // MENUS
}

//! @fn    Client* findClient(window w)
//! @brief Finds the Client wich holds the Window w.
//! @param w Window to use as criteria when searching.
//! @return Pointer to the client if found, else NULL
Client*
WindowManager::findClient(Window w)
{
	if (!w || w == root)
		return NULL;

	if(m_client_list.size()) {

		vector<Client*>::iterator it = m_client_list.begin();
		for(; it < m_client_list.end(); it++) {
			if (w == (*it)->getWindow()) {
				return (*it);

			} else if ((*it)->getFrame() &&
					((w == (*it)->getFrame()->getTitleWindow()) ||
					 (w == (*it)->getFrame()->getFrameWindow()) ||
					 ((*it)->getFrame()->findButton(w)) )) {
				return (*it);
			}
		}
	}

	return NULL;
}

//! @fn    void findTransientToMapOrUnmap(Window win, bool hide)
//! @brief
//! @param win
//! @param hide
void
WindowManager::findTransientsToMapOrUnmap(Window win, bool hide)
{
	if (!m_client_list.size())
		return;

	vector<Client*>::iterator it = m_client_list.begin();
	for (; it != m_client_list.end(); ++it) {
		if ((*it)->getTransientWindow() == win) {
			if (hide) {
				if (!(*it)->isIconified()) {
					(*it)->iconify();
				}
			} else if((*it)->isIconified()) {
				if ((*it)->onWorkspace() != (signed) m_active_workspace) {
					(*it)->setWorkspace(m_active_workspace);
				}

				(*it)->getFrame()->unhideClient((*it));
			}
		}
	}
}

void
WindowManager::removeFromClientList(Client *client)
{
	if (!m_client_list.size())
		return;

	if (client == m_focused_client)
		m_focused_client = NULL;

	vector<Client*>::iterator it =
		find(m_client_list.begin(), m_client_list.end(), client);

	if (it != m_client_list.end())
		m_client_list.erase(it);
}

void
WindowManager::addStrut(Strut *strut)
{
	if (! strut)
		return;

	m_strut_list.push_back(strut);

	vector<Strut*>::iterator it = m_strut_list.begin();
	for(; it != m_strut_list.end(); ++it) {
		if(m_master_strut->east < (*it)->east)
			m_master_strut->east = (*it)->east;

		if(m_master_strut->west < (*it)->west)
			m_master_strut->west = (*it)->west;

		if(m_master_strut->north < (*it)->north)
			m_master_strut->north = (*it)->north;

		if(m_master_strut->south < (*it)->south)
			m_master_strut->south = (*it)->south;
	}

	setExtendedNetWorkArea();
}

void
WindowManager::removeStrut(Strut *strut)
{
	if (!strut || !m_strut_list.size())
		return;

	vector<Strut*>::iterator it = m_strut_list.begin();
	for (; it < m_strut_list.end(); it++) {
		if ((*it) == strut) {
			m_strut_list.erase(it);
			break;
		}
	}

	m_master_strut->east = 0;
	m_master_strut->west = 0;
	m_master_strut->north = 0;
	m_master_strut->south = 0;

	if(m_strut_list.size()) {

		for(it = m_strut_list.begin(); it < m_strut_list.end(); it++) {
			if(m_master_strut->east < (*it)->east)
				m_master_strut->east = (*it)->east;
			if(m_master_strut->west < (*it)->west)
				m_master_strut->west = (*it)->west;
			if(m_master_strut->north < (*it)->north)
				m_master_strut->north = (*it)->north;
			if(m_master_strut->south < (*it)->south)
				m_master_strut->south = (*it)->south;
		}
	}

	setExtendedNetWorkArea();
}

#ifdef MENUS
void
WindowManager::addClientToIconMenu(Client *c)
{
	if (! c)
		return;

	m_iconmenu->hide();
	m_iconmenu->addThisClient(c);
	m_iconmenu->updateIconMenu();
}

void
WindowManager::removeClientFromIconMenu(Client *c)
{
	if (!c)
		return;

	m_iconmenu->hide();
	m_iconmenu->removeClientFromIconMenu(c);
	m_iconmenu->updateIconMenu();
}

void
WindowManager::updateIconMenu(void)
{
	m_iconmenu->hide();
	m_iconmenu->removeAll();

	// if we don't have any clients, noone will be iconified
	if (m_client_list.size()) {
		vector<Client*>::iterator it = m_client_list.begin();
		for(; it < m_client_list.end(); it++) {
			if ((*it)->isIconified()) {
				m_iconmenu->addThisClient((*it));
			}
		}
	}

	m_iconmenu->updateIconMenu();
}
#endif // MENUS

Frame*
WindowManager::findGroup(const AutoProps::ClassHint &class_hint,
												 unsigned int desktop, unsigned int max)
{
	if (! m_frame_list.size())
		return NULL;

	Client *client;

	list<Frame*>::iterator it = m_frame_list.begin();
	for (; it != m_frame_list.end(); ++it) {
		client = (*it)->getActiveClient(); // convinience

		if (((client->onWorkspace() == (signed) desktop) ||
				client->isSticky()) &&
				((*it)->getNumClients() < max) &&
				(class_hint == client->getClassHint()) &&
				(*it)->allClientsAreOfSameClass())
			return *it;
	}

	return NULL;
}

Client*
WindowManager::findClient(const AutoProps::ClassHint &class_hint)
{
	if (! m_client_list.size())
		return NULL;

	vector<Client*>::iterator it = m_client_list.begin();

	for (; it != m_client_list.end(); ++it) {
		if ((*it)->getClassHint() == class_hint) {
			return *it;
		}
	}

	return NULL;
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
	if (! text.size())
		return;

	unsigned int width = m_theme->getMenuFont()->getWidth(text) + 2;
	unsigned int height = m_theme->getMenuFont()->getHeight(text) + 2;

	// x and y defaults to -1 which means centered on the current head
	if ((x == -1) && (y == -1)) {
#ifdef XINERAMA
		ScreenInfo::HeadInfo head;
		unsigned int head_nr = 0;

		if (m_screen->hasXinerama())
			head_nr = m_screen->getCurrHead();
		m_screen->getHeadInfo(head_nr, head);

		x = head.x + ((head.width - width) / 2);
		y = head.y + ((head.height - height) / 2);
#else // !XINERAMA
		x = (m_screen->getWidth() - width) / 2;
		y = (m_screen->getHeight() - height) / 2;
#endif // XINERAMA
	}

	XMoveResizeWindow(dpy, m_status_window, x, y, width, height);
	XClearWindow(dpy, m_status_window);

	m_theme->getMenuFont()->draw(m_status_window, 1, 1, text);
}




// here follows methods for hints and atoms

void
WindowManager::initHints(void)
{
	// Motif hints
	m_atom_mwm_hints = XInternAtom(dpy, "_MOTIF_WM_HINTS", False);

	// Setup Extended Net WM hints
	m_extended_hints_win = XCreateSimpleWindow(dpy, m_screen->getRoot(),
																						 -200, -200, 5, 5, 0, 0, 0);

	XSetWindowAttributes pattr;
	pattr.override_redirect = True;

	XChangeWindowAttributes(dpy, m_extended_hints_win,
													CWOverrideRedirect, &pattr);

	setExtendedWMHintString(m_extended_hints_win, m_ewmh_atoms->net_wm_name,
													m_wm_name);
	setExtendedWMHint(m_extended_hints_win,
										m_ewmh_atoms->net_supporting_wm_check,
										m_extended_hints_win);
	setExtendedWMHint(m_screen->getRoot(),
										m_ewmh_atoms->net_supporting_wm_check,
										m_extended_hints_win);

	setExtendedNetSupported();
	setExtendedWMHint(m_screen->getRoot(), m_ewmh_atoms->net_number_of_desktops,
										m_config->getWorkspaces());
	setExtendedNetDesktopGeometry();
	setExtendedNetDesktopViewport();
	setExtendedWMHint(m_screen->getRoot(), m_ewmh_atoms->net_current_desktop, 0);
}

long
WindowManager::getHint(Window w, int a)
{
	Atom real_type;
	int real_format;
	unsigned long items_read, items_left;
	long *data=NULL, value=0;

	if(XGetWindowProperty(dpy, w, a, 0L, 1L, False,
												XA_CARDINAL, &real_type,
												&real_format, &items_read,
												&items_left,
												(unsigned char **)&data)==Success && items_read) {
		value = *data;
		XFree(data);
	}

	return value;
}

int
WindowManager::sendExtendedHintMessage(Window w, Atom a,
																			 long mask, long int data[])
{
	XEvent e;

	e.type = ClientMessage;
	e.xclient.window = w;
	e.xclient.message_type = a;
	e.xclient.format = 32;

	// xclient.data.l is a long int[5]
	e.xclient.data.l[0] = data[0];
	e.xclient.data.l[1] = data[1];
	e.xclient.data.l[2] = data[2];
	//e.xclient.data.l[3] = data[3];
	//e.xclient.data.l[4] = data[4];

	return XSendEvent(dpy, w, False,
										SubstructureNotifyMask|SubstructureRedirectMask, &e);
}

void
WindowManager::setExtendedWMHint(Window w, int a, long value)
{
	XChangeProperty(dpy, w, a, XA_CARDINAL, 32,
									PropModeReplace, (unsigned char *)&value, 1);
}

void
WindowManager::setExtendedWMHintString(Window w, int a, char* value)
{
	XChangeProperty(dpy, w, a, XA_STRING, 8,
									PropModeReplace, (unsigned char*) value, strlen (value));
}

bool
WindowManager::getExtendedWMHintString(Window w, int a, string &name)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, leftover;
	unsigned char *data = NULL;

	int status =
		XGetWindowProperty(dpy, w, a, 0L, (long)BUFSIZ,
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
WindowManager::getExtendedNetPropertyData(Window win, Atom prop,
																					Atom type, int *items)
{
	Atom type_ret;
	int format_ret;
	unsigned long items_ret;
	unsigned long after_ret;
	unsigned char *prop_data;

	prop_data = 0;

	XGetWindowProperty (dpy, win, prop, 0, 0x7fffffff, False,
											type, &type_ret, &format_ret, &items_ret,
											&after_ret, &prop_data);

	if (items) {
		*items = items_ret;
	}

	return prop_data;
}

bool
WindowManager::getExtendedNetWMStates(Window win, NetWMStates &win_state)
{
	int num = 0;
	Atom* states;
	states = (Atom*)
		getExtendedNetPropertyData(win, m_ewmh_atoms->net_wm_state, XA_ATOM, &num);

	if (states) {
		if (states[0] == m_ewmh_atoms->net_wm_state_modal)
			win_state.modal=true;
		if (states[1] == m_ewmh_atoms->net_wm_state_sticky)
			win_state.sticky=true;
		if (states[2] == m_ewmh_atoms->net_wm_state_maximized_vert)
			win_state.max_vertical=true;
		if (states[3] == m_ewmh_atoms->net_wm_state_maximized_horz)
			win_state.max_horizontal=true;
		if (states[4] == m_ewmh_atoms->net_wm_state_shaded)
			win_state.shaded=true;
		if (states[5] == m_ewmh_atoms->net_wm_state_skip_taskbar)
			win_state.skip_taskbar=true;
		if (states[6] == m_ewmh_atoms->net_wm_state_skip_pager)
			win_state.skip_pager=true;
		if (states[7] == m_ewmh_atoms->net_wm_state_hidden)
			win_state.hidden = true;
		if (states[8] == m_ewmh_atoms->net_wm_state_fullscreen)
			win_state.fullscreen = true;

		XFree(states);

		return true;
	} else {
		return false;
	}
}

void
WindowManager::setExtendedNetWMState(Window win,
																		 bool modal, bool sticky,
																		 bool max_vert, bool max_horz,
																		 bool shaded, bool skip_taskbar,
																		 bool skip_pager,
																		 bool hidden, bool fullscreen)
{
	// Blank all states
	for (unsigned int i = 0; i < NET_WM_MAX_STATES; ++i)
		m_net_wm_states[i] = 0;

	if (modal)
		m_net_wm_states[0] = m_ewmh_atoms->net_wm_state_modal;
	if (sticky)
		m_net_wm_states[1] = m_ewmh_atoms->net_wm_state_sticky;
	if (max_vert)
		m_net_wm_states[2] = m_ewmh_atoms->net_wm_state_maximized_vert;
	if (max_horz)
		m_net_wm_states[3] = m_ewmh_atoms->net_wm_state_maximized_horz;
	if (shaded)
		m_net_wm_states[4] = m_ewmh_atoms->net_wm_state_shaded;
	if (skip_taskbar)
		m_net_wm_states[5] = m_ewmh_atoms->net_wm_state_skip_taskbar;
	if (skip_pager)
		m_net_wm_states[6] = m_ewmh_atoms->net_wm_state_skip_pager;
	if (hidden)
		m_net_wm_states[7] = m_ewmh_atoms->net_wm_state_hidden;
	if (fullscreen)
		m_net_wm_states[8] = m_ewmh_atoms->net_wm_state_fullscreen;

	XChangeProperty(dpy, win, m_ewmh_atoms->net_wm_state, XA_ATOM,
									32, PropModeReplace, (unsigned char *) m_net_wm_states,
									NET_WM_MAX_STATES);

}

void
WindowManager::setExtendedNetSupported(void)
{
	int total_net_supported = 31;

	Atom net_supported_list[] = {
		m_ewmh_atoms->net_supported,
		m_ewmh_atoms->net_client_list,
		m_ewmh_atoms->net_client_list_stacking,
		m_ewmh_atoms->net_number_of_desktops,
		m_ewmh_atoms->net_desktop_geometry,
		m_ewmh_atoms->net_desktop_viewport,
		m_ewmh_atoms->net_current_desktop,
		//atom_extended_net_desktop_names,
		m_ewmh_atoms->net_active_window,
		m_ewmh_atoms->net_workarea,
		m_ewmh_atoms->net_supporting_wm_check,
		//atom_extended_net_virtual_roots,
		m_ewmh_atoms->net_close_window,
		//atom_extended_net_wm_moveresize,
		m_ewmh_atoms->net_wm_name,
		//atom_extended_net_wm_visible_name,
		//atom_extended_net_wm_icon_name,
		//atom_extended_net_wm_visible_icon_name,
		m_ewmh_atoms->net_wm_desktop,
		m_ewmh_atoms->net_wm_window_type,
		m_ewmh_atoms->net_wm_window_type_desktop,
		m_ewmh_atoms->net_wm_window_type_dock,
		m_ewmh_atoms->net_wm_window_type_toolbar,
		m_ewmh_atoms->net_wm_window_type_menu,
		m_ewmh_atoms->net_wm_window_type_utility,
		m_ewmh_atoms->net_wm_window_type_splash,
		m_ewmh_atoms->net_wm_window_type_dialog,
		m_ewmh_atoms->net_wm_window_type_normal,
		m_ewmh_atoms->net_wm_state,
		m_ewmh_atoms->net_wm_state_modal,
		m_ewmh_atoms->net_wm_state_sticky,
		m_ewmh_atoms->net_wm_state_maximized_vert,
		m_ewmh_atoms->net_wm_state_maximized_horz,
		m_ewmh_atoms->net_wm_state_shaded,
		m_ewmh_atoms->net_wm_state_skip_taskbar,
		m_ewmh_atoms->net_wm_state_skip_pager,
		m_ewmh_atoms->net_wm_strut,
		//atom_extended_net_wm_icon_geometry,
		//atom_extended_net_wm_icon,
		//atom_extended_net_wm_pid,
		//atom_extended_net_wm_handled_icons,
		//atom_extended_net_wm_ping
	};

	XChangeProperty(dpy, m_screen->getRoot(), m_ewmh_atoms->net_supported,
									XA_CARDINAL, 32, PropModeReplace,
									(unsigned char *)net_supported_list, total_net_supported);
}

void
WindowManager::setExtendedNetDesktopGeometry(void)

{
	CARD32 geometry[] = { m_screen->getWidth(), m_screen->getHeight() };

	XChangeProperty(dpy, m_screen->getRoot(),
									m_ewmh_atoms->net_desktop_geometry, XA_CARDINAL, 32,
									PropModeReplace, (unsigned char *)geometry, 2);
}

void
WindowManager::setExtendedNetDesktopViewport(void)
{
	CARD32 viewport[] = { 0, 0 };

	XChangeProperty(dpy, m_screen->getRoot(),
									m_ewmh_atoms->net_desktop_viewport, XA_CARDINAL, 32,
									PropModeReplace, (unsigned char *)viewport, 2);
}

void
WindowManager::setExtendedNetActiveWindow(Window w)
{
	XChangeProperty(dpy, m_screen->getRoot(), m_ewmh_atoms->net_active_window,
									XA_WINDOW, 32, PropModeReplace, (unsigned char *) &w, 1);
}

void
WindowManager::setExtendedNetWorkArea(void)
{
	int work_x, work_y, work_width, work_height;

	work_x = m_master_strut->west;
	work_y = m_master_strut->north;
	work_width = m_screen->getWidth() - m_master_strut->east - work_x;
	work_height = m_screen->getHeight() - m_master_strut->south - work_y;

	CARD32 workarea[] = { work_x, work_y, work_width, work_height };

	XChangeProperty(dpy, m_screen->getRoot(), m_ewmh_atoms->net_workarea,
									XA_CARDINAL, 32, PropModeReplace,
									(unsigned char *)workarea, 4);

}

//! @fn    int findDesktopHint(Window win)
//! @brief Tries to find a desktop hint on the Window win
//! @param win Window to search on
//! @returns -1 if no hint were found, else the hints value.
int
WindowManager::findDesktopHint(Window win)
{
	int desktop = -1;
	desktop = getDesktopHint(win, m_ewmh_atoms->net_wm_desktop);
	return desktop;
}

long
WindowManager::getDesktopHint(Window win, int a)
{
	Atom real_type;
	int real_format;
	unsigned long items_read, items_left;
	long *data=NULL, value=-1;

	if(XGetWindowProperty(dpy, win, a, 0L, 1L, False, XA_CARDINAL, &real_type,
												&real_format, &items_read, &items_left,
												(unsigned char **)&data)==Success && items_read) {
		value=*data;
		XFree(data);
	}

	return value;
}
