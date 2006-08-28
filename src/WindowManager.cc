//
// WindowManager.cc for pekwm
// Copyright (C) 2002-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// windowmanager.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "Frame.hh"
#include "Client.hh"
#include "WindowManager.hh"

#include "PScreen.hh"
#include "ScreenResources.hh"
#include "ActionHandler.hh"
#include "AutoProperties.hh"
#include "Config.hh"
#include "Theme.hh"
#include "PFont.hh"
#include "PTexture.hh"
#include "ColorHandler.hh"
#include "FontHandler.hh"
#include "PixmapHandler.hh"
#include "TextureHandler.hh"
#include "Viewport.hh"
#include "Workspaces.hh"
#include "Util.hh"

#include "RegexString.hh"

#include "KeyGrabber.hh"
#ifdef HARBOUR
#include "Harbour.hh"
#include "DockApp.hh"
#endif // HARBOUR
#include "CmdDialog.hh"
#include "StatusWindow.hh"

#include "PMenu.hh" // we need this for the "Alt+Tabbing"
#ifdef MENUS
#include "WORefMenu.hh"
#include "ActionMenu.hh"
#include "FrameListMenu.hh"
#include "DecorMenu.hh"
#ifdef HARBOUR
#include "HarbourMenu.hh"
#endif // HARBOUR
#endif // MENUS

#include <iostream>
#include <list>
#include <algorithm>
#include <functional>
#include <memory>
#include <cassert>

extern "C" {
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif // HAVE_XRANDR
}

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::vector;
using std::map;
using std::mem_fun;
using std::find;

// Static initializers
const string WindowManager::_wm_name = string("pekwm");

#ifdef MENUS
const char *WindowManager::MENU_NAMES_RESERVED[] = {
  "ATTACHCLIENTINFRAME",
  "ATTACHCLIENT",
  "ATTACHFRAMEINFRAME",
  "ATTACHFRAME",
  "DECORMENU",
  "GOTOCLIENT",
  "GOTO",
  "ICON",
  "ROOTMENU",
  "ROOT", // To avoid name conflict, ROOTMENU -> ROOT
  "WINDOWMENU",
  "WINDOW" // To avoid name conflict, WINDOWMENU -> WINDOW
};

const unsigned int WindowManager::MENU_NAMES_RESERVED_COUNT = 
  sizeof(WindowManager::MENU_NAMES_RESERVED)
  / sizeof(WindowManager::MENU_NAMES_RESERVED[0]);
#endif // MENUS

WindowManager *wm;

//! @brief
void
sigHandler(int signal)
{
	switch (signal) {
	case SIGHUP:
		wm->reload();
		break;
	case SIGINT:
	case SIGTERM:
		wm->shutdown();
		break;
	case SIGCHLD:
		wait(NULL);
		break;
	}
}

//! @brief
int
handleXError(Display *dpy, XErrorEvent *e)
{
	if ((e->error_code == BadAccess) &&
			(e->resourceid == (RootWindow(dpy, DefaultScreen(dpy))))) {
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

// WindowManager::EdgeWO

//! @brief EdgeWO constructor
WindowManager::EdgeWO::EdgeWO(Display *dpy, Window root, EdgeType edge) :
PWinObj(dpy),
_edge(edge)
{
	_type = WO_SCREEN_EDGE;
	_layer = LAYER_NONE; // hack, goes over LAYER_MENU
	_sticky = true; // don't map/unmap
	_iconified = true; // hack, to be ignored when placing
	_focusable = false; // focusing input only windows crashes X

	XSetWindowAttributes sattr;
	sattr.override_redirect = True;
	sattr.event_mask =
		EnterWindowMask|LeaveWindowMask|ButtonPressMask|ButtonReleaseMask;

	_window =
		XCreateWindow(_dpy, root,
									0, 0, 1, 1, 0,
									CopyFromParent, InputOnly, CopyFromParent,
									CWOverrideRedirect|CWEventMask, &sattr);

	_wo_list.push_back(this);
}

//! @brief EdgeWO destructor
WindowManager::EdgeWO::~EdgeWO(void)
{
	_wo_list.remove(this);

	XDestroyWindow(_dpy, _window);
}

//! @brief
void
WindowManager::EdgeWO::mapWindow(void)
{
	if (_mapped)
		return;

	PWinObj::mapWindow();
	_iconified = true;
}

//! @brief
ActionEvent*
WindowManager::EdgeWO::handleEnterEvent(XCrossingEvent *ev)
{
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
																				MOUSE_EVENT_ENTER,
																				Config::instance()->getEdgeListFromPosition(_edge));
}

//! @brief
ActionEvent*
WindowManager::EdgeWO::handleButtonPress(XButtonEvent *ev)
{
	return ActionHandler::findMouseAction(ev->button, ev->state,
																				MOUSE_EVENT_PRESS,
																				Config::instance()->getEdgeListFromPosition(_edge));
}

//! @brief
ActionEvent*
WindowManager::EdgeWO::handleButtonRelease(XButtonEvent *ev)
{
	MouseEventType mb = MOUSE_EVENT_RELEASE;

	// first we check if it's a double click
	if (PScreen::instance()->isDoubleClick(ev->window, ev->button - 1, ev->time,
																				 Config::instance()->getDoubleClickTime())) {
		PScreen::instance()->setLastClickID(ev->window);
		PScreen::instance()->setLastClickTime(ev->button - 1, 0);

		mb = MOUSE_EVENT_DOUBLE;

	} else {
		PScreen::instance()->setLastClickID(ev->window);
		PScreen::instance()->setLastClickTime(ev->button - 1, ev->time);
	}

	return ActionHandler::findMouseAction(ev->button, ev->state, mb,
																				Config::instance()->getEdgeListFromPosition(_edge));
}

// WindowManager::RootWO

//! @brief RootWO constructor
WindowManager::RootWO::RootWO(Display *dpy, Window root) :
PWinObj(dpy)
{
	_type = WO_SCREEN_ROOT;
	_layer = LAYER_NONE;
	_mapped = true;

	_window = root;
	_gm.width = PScreen::instance()->getWidth();
	_gm.height = PScreen::instance()->getHeight();

	_wo_list.push_back(this);
}

//! @brief RootWO destructor
WindowManager::RootWO::~RootWO(void)
{
	_wo_list.remove(this);
}

//! @brief
ActionEvent*
WindowManager::RootWO::handleButtonPress(XButtonEvent *ev)
{
	return ActionHandler::findMouseAction(ev->button, ev->state,
																				MOUSE_EVENT_PRESS,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

//! @brief
ActionEvent*
WindowManager::RootWO::handleButtonRelease(XButtonEvent *ev)
{
	MouseEventType mb = MOUSE_EVENT_RELEASE;

	// first we check if it's a double click
	if (PScreen::instance()->isDoubleClick(ev->window, ev->button - 1, ev->time,
													Config::instance()->getDoubleClickTime())) {
		PScreen::instance()->setLastClickID(ev->window);
		PScreen::instance()->setLastClickTime(ev->button - 1, 0);

		mb = MOUSE_EVENT_DOUBLE;

	} else {
		PScreen::instance()->setLastClickID(ev->window);
		PScreen::instance()->setLastClickTime(ev->button - 1, ev->time);
	}

	return ActionHandler::findMouseAction(ev->button, ev->state, mb,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

//! @brief
ActionEvent*
WindowManager::RootWO::handleMotionEvent(XMotionEvent *ev)
{
	uint button = PScreen::instance()->getButtonFromState(ev->state);

	return ActionHandler::findMouseAction(button, ev->state,
																				MOUSE_EVENT_MOTION,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

//! @brief
ActionEvent*
WindowManager::RootWO::handleEnterEvent(XCrossingEvent *ev)
{
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
																				MOUSE_EVENT_ENTER,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

//! @brief
ActionEvent*
WindowManager::RootWO::handleLeaveEvent(XCrossingEvent *ev)
{
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
																				MOUSE_EVENT_LEAVE,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

// WindowManager

//! @brief Constructor for WindowManager class
WindowManager::WindowManager(const std::string &command_line, const std::string &config_file) :
_screen(NULL), _screen_resources(NULL),
_keygrabber(NULL),
_config(NULL), _color_handler(NULL),
_font_handler(NULL), _texture_handler(NULL),
_theme(NULL), _action_handler(NULL),
_autoproperties(NULL), _workspaces(NULL),
#ifdef HARBOUR
_harbour(NULL),
#endif // HARBOUR
_cmd_dialog(NULL), _status_window(NULL),
_command_line(command_line),
_startup(false), _shutdown(false),
_tagged_frame(NULL), _tag_activate(true), _allow_grouping(true),
_root_wo(NULL),
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

	// construct
	_config = new Config();
	_config->load(config_file);

	setupDisplay();

	scanWindows();
	setupFrameIds();

	// add all frames to the MRU list
  _mru_list.resize (_frame_list.size ());
  copy (_frame_list.begin (), _frame_list.end (), _mru_list.begin ());

	// make sure windows are inside the virtual viewports
	for (uint i = 0; i < _workspaces->size(); ++i) {
		_workspaces->getViewport(i)->makeAllInsideVirtual();
	}

	execStartFile();

	doEventLoop();
}

//! @brief WindowManager destructor
WindowManager::~WindowManager(void)
{
	cleanup();

	if (_root_wo != NULL)
		delete _root_wo;

#ifdef HARBOUR
	if (_harbour) {
		delete _harbour;
	}
#endif // HARBOUR

	if (_cmd_dialog)
		delete _cmd_dialog;
	if (_status_window)
		delete _status_window;

	if (_action_handler)
		delete _action_handler;
	if (_autoproperties)
		delete _autoproperties;

#ifdef MENUS
  deleteMenus();
#endif // MENUS

	if (_pekwm_atoms != NULL)
		delete _pekwm_atoms;
	if (_icccm_atoms != NULL)
		delete _icccm_atoms;
	if (_ewmh_atoms != NULL)
		delete _ewmh_atoms;
	if (_keygrabber != NULL)
		delete _keygrabber;
	if (_workspaces != NULL)
		delete _workspaces;
	if (_config != NULL)
		delete _config;
	if (_theme != NULL)
		delete _theme;
	if (_color_handler != NULL)
		delete _color_handler;
	if (_font_handler != NULL)
		delete _font_handler;
	if (_texture_handler != NULL)
		delete _texture_handler;
	if (_screen_resources != NULL)
		delete _screen_resources;

	if (_screen != NULL) {
		Display *dpy = _screen->getDpy();

		delete _screen;

		XCloseDisplay(dpy);
	}
}

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

//! @brief Cleans up, Maps windows etc.
void
WindowManager::cleanup(void)
{
	// update all nonactive clients properties
 	list<Frame*>::iterator it_f(_frame_list.begin());
 	for (; it_f != _frame_list.end(); ++it_f) {
 		(*it_f)->updateInactiveChildInfo();
 	}

	// make sure windows are inside the real screen
	for (uint i = 0; i < _workspaces->size(); ++i) {
		_workspaces->getViewport(i)->makeAllInsideReal();
	}

	// remove all dockapps
#ifdef HARBOUR
	_harbour->removeAllDockApps();
#endif // HARBOUR

	// To preserve stacking order when destroying the frames, we go through
	// the PWinObj list from the Workspaces and put all Frames into our own
	// list, then we delete the frames in order.
	list<Frame*> frame_list;
	list<PWinObj*>::iterator it_w(_workspaces->begin());
	for (; it_w != _workspaces->end(); ++it_w) {
		if ((*it_w)->getType() == PWinObj::WO_FRAME)
			frame_list.push_back(static_cast<Frame*>(*it_w));
	}
	// Delete all Frames. This reparents the Clients.
	for (it_f = frame_list.begin(); it_f != frame_list.end(); ++it_f)
		delete *it_f;

	// Delete all Clients.
	list<Client*> client_list(_client_list);
	list<Client*>::iterator it_c(client_list.begin());
	for (; it_c != client_list.end(); ++it_c)
		delete *it_c;

#ifdef DEBUG
	if (_frame_list.size()) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "WindowManager::cleanup()" << endl
				 << " *** _frame_list.size() > 0: " << _frame_list.size() << endl;
	}
	if (_client_list.size()) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "WindowManager::cleanup()" << endl
				 << " *** _client_list.size() > 0: " << _client_list.size() << endl;
	}
#endif // DEBUG

	_keygrabber->ungrabKeys(_screen->getRoot());

	// destroy screen edge
	screenEdgeDestroy();

	// destroy atom windows
	XDestroyWindow(_screen->getDpy(), _extended_hints_win);

	XInstallColormap(_screen->getDpy(), _screen->getColormap());
	XSetInputFocus(_screen->getDpy(), PointerRoot,
								 RevertToPointerRoot, CurrentTime);
}

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

	_screen = new PScreen(dpy);
	_color_handler = new ColorHandler(dpy);
	_font_handler = new FontHandler();
	_texture_handler = new TextureHandler();
	_action_handler = new ActionHandler(this);

	// setup the font trimming
	PFont::setTrimString(_config->getTrimTitle());

	// load colors, fonts
	_screen_resources = new ScreenResources();
	_theme = new Theme(_screen);

	_autoproperties = new AutoProperties();
	_autoproperties->load();

	_pekwm_atoms = new PekwmAtoms();
	_icccm_atoms = new IcccmAtoms();
	_ewmh_atoms = new EwmhAtoms();

	_workspaces = new Workspaces(_config->getWorkspaces());

	// set initial values of hints
	initHints();

#ifdef HARBOUR
	_harbour = new Harbour(_screen, _theme, _workspaces);
#endif // HARBOUR

	_cmd_dialog = new CmdDialog(_screen->getDpy(), _theme, "CmdDialog");
	_status_window = new StatusWindow(_screen->getDpy(), _theme);

	XDefineCursor(dpy, _screen->getRoot(),
								_screen_resources->getCursor(ScreenResources::CURSOR_ARROW));

	// select root window events
	XSetWindowAttributes sattr;
	sattr.event_mask =
		SubstructureRedirectMask|SubstructureNotifyMask|
		ColormapChangeMask|FocusChangeMask|
		EnterWindowMask|
		PropertyChangeMask|
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;

	XChangeWindowAttributes(dpy, _screen->getRoot(), CWEventMask, &sattr);

#ifdef HAVE_XRANDR
#ifdef X_RRScreenChangeSelectInput
	XRRScreenChangeSelectInput(dpy, _screen->getRoot(), True);
#else //! X_RRScreenChangeSelectInput
	XRRSelectInput(dpy, _screen->getRoot(), RRScreenChangeNotifyMask);	
#endif // X_RRScreenChangeSelectInput
#endif // HAVE_XRANDR

	_keygrabber = new KeyGrabber(_screen);
	_keygrabber->load(_config->getKeyFile());
	_keygrabber->grabKeys(_screen->getRoot());

#ifdef MENUS
  createMenus();
#endif // MENUS

	// Create screen edge windows
	screenEdgeCreate();
	screenEdgeMapUnmap();

	// Create root PWinObj
	_root_wo = new RootWO(dpy, _screen->getRoot());
	PWinObj::setRootPWinObj(_root_wo);
}

//! @brief Goes through the window and creates Clients/DockApps.
void
WindowManager::scanWindows(void)
{
	if (_startup) // only done once when we start
		return;

	uint num_wins;
	Window d_win1, d_win2, *wins;
	XWindowAttributes attr;

	// Lets create a list of windows on the display
	XQueryTree(_screen->getDpy(), _screen->getRoot(),
						 &d_win1, &d_win2, &wins, &num_wins);
	list<Window> win_list(wins, wins + num_wins);
	XFree(wins);

	list<Window>::iterator it(win_list.begin());

#ifdef HARBOUR
	// If we have the Harbour on, we filter out all windows with the
	// the IconWindowHint set not pointing to themself, making DockApps
	// work as they are supposed to.
	for (; it != win_list.end(); ++it) {
		if (*it == None)
			continue;

		XWMHints *wm_hints = XGetWMHints(_screen->getDpy(), *it);
		if (wm_hints) {
			if ((wm_hints->flags&IconWindowHint) &&
					(wm_hints->icon_window != *it)) {
				list<Window>::iterator i_it(find(win_list.begin(), win_list.end(), wm_hints->icon_window));
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

		XGetWindowAttributes(_screen->getDpy(), *it, &attr);
		if (!attr.override_redirect && (attr.map_state != IsUnmapped)) {
#ifdef HARBOUR
			XWMHints *wm_hints = XGetWMHints(_screen->getDpy(), *it);
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
	PWinObj *wo = _workspaces->getTopWO(PWinObj::WO_FRAME);
	if ((wo != NULL) && wo->isMapped()) {
		wo->giveInputFocus();
	} else {
		_root_wo->giveInputFocus();
	}

	// We won't be needing these anymore until next restart
	_autoproperties->removeApplyOnStart();

	_startup = true;
}

//! @brief Creates and places screen edge
void
WindowManager::screenEdgeCreate(void)
{
	if (_screen_edge_list.size() != 0) {
		return;
	}

	_screen_edge_list.push_back(new EdgeWO(_screen->getDpy(), _screen->getRoot(), SCREEN_EDGE_LEFT));
	_screen_edge_list.push_back(new EdgeWO(_screen->getDpy(), _screen->getRoot(), SCREEN_EDGE_RIGHT));
	_screen_edge_list.push_back(new EdgeWO(_screen->getDpy(), _screen->getRoot(), SCREEN_EDGE_TOP));
	_screen_edge_list.push_back(new EdgeWO(_screen->getDpy(), _screen->getRoot(), SCREEN_EDGE_BOTTOM));

	// make sure the edge stays ontop
	list<EdgeWO*>::iterator it(_screen_edge_list.begin());
	for (; it != _screen_edge_list.end(); ++it) {
		_workspaces->insert(*it);
	}

	screenEdgeResize();
}

//! @brief
void
WindowManager::screenEdgeDestroy(void)
{
	if (_screen_edge_list.size() == 0) {
		return;
	}

	list<EdgeWO*>::iterator it(_screen_edge_list.begin());
	for (; it != _screen_edge_list.end(); ++it) {
		_workspaces->remove(*it);
		delete *it;
	}
}

//! @brief
void
WindowManager::screenEdgeResize(void)
{
	if (_screen_edge_list.size() != 4)
		return;

	uint size = _config->getScreenEdgeSize(); // convenience
	if (size == 0)
		size = 1;

	list<EdgeWO*>::iterator it(_screen_edge_list.begin());

	// left edge
	(*it)->move(0, 0);
	(*it)->resize(size, _screen->getHeight());
	++it;

	// right edge
	(*it)->move(_screen->getWidth() - size, 0);
	(*it)->resize(size, _screen->getHeight());
	++it;

	// top edge
	(*it)->move(size, 0);
	(*it)->resize(_screen->getWidth() - (size * 2), size);
	++it;

	// bottom edge
	(*it)->move(size, _screen->getHeight() - size);
	(*it)->resize(_screen->getWidth() - (size * 2), size);
}

void
WindowManager::screenEdgeMapUnmap(void)
{
	if (_screen_edge_list.size() != 4) {
		return;
	}

	list<EdgeWO*>::iterator it(_screen_edge_list.begin());
	for (; it != _screen_edge_list.end(); ++it) {
		if ((_config->getScreenEdgeSize() > 0) &&
				(_config->getEdgeListFromPosition((*it)->getEdge())->size() > 0)) {
			(*it)->mapWindow();
		} else {
			(*it)->unmapWindow();
		}
	}
}

//! @brief Reloads configuration and updates states.
void
WindowManager::doReload(void)
{
	// unique client names state
	bool old_client_unique_name = _config->getClientUniqueName();
	string old_client_unique_name_pre =
		_config->getClientUniqueNamePre();
	string old_client_unique_name_post =
		_config->getClientUniqueNamePost();

#ifdef HARBOUR
	// I do not want to restack or rearrange if nothing has changed.
	uint old_harbour_placement = _config->getHarbourPlacement();
	bool old_harbour_stacking = _config->isHarbourOntop();
#ifdef HAVE_XINERAMA
	int old_harbour_head_nr = _config->getHarbourHead();
#endif // HAVE_XINERAMA
#endif // HARBOUR

	_config->load(_config->getConfigFile()); // reload the config
	_autoproperties->load(); // reload autoprops config

	// update what might have changed in the cfg toouching the hints
	_workspaces->setSize(_config->getWorkspaces());

	// flush pixmap cache and set size
	_screen_resources->getPixmapHandler()->setCacheSize(_config->getScreenPixmapCacheSize());

	// set the font title trim before reloading the themes
	PFont::setTrimString(_config->getTrimTitle());

	// update the ClientUniqueNames if needed
	if ((old_client_unique_name != _config->getClientUniqueName()) ||
			(old_client_unique_name_pre != _config->getClientUniqueNamePre()) ||
			(old_client_unique_name_post != _config->getClientUniqueNamePost())) {
		for_each(_client_list.begin(), _client_list.end(),
						 mem_fun(&Client::getXClientName));
	}

	// reload the theme
	_theme->load(_config->getThemeFile());

	// resize the viewports
	for (uint i = 0; i < _workspaces->size(); ++i) {
		_workspaces->getViewport(i)->reload();
	}

	// resize the screen edge
	screenEdgeResize();
	screenEdgeMapUnmap();

	// regrab the buttons on the client windows
	for_each(_client_list.begin(), _client_list.end(),
					 mem_fun(&Client::grabButtons));

	// reload the keygrabber
	_keygrabber->load(_config->getKeyFile());

	_keygrabber->ungrabKeys(_screen->getRoot());
	_keygrabber->grabKeys(_screen->getRoot());

	// regrab keys
	list<Client*>::iterator c_it(_client_list.begin());
	for (; c_it != _client_list.end(); ++c_it) {
		_keygrabber->ungrabKeys((*c_it)->getWindow());
		_keygrabber->grabKeys((*c_it)->getWindow());
	}

	// update the status window and cmd dialog theme
	_cmd_dialog->loadDecor();
	_status_window->loadDecor();

	// reload the themes on the frames
	for_each(_frame_list.begin(), _frame_list.end(),
					 mem_fun(&PDecor::loadDecor));

	// NOTE: we need to load autoproperties after decor have been updated
	// as otherwise old theme data pointer will be used and sig 11 pekwm.
	list<Frame*>::iterator f_it(_frame_list.begin());
	for (; f_it != _frame_list.end(); ++f_it) {
		(*f_it)->readAutoprops();
	}


#ifdef HARBOUR
	_harbour->loadTheme();
	if (old_harbour_placement != _config->getHarbourPlacement())
		_harbour->rearrange();
	if (old_harbour_stacking != _config->isHarbourOntop())
		_harbour->restack();
#ifdef HAVE_XINERAMA
	if (old_harbour_head_nr != _config->getHarbourHead())
		_harbour->rearrange();
#endif // HAVE_XINERAMA
	_harbour->updateHarbourSize();
#endif // HARBOUR

#ifdef MENUS
  updateMenus();
#endif // MENUS

	_screen->updateStrut();

	_reload = false;
}

//! @brief Exit pekwm and restart with the command command
void
WindowManager::restart(std::string command)
{
	if (command.size() == 0) {
		command = _command_line;
	}
	_restart_command = command;
	_shutdown = true;
}

// Event handling routins beneath this =====================================

void
WindowManager::doEventLoop(void)
{
	Display *dpy = _screen->getDpy();

#ifdef HAVE_SHAPE
	Client *client = NULL; // used for shape events
#endif // HAVE_SHAPE

	XEvent ev;
	while (!_shutdown) {
		if (_reload)
			doReload();

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

		default:
#ifdef HAVE_SHAPE
			if (_screen->hasExtensionShape() &&
					(ev.type == _screen->getEventShape())) {
				client = findClient(ev.xany.window);
				if ((client != NULL) && (client->getParent() != NULL)) {
					static_cast<Frame*>(client->getParent())->handleShapeEvent(&ev.xany);
				}
			}
#endif // HAVE_SHAPE
#ifdef HAVE_XRANDR
			if (_screen->hasExtensionXRandr() &&
					(ev.type == _screen->getEventXRandr())) {
				handleXRandrEvent((XRRScreenChangeNotifyEvent*) &ev);
			}
#endif // HAVE_XRANDR
			break;
		}
	}
}

//! @brief Handle XKeyEvents
void
WindowManager::handleKeyEvent(XKeyEvent *ev)
{
	ActionEvent *ae =	NULL;
	PWinObj *wo = PWinObj::getFocusedPWinObj();
	PWinObj::Type type = (wo == NULL) ? PWinObj::WO_SCREEN_ROOT : wo->getType();

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
		if (ev->type == KeyPress) {
			ae = wo->handleKeyPress(ev);
		} else {
			ae = wo->handleKeyRelease(ev);
		}
		wo = static_cast<CmdDialog*>(wo)->getWORef();
		break;
	default:
		if (wo != NULL) {
			if (ev->type == KeyPress) {
				ae = wo->handleKeyPress(ev);
			} else {
				ae = wo->handleKeyRelease(ev);
			}
		}
		break;
	}

	if (ae != NULL) {
		ActionPerformed ap(wo, *ae);
		ap.type = ev->type;
		ap.event.key = ev;

		_action_handler->handleAction(ap);
	}

	// flush Enter events caused by keygrabbing
	XEvent e;
	while (XCheckTypedEvent(_screen->getDpy(), EnterNotify, &e));
}

//! @brief
void
WindowManager::handleButtonPressEvent(XButtonEvent *ev)
{
	// Clear event queue
	while (XCheckTypedEvent(_screen->getDpy(), ButtonPress, (XEvent *) ev));

	ActionEvent *ae = NULL;
	PWinObj *wo = NULL;

	wo = PWinObj::findPWinObj(ev->window);
	if (wo != NULL) {
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

	if (ae != NULL) {
		ActionPerformed ap(wo, *ae);
		ap.type = ev->type;
		ap.event.button = ev;

		_action_handler->handleAction(ap);
	}
#ifdef HARBOUR
		else {
			DockApp *da = _harbour->findDockAppFromFrame(ev->window);
			if (da) {
				_harbour->handleButtonEvent(ev, da);
			}
		}
#endif // HARBOUR
}

//! @brief
void
WindowManager::handleButtonReleaseEvent(XButtonEvent *ev)
{
	// Flush ButtonReleases
	while (XCheckTypedEvent(_screen->getDpy(), ButtonRelease, (XEvent *) ev));

	ActionEvent *ae = NULL;
	PWinObj *wo = PWinObj::findPWinObj(ev->window);

	if (wo != NULL) {
		ae = wo->handleButtonRelease(ev);

		if (wo->getType() == PWinObj::WO_FRAME) {
			// this is done so that clicking the titlebar executes action on
			// the client clicked on, doesn't apply when subwindow is set (meaning
			// a titlebar button beeing pressed)
			if ((ev->subwindow == None) &&
					(ev->window == static_cast<Frame*>(wo)->getTitleWindow())) {
				wo = static_cast<Frame*>(wo)->getChildFromPos(ev->x);
			} else {
				wo = static_cast<Frame*>(wo)->getActiveChild();
			}
		}

		if (ae != NULL) {
			ActionPerformed ap(wo, *ae);
			ap.type = ev->type;
			ap.event.button = ev;

			_action_handler->handleAction(ap);
		}
	}
#ifdef HARBOUR
	else {
		DockApp *da = _harbour->findDockAppFromFrame(ev->window);
		if (da) {
			_harbour->handleButtonEvent(ev, da);
		}
	}
#endif // HARBOUR
}

void
WindowManager::handleConfigureRequestEvent(XConfigureRequestEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		((Frame*) client->getParent())->handleConfigureRequest(ev, client);

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

				XConfigureWindow(_screen->getDpy(), ev->window,
												 ev->value_mask, &wc);
			}
	}
}

//! @brief
void
WindowManager::handleMotionEvent(XMotionEvent *ev)
{
	ActionEvent *ae = NULL;
	PWinObj *wo = PWinObj::findPWinObj(ev->window);

	if (wo != NULL) {
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

		if (ae != NULL) {
			ActionPerformed ap(wo, *ae);
			ap.type = ev->type;
			ap.event.motion = ev;

			_action_handler->handleAction(ap);
		}
	}
#ifdef HARBOUR
		else {
			DockApp *da = _harbour->findDockAppFromFrame(ev->window);
			if (da) {
				_harbour->handleMotionNotifyEvent(ev, da);
			}
		}
#endif // HARBOUR
}

//! @brief
void
WindowManager::handleMapRequestEvent(XMapRequestEvent *ev)
{
	PWinObj *wo = PWinObj::findPWinObj(ev->window);

	if (wo != NULL) {
		wo->handleMapRequest(ev);

	} else {
		XWindowAttributes attr;
		XGetWindowAttributes(_screen->getDpy(), ev->window, &attr);
		if (!attr.override_redirect) {
			// if we have the harbour enabled, we need to figure out wheter or
			// not this is a dockapp.
#ifdef HARBOUR
			XWMHints *wm_hints = XGetWMHints(_screen->getDpy(), ev->window);
			if (wm_hints != NULL) {
				if ((wm_hints->flags&StateHint) &&
						(wm_hints->initial_state == WithdrawnState)) {
					_harbour->addDockApp(new DockApp(_screen, _theme, ev->window));
				} else {
					Client *client = new Client(this, ev->window, true);
					if (client->isAlive() == false) {
						delete client;
					}
				}
				XFree(wm_hints);
			} else
#endif // HARBOUR
				{
					Client *client = new Client(this, ev->window, true);
					if (client->isAlive() == false) {
						delete client;
					}
				}
		}
	}
}

//! @brief
void
WindowManager::handleUnmapEvent(XUnmapEvent *ev)
{
	PWinObj *wo = PWinObj::findPWinObj(ev->window);
	PWinObj *wo_search = NULL;

	PWinObj::Type wo_type = PWinObj::WO_NO_TYPE;

	if (wo != NULL) {
		wo_type = wo->getType();
		if (wo_type == PWinObj::WO_CLIENT) {
			wo_search = wo->getParent();
		}

		wo->handleUnmapEvent(ev);

		if (wo == PWinObj::getFocusedPWinObj()) {
			PWinObj::setFocusedPWinObj(NULL);
		}
	}

#ifdef HARBOUR
	else {
		DockApp *da = _harbour->findDockApp(ev->window);
		if (da) {
			if (ev->window == ev->event) {
				_harbour->removeDockApp(da);
			}
		}
	}
#endif // HARBOUR

	if ((wo_type != PWinObj::WO_MENU) &&
			(PWinObj::getFocusedPWinObj() == NULL)) {
		findWOAndFocus(wo_search);
	}
}

void
WindowManager::handleDestroyWindowEvent(XDestroyWindowEvent *ev)
{
	Client *client = findClientFromWindow(ev->window);

	if (client) {
		PWinObj *wo_search = client->getParent();
		client->handleDestroyEvent(ev);

		if (PWinObj::getFocusedPWinObj() == NULL) {
			findWOAndFocus(wo_search);
		}
	}
#ifdef HARBOUR
	else {
		DockApp *da = _harbour->findDockApp(ev->window);
		if (da) {
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
	while (XCheckTypedEvent(_screen->getDpy(), EnterNotify, (XEvent *) ev));

	if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab)) {
		return;
	}

	PWinObj *wo = PWinObj::findPWinObj(ev->window);

	if (wo != NULL) {

		if (wo->getType() == PWinObj::WO_CLIENT) {
			wo = wo->getParent();
		}

		ActionEvent *ae = wo->handleEnterEvent(ev);

		if (ae != NULL) {
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
	while (XCheckTypedEvent(_screen->getDpy(), LeaveNotify, (XEvent *) ev));

	if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab)) {
		return;
	}

	PWinObj *wo = PWinObj::findPWinObj(ev->window);

	if (wo != NULL) {
		ActionEvent *ae = wo->handleLeaveEvent(ev);

		if (ae != NULL) {
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
	// Get the last focus in event, no need to go through them all.
	while (XCheckTypedEvent(_screen->getDpy(), FocusIn, (XEvent *) ev));

	if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab))
		return;

	PWinObj *wo = PWinObj::findPWinObj(ev->window);
	if (wo) {
		// To simplify logic, changing client in frame wouldn't update the
		// focused window because wo != focused_wo woule be true.
		if (wo->getType() == PWinObj::WO_FRAME)
			wo = static_cast<Frame*>(wo)->getActiveChild();

		if (!wo->isFocusable() || !wo->isMapped()) {
			findWOAndFocus(NULL);

		} else if (wo != PWinObj::getFocusedPWinObj()) {
			// If it's a valid FocusIn event with accepatable target lets flush
			// all EnterNotify and LeaveNotify as they can interfere with
			// focusing if Sloppy or Follow like focus model is used.
			XEvent e_flush;
			while (XCheckTypedEvent(_screen->getDpy(), EnterNotify, &e_flush));
			while (XCheckTypedEvent(_screen->getDpy(), LeaveNotify, &e_flush));

			PWinObj *focused_wo = PWinObj::getFocusedPWinObj(); // convenience

			// unfocus last window
			if (focused_wo != NULL) {
				if (focused_wo->getType() == PWinObj::WO_CLIENT) {
					focused_wo->getParent()->setFocused(false);
				} else {
					focused_wo->setFocused(false);
				}
				_workspaces->setLastFocused(_workspaces->getActive(), focused_wo);
			}

			PWinObj::setFocusedPWinObj(wo);

			if (wo->getType() == PWinObj::WO_CLIENT) {
				wo->getParent()->setFocused(true);
				setEwmhActiveWindow(wo->getWindow());

			} else {
				wo->setFocused(true);
				setEwmhActiveWindow(None);
			}

			// update the MRU list
			if (wo->getType() == PWinObj::WO_CLIENT) {
				_mru_list.remove(wo->getParent());
				_mru_list.push_back(wo->getParent());
			}
		}
	}
}

//! @brief Handles FocusOut Events.
void
WindowManager::handleFocusOutEvent(XFocusChangeEvent *ev)
{
	// Get the last focus in event, no need to go through them all.
	while (XCheckTypedEvent(_screen->getDpy(), FocusOut, (XEvent *) ev));

	if ((ev->mode == NotifyGrab) || (ev->mode == NotifyUngrab))
		return;

	// Match against focusd PWinObj if any, if matches we see if
	// there are any FocusIn events in the queue, if not we give focus to
	// the root window as we don't want to get left without focus.
	if (PWinObj::getFocusedPWinObj()
			&& (*PWinObj::getFocusedPWinObj() == ev->window)) {
		if (XCheckTypedEvent(_screen->getDpy(), FocusIn, (XEvent *) ev)) {
			// We found a Event, put it back and handle it later on.
			XPutBackEvent(_screen->getDpy(), (XEvent *) ev);
		} else {
			// No event found, give the root PWinObj focus.
			PWinObj::getRootPWinObj()->giveInputFocus();
		}
	}
}

void
WindowManager::handleClientMessageEvent(XClientMessageEvent *ev)
{
	if (ev->window == _screen->getRoot()) {
		// root window messages

		if (ev->format == 32) {

			if (ev->message_type == _ewmh_atoms->getAtom(NET_CURRENT_DESKTOP)) {
				_workspaces->setWorkspace(ev->data.l[0], true);

			} else if (ev->message_type ==
								 _ewmh_atoms->getAtom(NET_NUMBER_OF_DESKTOPS)) {
				if (ev->data.l[0] > 0) {
					_workspaces->setSize(ev->data.l[0]);
				}
			} else if (ev->message_type ==
								 _ewmh_atoms->getAtom(NET_DESKTOP_VIEWPORT)) {
				_workspaces->getActiveViewport()->move(ev->data.l[0], ev->data.l[1]);
			}
		}

	} else {
		// client messages

		Client *client = findClientFromWindow(ev->window);
		if (client) {
			static_cast<Frame*>(client->getParent())->handleClientMessage(ev, client);
		}
	}
}

void
WindowManager::handleColormapEvent(XColormapEvent *ev)
{
	Client *client = findClient(ev->window);
	if (client) {
		client =
			static_cast<Client*>(((Frame*) client->getParent())->getActiveChild());
		client->handleColormapChange(ev);
	}
}

void
WindowManager::handlePropertyEvent(XPropertyEvent *ev)
{
	if (ev->window == _screen->getRoot())
		return;

	Client *client = findClientFromWindow(ev->window);

	if (client != NULL) {
		((Frame*) client->getParent())->handlePropertyChange(ev, client);
	}
}

void
WindowManager::handleExposeEvent(XExposeEvent *ev)
{
	ActionEvent *ae = NULL;

	PWinObj *wo = PWinObj::findPWinObj(ev->window);
	if (wo != NULL) {
		ae = wo->handleExposeEvent(ev);
	}

	if (ae != NULL) {
		ActionPerformed ap(wo, *ae);
		ap.type = ev->type;
		ap.event.expose = ev;

		_action_handler->handleAction(ap);
	}
}

#ifdef HAVE_XRANDR
//! @brief Handles XRandr event.
void
WindowManager::handleXRandrEvent(XRRScreenChangeNotifyEvent *ev)
{
	// don't care about what it is, only update screen size
	_screen->updateGeometry(ev->width, ev->height);
#ifdef HARBOUR
	_harbour->updateGeometry();
#endif // HARBOUR

	Viewport *vp;
	for (uint i = 0; i < _workspaces->size(); ++i) {
		if ((vp = _workspaces->getViewport(i)) != NULL) {
			vp->updateGeometry();
		}
	}

	screenEdgeResize();
}
#endif // HAVE_XRANDR

// Event handling routines stop ============================================

#ifdef MENUS
//! @brief Creates reserved menus and populates _menu_map
void
WindowManager::createMenus(void)
{
  // Make sure this is not called twice without an delete in between
  assert(! _menu_map.size());

  _menu_map["ATTACHCLIENTINFRAME"] =
    new FrameListMenu(_screen, _theme, _frame_list, ATTACH_CLIENT_IN_FRAME_TYPE,
		      "Attach Client In Frame", "ATTACHCLIENTINFRAME");
  _menu_map["ATTACHCLIENT"] =
    new FrameListMenu(_screen, _theme, _frame_list, ATTACH_CLIENT_TYPE,
		      "Attach Client", "ATTACHCLIENT");
  _menu_map["ATTACHFRAMEINFRAME"] =
    new FrameListMenu(_screen, _theme, _frame_list, ATTACH_FRAME_IN_FRAME_TYPE,
		      "Attach Frame In Frame", "ATTACHFRAMEINFRAME");
  _menu_map["ATTACHFRAME"] =
    new FrameListMenu(_screen, _theme, _frame_list, ATTACH_FRAME_TYPE,
		      "Attach Frame", "ATTACHFRAME");
  _menu_map["DECORMENU"] =
    new DecorMenu(_screen, _theme, _action_handler, "DECORMENU");
  _menu_map["GOTOCLIENT"] =
    new FrameListMenu(_screen, _theme, _frame_list, GOTOCLIENTMENU_TYPE,
		      "Focus Client", "GOTOCLIENT");
  _menu_map["GOTO"] =
    new FrameListMenu(_screen, _theme, _frame_list, GOTOMENU_TYPE,
		      "Focus Frame", "GOTO");
  _menu_map["ICON"] =
    new FrameListMenu(_screen, _theme, _frame_list, ICONMENU_TYPE,
		      "Focus Iconified Frame", "ICON");
  _menu_map["ROOT"] = // It's named ROOT to be backwards compatible
    new ActionMenu(this, ROOTMENU_TYPE, "", "ROOTMENU");
  _menu_map["WINDOW"] = // It's named WINDOW to be backwards compatible
    new ActionMenu(this, WINDOWMENU_TYPE, "", "WINDOWMENU");

  // As the previous step is done manually, make sure it's done correct.
  assert(_menu_map.size() == (MENU_NAMES_RESERVED_COUNT - 2));

  // Load configuration, pass specific section to loading
  CfgParser menu_cfg;
  if (menu_cfg.parse(_config->getMenuFile())
      || menu_cfg.parse (string(SYSCONFDIR "/menu"))) {

    // Load standard menus
    map<string, PMenu*>::iterator it = _menu_map.begin();
    for (; it != _menu_map.end(); ++it) {
      it->second->reload(menu_cfg.get_entry_root()->find_section(it->second->getName()));
    }

    // Load standalone menus
    updateMenusStandalone(menu_cfg.get_entry_root()->get_entry_next());
  }
}

//! @brief (re)loads the menus in the menu configuration
void
WindowManager::updateMenus(void)
{
  // Load configuration, pass specific section to loading
  bool cfg_ok = true;
  CfgParser menu_cfg;
  if (! menu_cfg.parse(_config->getMenuFile())) {
    if (! menu_cfg.parse (string(SYSCONFDIR "/menu"))) {
      cfg_ok = false;
    }
  }

  // Update, delete standalone root menus, load decors on others
  map<string, PMenu*>::iterator it = _menu_map.begin();
  for (; it != _menu_map.end(); ++it) {
    if (it->second->getMenuType() == ROOTMENU_STANDALONE_TYPE) {
      delete it->second;
      _menu_map.erase(it);
    } else {
      it->second->loadDecor();
      // Only reload the menu if we got a ok configuration
      if (cfg_ok) {
	it->second->reload(menu_cfg.get_entry_root()->find_section(it->second->getName()));
      }
    }
  }

  // Update standalone root menus (name != ROOTMENU)
  updateMenusStandalone(menu_cfg.get_entry_root()->get_entry_next());

  // Special case for HARBOUR menu which is not included in the menu map
#ifdef HARBOUR
  _harbour->getHarbourMenu()->loadDecor();
  _harbour->getHarbourMenu()->reload(static_cast<CfgParser::Entry*>(0));
#endif // HARBOUR
}

//! @brief Updates standalone root menus
void
WindowManager::updateMenusStandalone(CfgParser::Entry *cfg_root)
{
  // Temporary name, as names are stored uppercase
  string menu_name;

  // Go through all but reserved section names and create menus
  for (CfgParser::Entry *it = cfg_root; it; it = it->get_section_next ()) {
    // Uppercase name
    menu_name = it->get_name();
    Util::to_upper(menu_name);

    // Create new menus, if the name is not reserved and not used
    if (! binary_search(MENU_NAMES_RESERVED,
			MENU_NAMES_RESERVED + MENU_NAMES_RESERVED_COUNT,
			menu_name)
	&& ! getMenu(menu_name)) {
      // Create, parse and add to map
      PMenu *menu = new ActionMenu(this, ROOTMENU_STANDALONE_TYPE,
				   "", menu_name);
      menu->reload(it);
      _menu_map[menu->getName()] = menu;
    }
  }

}

//! @brief Clears the menu map and frees up resources used by menus
void
WindowManager::deleteMenus(void)
{
  map<std::string, PMenu*>::iterator it(_menu_map.begin());
  for (; it != _menu_map.end(); ++it) {
    delete it->second;
  }
  _menu_map.clear();
}
#endif // MENUS

//! @brief Searches for a PWinObj to focus, and then gives it input focus
void
WindowManager::findWOAndFocus(PWinObj *search)
{
	PWinObj *focus = NULL;

	if ((PWinObj::windowObjectExists(search) == true) &&
			(search->isMapped()) && (search->isFocusable()) &&
			_workspaces->getActiveViewport()->isInside(search))  {
		focus = search;
	}

	// search window object didn't exist, go through the MRU list
	if (focus == NULL) {
		list<PWinObj*>::reverse_iterator f_it = _mru_list.rbegin();
		for (; (focus == NULL) && (f_it != _mru_list.rend()); ++f_it) {
			if ((*f_it)->isMapped() && (*f_it)->isFocusable() &&
					_workspaces->getActiveViewport()->isInside(*f_it)) {
				focus = *f_it;
			}
		}
	}

	if (focus != NULL) {
		focus->giveInputFocus();

	}  else if (PWinObj::getFocusedPWinObj() == NULL) {
		_root_wo->giveInputFocus();
		setEwmhActiveWindow(None);
	}
}

//! @brief Finds the Client wich holds the window w, only matching the window.
//! @param win Window to use as criteria when searching
Client*
WindowManager::findClientFromWindow(Window win)
{
	if (win == _screen->getRoot())
		return NULL;
	list<Client*>::iterator it(_client_list.begin());
	for(; it != _client_list.end(); ++it) {
		if (win == (*it)->getWindow())
			return (*it);
	}
	return NULL;
}

//! @brief
Frame*
WindowManager::findFrameFromWindow(Window win)
{
	if (win == _screen->getRoot())
		return NULL;
	list<Frame*>::iterator it(_frame_list.begin());
	for(; it != _frame_list.end(); ++it) {
		if (win == (*it)->getWindow()) { // operator == does more than that
			return (*it);
		}
	}
	return NULL;
}

//! @brief Finds the Client wich holds the Window w.
//! @param win Window to use as criteria when searching.
//! @return Pointer to the client if found, else NULL
Client*
WindowManager::findClient(Window win)
{
	if ((win == None) || (win == _screen->getRoot()))
		return NULL;

	Frame *frame;

	if (_client_list.size()) {
		list<Client*>::iterator it(_client_list.begin());
		for (; it != _client_list.end(); ++it) {
			if (win == (*it)->getWindow()) {
				return (*it);
			} else {
				frame = dynamic_cast<Frame*>((*it)->getParent());
				if ((frame != NULL) && (*frame == win)) {
					return (*it);
				}
			}
		}
	}
	return NULL;
}

//! @brief Insert all clients with the transient for set to win
void
WindowManager::findFamily(std::list<Client*> &client_list, Window win)
{
	// search for windows having transient for set to this window
	list<Client*>::iterator it(_client_list.begin());
	for (; it != _client_list.end(); ++it) {
		if ((*it)->getTransientWindow() == win) {
			client_list.push_back(*it);
		}
	}
}

//! @brief (Un)Maps all windows having transient_for set to win
void
WindowManager::findTransientsToMapOrUnmap(Window win, bool hide)
{
	list<Client*> client_list;
	findFamily(client_list, win);

	list<Client*>::iterator it(client_list.begin());
	for (; it != client_list.end(); ++it) {
		if (static_cast<Frame*>((*it)->getParent())->getActiveChild() == *it) {
			if (hide) {
				(*it)->getParent()->iconify();
			} else {
				(*it)->getParent()->mapWindow();
			}
		}
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
	if (client->getTransientWindow() == None) {
		parent = client;
		win_search = client->getWindow();
	} else {
		parent = findClientFromWindow(client->getTransientWindow());
		win_search = client->getTransientWindow();
	}

	list<Client*> client_list;
	findFamily(client_list, win_search);

	if (parent != NULL) { // make sure parent gets underneath the children
		if (raise == true) {
			client_list.push_front(parent);
		} else {
			client_list.push_back(parent);
		}
	}

	Frame *frame;
	list<Client*>::iterator it(client_list.begin());
	for (; it != client_list.end(); ++it) {
		frame = dynamic_cast<Frame*>((*it)->getParent());
		if ((frame != NULL) && (frame->getActiveChild() == *it)) {
			if (raise == true) {
				frame->raise();
			} else {
				frame->lower();
			}
		}
	}
}

//! @brief
void
WindowManager::removeFromClientList(Client *client)
{
	_client_list.remove(client);
}

//! @brief
void
WindowManager::removeFromFrameList(Frame *frame)
{
	if (_tagged_frame == frame) {
		_tagged_frame = NULL;
	}

	_frame_list.remove(frame);
	_mru_list.remove(frame);
	_frameid_list.push_back(frame->getId());
}

//! @brief
Client*
WindowManager::findClient(const ClassHint *class_hint)
{
	if (! _client_list.size())
		return NULL;
	list<Client*>::iterator it(_client_list.begin());
	for (; it != _client_list.end(); ++it) {
		if (*class_hint == *(*it)->getClassHint())
			return *it;
	}
	return NULL;
}

//! @brief Tries to find a Frame to autogroup with
Frame*
WindowManager::findGroup(AutoProperty *property)
{
	if (_allow_grouping == false) {
		return NULL;
	}

	Frame *frame = NULL;
	Viewport *vp = _workspaces->getActiveViewport();

#define MATCH_GROUP(F,P) \
((P->group_global || ((F)->isMapped() && vp->isInside(F))) && \
((P->group_size == 0) || (signed((F)->size()) < P->group_size)) && \
((((F)->getClassHint()->group.size() > 0) \
	? ((F)->getClassHint()->group == P->group_name) : false) || \
 AutoProperties::matchAutoClass(*(F)->getClassHint(), (Property*) P)))

	// try to match the focused window first
	if (property->group_focused_first &&
			(PWinObj::getFocusedPWinObj() != NULL) &&
			(PWinObj::getFocusedPWinObj()->getType() == PWinObj::WO_CLIENT)) {

		Frame *fo_frame =
			static_cast<Frame*>(PWinObj::getFocusedPWinObj()->getParent());

		if (MATCH_GROUP(fo_frame, property)) {
			frame = fo_frame;
		}
	}

	// search the list of frames
	if (frame == NULL) {
		list<Frame*>::iterator it(_frame_list.begin());
		for (; it != _frame_list.end(); ++it) {
			if (MATCH_GROUP(*it, property)) {
				frame = *it;
				break;
			}
		}
	}

#undef MATCH_GROUP

	return frame;
}

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

//! @brief
Frame*
WindowManager::findFrameFromId(uint id)
{
	list<Frame*>::iterator it(_frame_list.begin());
	for (; it != _frame_list.end(); ++it) {
		if ((*it)->getId() == id)
			return (*it);
	}

	return NULL;
}

//! @brief
uint
WindowManager::findUniqueFrameId(void)
{
	if (!_startup) // always return 0 while scanning windows
		return 0;

	uint id = 0;

	if (_frameid_list.size()) {
		 id = _frameid_list.back();
		 _frameid_list.pop_back();
	} else {
		id = (_frame_list.size() + 1); // this is called before the frame is pushed
	}

	return id;
}

//! @brief
void
WindowManager::setupFrameIds(void)
{
	list<Frame*>::iterator it(_frame_list.begin());
	for (uint i = 1; it != _frame_list.end(); ++i, ++it)
		(*it)->setId(i);
}

//! @brief
void
WindowManager::setTaggedFrame(Frame *frame, bool activate)
{
	_tagged_frame = frame;
	_tag_activate = activate;
}

//! @brief Attaches all marked clients to frame
void
WindowManager::attachMarked(Frame *frame)
{
	list<Client*>::iterator it(_client_list.begin());
	for (; it != _client_list.end(); ++it) {
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
	if (!client)
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
	if (!frame || (_frame_list.size() < 2))
		return NULL;

	Frame *next_frame = NULL;
	list<Frame*>::iterator f_it(find(_frame_list.begin(), _frame_list.end(), frame));

	if (f_it != _frame_list.end()) {
		list<Frame*>::iterator n_it(f_it);

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

//! @brief Finds the previous frame in the list
//!
//! @param frame Frame to search from
//! @param mapped Match only agains mapped frames
//! @param mask Defaults to 0
Frame*
WindowManager::getPrevFrame(Frame* frame, bool mapped, uint mask)
{
	if (!frame || (_frame_list.size() < 2))
		return NULL;

	Frame *next_frame = NULL;
	list<Frame*>::iterator f_it(find(_frame_list.begin(), _frame_list.end(), frame));

	if (f_it != _frame_list.end()) {
		list<Frame*>::iterator n_it(f_it);

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
//! @brief Hides all menus
void
WindowManager::hideAllMenus(void)
{
  map<std::string, PMenu*>::iterator it(_menu_map.begin());
  for (; it != _menu_map.end(); ++it) {
    it->second->unmapAll();
  }
}
#endif // MENUS
// here follows methods for hints and atoms

void
WindowManager::initHints(void)
{
	// Motif hints
	_atom_mwm_hints =
		XInternAtom(_screen->getDpy(), "_MOTIF_WM_HINTS", False);

	// Setup Extended Net WM hints
	_extended_hints_win =
			XCreateSimpleWindow(_screen->getDpy(), _screen->getRoot(),
												  -200, -200, 5, 5, 0, 0, 0);

	XSetWindowAttributes pattr;
	pattr.override_redirect = True;

	XChangeWindowAttributes(_screen->getDpy(), _extended_hints_win,
													CWOverrideRedirect, &pattr);

	AtomUtil::setString(_extended_hints_win, _ewmh_atoms->getAtom(NET_WM_NAME),
											_wm_name);
	AtomUtil::setWindow(_extended_hints_win,
										_ewmh_atoms->getAtom(NET_SUPPORTING_WM_CHECK),
										_extended_hints_win);
	AtomUtil::setWindow(_screen->getRoot(),
										_ewmh_atoms->getAtom(NET_SUPPORTING_WM_CHECK),
									 _extended_hints_win);

	setEwmhSupported();
	AtomUtil::setLong(_screen->getRoot(),
									 _ewmh_atoms->getAtom(NET_NUMBER_OF_DESKTOPS),
									 _config->getWorkspaces());

	_workspaces->getActiveViewport()->hintsUpdate();

	AtomUtil::setLong(_screen->getRoot(),
									 _ewmh_atoms->getAtom(NET_CURRENT_DESKTOP), 0);
}

void
WindowManager::setEwmhSupported(void)
{
	Atom *atoms = _ewmh_atoms->getAtomArray();

	AtomUtil::setAtoms(_screen->getRoot(), _ewmh_atoms->getAtom(NET_SUPPORTED),
										 atoms, _ewmh_atoms->size());

	delete [] atoms;
}

void
WindowManager::setEwmhActiveWindow(Window w)
{
	AtomUtil::setWindow(_screen->getRoot(), _ewmh_atoms->getAtom(NET_ACTIVE_WINDOW), w);
}
