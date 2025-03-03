//
// WindowManager.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2002-2021 the pekwm development team
//
// windowmanager.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "PDecor.hh"
#include "Frame.hh"
#include "Client.hh"
#include "WindowManager.hh"

#include "ActionHandler.hh"
#include "AutoProperties.hh"
#include "Config.hh"
#include "Workspaces.hh"
#include "Util.hh"
#include "X11.hh"

#include "Os.hh"
#include "RegexString.hh"

#include "KeyGrabber.hh"
#include "MenuHandler.hh"
#include "Harbour.hh"
#include "DockApp.hh"
#include "CmdDialog.hh"
#include "SearchDialog.hh"
#include "StatusWindow.hh"
#include "WorkspaceIndicator.hh"

#include "tk/Color.hh"
#include "tk/CfgUtil.hh"
#include "tk/FontHandler.hh"
#include "tk/PFont.hh"
#include "tk/PTexture.hh"
#include "tk/PWinObj.hh"
#include "tk/TextureHandler.hh"
#include "tk/Theme.hh"
#include "tk/X11Util.hh"

#include <cstring>
#include <iostream>
#include <memory>
#include <cassert>

extern "C" {
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#include <X11/Xatom.h>
#include <X11/keysym.h>
}

// include after all includes to get ifndefs right
#include "Compat.hh"

extern "C" {

	static bool is_signal = false;
	static bool is_signal_hup = false;
	static bool is_signal_int_term = false;
	static bool is_signal_chld = false;

	/**
	 * Signal handler setting signal flags.
	 */
	static void
	sigHandler(int signal)
	{
		is_signal = true;

		switch (signal) {
		case SIGHUP:
			is_signal_hup = true;
			break;
		case SIGINT:
		case SIGTERM:
			is_signal_int_term = true;
			break;
		case SIGCHLD:
			is_signal_chld = true;
			break;
		}
	}

} // extern "C"


// WindowManager

/**
 * Create window manager instance and run main routine.
 */
WindowManager*
WindowManager::start(const std::string &config_file,
		     bool replace, bool synchronous)
{
	Display *dpy = XOpenDisplay(0);
	if (! dpy) {
		std::cerr << "Can not open display!" << std::endl
			  << "Your DISPLAY variable currently is set to: "
			  << Util::getEnv("DISPLAY") << std::endl;
		return nullptr;
	}

	// Setup window manager
	Os *os = mkOs();
	WindowManager *wm = new WindowManager(os);
	Workspaces::init();
	if (! pekwm::init(wm, wm, os, dpy, config_file, replace,
			  synchronous)) {
		delete wm;
		wm = nullptr;

	} else {
		P_DBG("pekwm_wm " << getpid() << " starting")

		wm->setupDisplay();
		wm->scanWindows();
		Frame::resetFrameIDs();

		pekwm::rootWo()->setEwmhDesktopNames();
		pekwm::rootWo()->setEwmhDesktopLayout();
		Workspaces::updateClientList();

		// add all frames to the MRU list
		Frame::frame_cit it = Frame::frame_begin();
		for (; it != Frame::frame_end(); ++it) {
			Workspaces::addToMRUBack(*it);
		}

		wm->startSys();
		wm->startBackground(pekwm::theme()->getThemeDir(),
				    pekwm::theme()->getBackground());
		wm->execStartFile();
	}

	return wm;
}

WindowManager::WindowManager(Os *os)
	: _os(os),
	  _shutdown(false),
	  _reload(false),
	  _restart(false),
	  _select(mkOsSelect()),
	  _bg_pid(-1),
	  _sys_process(nullptr),
	  _event_handler(nullptr),
	  _skip_enter(false)
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
}

//! @brief WindowManager destructor
WindowManager::~WindowManager(void)
{
	cleanup();

	MenuHandler::deleteMenus();
	Workspaces::cleanup();

	delete _sys_process;
	delete _select;
	delete _os;
}

//! @brief Checks if the start file is executable then execs it.
void
WindowManager::execStartFile(void)
{
	std::string start_file(pekwm::config()->getStartFile());
	if (start_file.empty()) {
		return;
	}

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
	stopBackground();
	stopSys();

	// update all nonactive clients properties
	Frame::frame_cit it_f(Frame::frame_begin());
	for (; it_f != Frame::frame_end(); ++it_f) {
		(*it_f)->updateInactiveChildInfo();
	}

	if (pekwm::harbour()) {
		pekwm::harbour()->removeAllDockApps();
	}

	// To preserve stacking order when destroying the frames, we go through
	// the PWinObj list from the Workspaces and put all Frames into our own
	// list, then we delete the frames in order.
	std::vector<Frame*> frame_list;
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
	while (Client::client_begin() != Client::client_end()) {
		delete *Client::client_begin();
	}

	if (pekwm::keyGrabber()) {
		pekwm::keyGrabber()->ungrabKeys(X11::getRoot());
	}

	// destroy screen edge
	for (int i=0; i < 4; ++i) {
		Workspaces::remove(_screen_edges[i]);
		delete _screen_edges[i];
		_screen_edges[i]=0;
	}

	if (X11::getDpy()) {
		XInstallColormap(X11::getDpy(), X11::getColormap());
		X11::setInputFocus(PointerRoot);
	}
}

/**
 * Setup display and claim resources.
 */
void
WindowManager::setupDisplay()
{
	_select->add(X11::getFd(), OsSelect::OS_SELECT_READ);

	pekwm::autoProperties()->load();

	Workspaces::setSize(pekwm::config()->getWorkspaces());
	Workspaces::setPerRow(pekwm::config()->getWorkspacesPerRow());

	MenuHandler::createMenus(pekwm::actionHandler());

	XDefineCursor(X11::getDpy(), X11::getRoot(),
		      X11::getCursor(CURSOR_ARROW));

	X11::selectXRandrInput();

	// Create screen edge windows
	screenEdgeCreate();
	screenEdgeMapUnmap();
}

/**
 * Goes through the window and creates Clients/DockApps.
 */
void
WindowManager::scanWindows(void)
{
	// only done once when we start
	if (! pekwm::isStarting()) {
		return;
	}

	// Lets create a list of windows on the display
	Window root, parent;
	std::vector<Window> win_list;
	X11::queryTree(X11::getRoot(), root, parent, win_list);

	// Filter out all windows with the the IconWindowHint set not pointing
	// to themselves, making DockApps work as they are supposed to.
	XWMHints wm_hints;
	std::vector<Window>::iterator it(win_list.begin());
	for (; it != win_list.end(); ++it) {
		if (*it == None) {
			continue;
		}

		if (X11::getWMHints(*it, wm_hints)
		    && (wm_hints.flags&IconWindowHint)
		    && (wm_hints.icon_window != *it)) {
			std::vector<Window>::iterator
				i_it(std::find(win_list.begin(),
					       win_list.end(),
					       wm_hints.icon_window));
			if (i_it != win_list.end()) {
				*i_it = None;
			}
		}
	}

	for (it = win_list.begin(); it != win_list.end(); ++it) {
		if (*it != None) {
			createClient(*it, false);
		}
	}

	// Try to focus the ontop window, if no window we give root focus
	PWinObj *wo = Workspaces::getTopFocusableWO(PWinObj::WO_FRAME);
	if (wo) {
		wo->giveInputFocus();
	} else {
		pekwm::rootWo()->giveInputFocus();
	}

	// Try to find transients for all clients, on restarts ordering might
	// not be correct.
	Client::client_cit it_client = Client::client_begin();
	for (; it_client != Client::client_end(); ++it_client) {
		if ((*it_client)->isTransient()
		    && ! (*it_client)->getTransientForClient()) {
			(*it_client)->findAndRaiseIfTransient();
		}
	}

	// Update client list _once_ after creating all clients
	Workspaces::updateClientList();

	// Discard on start autoproperties, will not be used until next start
	// of pekwm.
	pekwm::autoProperties()->removeApplyOnStart();

	pekwm::setStarted();
}

/**
 * Creates and places screen edge
 */
void
WindowManager::screenEdgeCreate(void)
{
	bool indent = pekwm::config()->getScreenEdgeIndent();

	Config *cfg = pekwm::config();
	RootWO *root_wo = pekwm::rootWo();
	_screen_edges[0] =
		new EdgeWO(root_wo, SCREEN_EDGE_LEFT,
			   indent
			   && (cfg->getScreenEdgeSize(SCREEN_EDGE_LEFT) > 0),
			   cfg);
	_screen_edges[1] =
		new EdgeWO(root_wo, SCREEN_EDGE_RIGHT,
			   indent
			   && (cfg->getScreenEdgeSize(SCREEN_EDGE_RIGHT) > 0),
			   cfg);
	_screen_edges[2] =
		new EdgeWO(root_wo, SCREEN_EDGE_TOP,
			   indent
			   && (cfg->getScreenEdgeSize(SCREEN_EDGE_TOP) > 0),
			   cfg);
	_screen_edges[3] =
		new EdgeWO(root_wo, SCREEN_EDGE_BOTTOM,
			   indent
			   && (cfg->getScreenEdgeSize(SCREEN_EDGE_BOTTOM) > 0),
			   cfg);

	// make sure the edge stays ontop
	for (int i=0; i < 4; ++i) {
		Workspaces::insert(_screen_edges[i]);
	}

	screenEdgeResize();
}

void
WindowManager::screenEdgeResize(void)
{
	Config *cfg = pekwm::config();
	uint l_size = std::max(cfg->getScreenEdgeSize(SCREEN_EDGE_LEFT), 1);
	uint r_size = std::max(cfg->getScreenEdgeSize(SCREEN_EDGE_RIGHT), 1);
	uint t_size = std::max(cfg->getScreenEdgeSize(SCREEN_EDGE_TOP), 1);
	uint b_size = std::max(cfg->getScreenEdgeSize(SCREEN_EDGE_BOTTOM), 1);

	// Left edge
	_screen_edges[0]->moveResize(0, 0,
				     l_size, X11::getHeight());

	// Right edge
	_screen_edges[1]->moveResize(X11::getWidth() - r_size, 0,
				     r_size, X11::getHeight());

	// Top edge
	_screen_edges[2]->moveResize(l_size, 0,
				     X11::getWidth() - l_size - r_size, t_size);

	// Bottom edge
	_screen_edges[3]->moveResize(l_size, X11::getHeight() - b_size,
				     X11::getWidth() - l_size - r_size, b_size);

	for (int i = 0; i < 4; ++i) {
		int indent = cfg->getScreenEdgeIndent();
		int size = cfg->getScreenEdgeSize(_screen_edges[i]->getEdge());
		_screen_edges[i]->configureStrut(indent && size > 0);
	}

	pekwm::rootWo()->updateStrut();
}

void
WindowManager::screenEdgeMapUnmap(void)
{
	for (int i=0; i<4; ++i) {
		EdgeWO *edge = _screen_edges[i];
		Config *cfg = pekwm::config();
		if (cfg->getScreenEdgeSize(edge->getEdge()) > 0
		    && cfg->getEdgeListFromPosition(edge->getEdge())->size()
		       > 0) {
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

	MenuHandler::reloadMenus(pekwm::actionHandler());
	doReloadHarbour();

	pekwm::rootWo()->setEwmhDesktopNames();
	pekwm::rootWo()->setEwmhDesktopLayout();

	// (re)start pekwm_sys if it is enabled and not running, either because
	// a crash or it was not previously enabled.
	if (pekwm::config()->isSysEnabled()) {
		startSys();
	} else {
		stopSys();
	}

	_reload = false;
}

/**
 * Reload main config file.
 */
void
WindowManager::doReloadConfig(void)
{
	Config *cfg = pekwm::config();
	// If any of these changes, re-fetch of all names is required
	bool old_client_unique_name = cfg->getClientUniqueName();
	std::string
		old_client_unique_name_pre(cfg->getClientUniqueNamePre());
	std::string
		old_client_unique_name_post(cfg->getClientUniqueNamePost());

	// Reload configuration
	if (! cfg->load(cfg->getConfigFile())) {
		return;
	}

	// Update what might have changed in the cfg touching the hints
	Workspaces::setSize(cfg->getWorkspaces());
	Workspaces::setPerRow(cfg->getWorkspacesPerRow());
	Workspaces::setNames();

	// Update the ClientUniqueNames if needed
	if ((old_client_unique_name != cfg->getClientUniqueName()) ||
	    (old_client_unique_name_pre != cfg->getClientUniqueNamePre()) ||
	    (old_client_unique_name_post != cfg->getClientUniqueNamePost())) {
		Client::client_cit it = Client::client_begin();
		for (; it != Client::client_end(); ++it) {
			(*it)->readName();
		}
	}

	// Resize the screen edge
	screenEdgeResize();
	screenEdgeMapUnmap();

	pekwm::rootWo()->updateStrut();
}

/**
 * Reload theme file and update decorations.
 */
void
WindowManager::doReloadTheme(bool force)
{
	Config *cfg = pekwm::config();
	Theme *theme = pekwm::theme();

	// auto theme variant handling
	std::string variant = cfg->getThemeVariant();
	CfgUtil::lookupThemeVariant(cfg->getThemeFile(), variant);

	// reload the theme, reloading the decors only if the theme changed.
	bool loaded = theme->load(cfg->getThemeFile(), variant, force);

	// always start the background as the override texture can change
	// without the theme changing
	startBackground(theme->getThemeDir(), theme->getBackground());

	if (loaded) {
		doReloadThemeDecors();
	} else {
		P_TRACE("not reloading decors, theme not changed");
	}
}

void
WindowManager::doReloadThemeDecors()
{
	// Reload the themes on all decors
	std::vector<PDecor*>::const_iterator it = PDecor::pdecor_begin();
	for (; it != PDecor::pdecor_end(); ++it) {
		(*it)->loadDecor();
	}
}

void
WindowManager::startBackground(const std::string& theme_dir,
			       const std::string& theme_texture)
{
	const std::string &override_texture =
		pekwm::config()->getThemeBackgroundOverride();
	const std::string &texture =
		override_texture.empty() ? theme_texture : override_texture;

	if (pekwm::config()->getThemeBackground() && ! texture.empty()) {
		std::string bg_args = theme_dir + texture;
		if (bg_args != _bg_args || ! _os->isProcessAlive(_bg_pid)) {
			stopBackground();

			std::vector<std::string> args;
			args.push_back(BINDIR "/pekwm_bg");
			args.push_back("--load-dir");
			args.push_back(theme_dir + "/backgrounds");
			args.push_back(texture);
			_bg_pid = _os->processExec(args);
			_bg_args = bg_args;

			P_LOG("started " BINDIR "/pekwm_bg --load-dir "
			      << theme_dir + "/backgrounds " << texture
			      << " -> pid " << _bg_pid);
		}
	} else {
		stopBackground();
	}
}

void
WindowManager::stopBackground(void)
{
	if (_bg_pid != -1) {
		// SIGCHILD will take care of waiting for the child
		P_LOG("stopping pekwm_bg pid " << _bg_pid);
		_os->processSignal(_bg_pid, SIGKILL);
	}
	_bg_pid = -1;
	_bg_args = "";
}

/**
 * Start pekwm_sys process if it is enabled in the configuration.
 */
void
WindowManager::startSys()
{
	if (! pekwm::config()->isSysEnabled() || _sys_process) {
		return;
	}
	std::vector<std::string> args;
	args.push_back(BINDIR "/pekwm_sys");
	_sys_process = _os->childExec(args, ChildProcess::CHILD_IO_ALL);
	if (_sys_process) {
		P_LOG("started " BINDIR "/pekwm_sys -> pid "
		      << _sys_process->getPid());
		pekwm::actionHandler()->setSysProcess(_sys_process);
	}
}

void
WindowManager::stopSys()
{
	if (_sys_process) {
		// SIGCHILD will take care of waiting for the child
		P_LOG("stopping pekwm_sys pid " << _sys_process->getPid());
		_os->processSignal(_sys_process->getPid(), SIGKILL);
	}
}

/**
 * Reload mouse configuration and re-grab buttons on all windows.
 */
void
WindowManager::doReloadMouse(void)
{
	Config *cfg = pekwm::config();
	if (! cfg->loadMouseConfig(cfg->getMouseConfigFile())) {
		return;
	}

	Client::client_cit it = Client::client_begin();
	for (; it != Client::client_end(); ++it) {
		(*it)->grabButtons();
	}
}

/**
 * Reload keygrabber configuration and re-grab keys on all windows.
 */
void
WindowManager::doReloadKeygrabber(bool force)
{
	// Reload the keygrabber
	if (! pekwm::keyGrabber()->load(pekwm::config()->getKeyFile(), force)) {
		return;
	}

	pekwm::keyGrabber()->ungrabKeys(X11::getRoot());
	pekwm::keyGrabber()->grabKeys(X11::getRoot());

	// Regrab keys and buttons
	Client::client_cit c_it(Client::client_begin());
	for (; c_it != Client::client_end(); ++c_it) {
		(*c_it)->grabButtons();
		pekwm::keyGrabber()->ungrabKeys((*c_it)->getWindow());
		pekwm::keyGrabber()->grabKeys((*c_it)->getWindow());
	}
}

/**
 * Reload autoproperties.
 */
void
WindowManager::doReloadAutoproperties(void)
{
	if (! pekwm::autoProperties()->load()) {
		return;
	}

	// NOTE: we need to load autoproperties after decor have been updated
	// as otherwise old theme data pointer will be used and sig 11 pekwm.
	Client::client_cit it_c(Client::client_begin());
	for (; it_c != Client::client_end(); ++it_c) {
		(*it_c)->readAutoprops(APPLY_ON_RELOAD);
	}

	Frame::frame_cit it_f(Frame::frame_begin());
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
	pekwm::harbour()->loadTheme();
	pekwm::harbour()->rearrange();
	pekwm::harbour()->restack();
	pekwm::harbour()->updateHarbourSize();
}

/**
 * Reload Xrm Resources and update resource dependent parts (if any at
 * the time).
 */
void
WindowManager::doReloadResources(void)
{
	P_TRACE("RESOURCE_MANAGER changed, reloading resources");
	X11::loadXrmResources();
	if (isResourcesChanged()) {
		P_TRACE("used resources changed, reloading theme");
		doReloadTheme(true);
	}
}

bool
WindowManager::isResourcesChanged(void)
{
	const std::map<std::string, std::string> &resources =
		X11::getRefResources();
	std::map<std::string, std::string>::const_iterator it =
		resources.begin();
	for (; it != resources.end(); ++it) {
		P_TRACE("check referenced resource " << it->first);
		std::string val;
		X11::getXrmString(it->first, val);
		if (val != it->second) {
			return true;
		}
	}
	return false;
}

/**
 * Exit pekwm and restart with the command command
 */
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
WindowManager::handleSignals(void)
{
	// SIGHUP
	if (is_signal_hup || _reload) {
		P_TRACE("handle SIGHUP or reload");
		is_signal_hup = false;
		doReload();
	}

	// Wait for children if a SIGCHLD was received
	if (is_signal_chld) {
		P_TRACE("handle SIGCHLD");
		pid_t pid;
		do {
			pid = waitpid(WAIT_ANY, nullptr, WNOHANG);
			if (pid == -1) {
				if (errno == EINTR) {
					P_TRACE("waitpid interrupted, "
						"retrying");
				}
			} else if (pid == 0) {
				P_TRACE("no more finished child processes");
			} else if (pid == _bg_pid) {
				P_WARN("pekwm_bg stopped unexpectedly");
				showDialog("pekwm: warning",
					   "pekwm_bg stopped unexpectedly");
				_bg_pid = -1;
			} else if (_sys_process
				   && pid == _sys_process->getPid()) {
				P_WARN("pekwm_sys stopped unexpectedly");
				showDialog("pekwm: warning",
					   "pekwm_sys stopped unexpectedly");
				delete _sys_process;
				_sys_process = nullptr;
				pekwm::actionHandler()->setSysProcess(nullptr);
			} else {
				P_TRACE("child process " << pid
					<< " finished");
			}
		} while (pid > 0 || (pid == -1 && errno == EINTR));

		is_signal_chld = false;
	}
}

void
WindowManager::doEventLoop(void)
{
	while (! _shutdown && ! is_signal_int_term) {
		if (is_signal) {
			handleSignals();
			is_signal = false;
		}
		if (_reload) {
			doReload();
		}

		XEvent ev;
		if (getEvent(ev) && ! handleEventHandlerEvent(ev)) {
			handleEvent(ev);
		}
	}
}

/**
 * Get next X11 event, handling timeout actions if any.
 */
bool
WindowManager::getEvent(XEvent &ev)
{
	if (X11::pending() > 0) {
		X11::getNextEvent(ev);
		return true;
	}

	TimeoutAction ta;
	struct timeval *tv;
	if (pekwm::timeouts()->getNextTimeout(&tv, ta)) {
		ActionHandler *action_handler = pekwm::actionHandler();

		Action action(static_cast<enum ActionType>(ta.getKey()));
		ActionEvent ae(action);
		ActionPerformed ap(nullptr, ae);
		action_handler->handleAction(&ap);
	} else if (_select->wait(tv)) {
		if (_select->isSet(X11::getFd(),
				   OsSelect::OS_SELECT_READ)) {
			return X11::getNextEvent(ev);
		}
	}
	return false;
}

bool
WindowManager::handleEventHandlerEvent(XEvent &ev)
{
	if (_event_handler == nullptr) {
		return false;
	}

	EventHandler::Result res;
	switch (ev.type) {
	case ButtonPress:
		res = _event_handler->handleButtonPressEvent(&ev.xbutton);
		break;
	case ButtonRelease:
		res = _event_handler->handleButtonReleaseEvent(&ev.xbutton);
		break;
	case Expose:
		res = _event_handler->handleExposeEvent(&ev.xexpose);
		break;
	case KeyPress:
	case KeyRelease:
		res = _event_handler->handleKeyEvent(&ev.xkey);
		break;
	case MotionNotify:
		res = _event_handler->handleMotionNotifyEvent(&ev.xmotion);
		break;
	default:
		res = EventHandler::EVENT_SKIP;
	}

	switch (res) {
	case EventHandler::EVENT_STOP_PROCESSED:
	case EventHandler::EVENT_STOP_SKIP:
		P_TRACE("removing event handler " << _event_handler);
		setEventHandler(nullptr);
		return res == EventHandler::EVENT_STOP_PROCESSED;
	default:
		return res == EventHandler::EVENT_PROCESSED;
	}
}

void
WindowManager::handleEvent(XEvent &ev)
{
	static ScreenChangeNotification scn;

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
		P_LOG("being replaced by another WM");
		_shutdown = true;
		break;

	default:
#ifdef PEKWM_HAVE_SHAPE
		if (X11::hasExtensionShape()
		    && ev.type == X11::getEventShape()) {
			XShapeEvent *sev = reinterpret_cast<XShapeEvent*>(&ev);
			X11::setLastEventTime(sev->time);
			Client *client = Client::findClient(sev->window);
			if (client) {
				client->handleShapeEvent(sev);
			}
		}
#endif // PEKWM_HAVE_SHAPE
		if (X11::getScreenChangeNotification(&ev, scn)) {
			pekwm::rootWo()->updateGeometry(scn.width, scn.height);
			pekwm::harbour()->updateGeometry();
			screenEdgeResize();

			// Make sure windows are visible after resize
			std::vector<PDecor*>::const_iterator it =
				PDecor::pdecor_begin();
			for (; it != PDecor::pdecor_end(); ++it) {
				Workspaces::placeWoInsideScreen(*it);
			}
		}
		break;
	}
}

//! @brief Handle XKeyEvents
void
WindowManager::handleKeyEvent(XKeyEvent *ev)
{
	bool matched = false;
	ActionEvent *ae = 0;
	PWinObj *wo, *wo_orig;
	wo = wo_orig = PWinObj::getFocusedPWinObj();
	PWinObj::Type type = (wo) ? wo->getType() : PWinObj::WO_SCREEN_ROOT;

	if (wo && wo->getWindow() == ev->window) {
		wo->setLastActivity(ev->time);
	}

	switch (type) {
	case PWinObj::WO_CLIENT:
	case PWinObj::WO_FRAME:
	case PWinObj::WO_SCREEN_ROOT:
	case PWinObj::WO_MENU:
		if (ev->type == KeyPress) {
			ae = pekwm::keyGrabber()->findAction(ev, type, matched);
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

	handleKeyEventAction(ev, ae, wo, wo_orig);

	// Flush Enter events caused by keygrabbing
	if (matched) {
		XEvent e;
		while (X11::checkTypedEvent(EnterNotify, &e)) {
			if (! e.xcrossing.send_event) {
				X11::setLastEventTime(e.xcrossing.time);
			}
		}
	}
}

void
WindowManager::handleKeyEventAction(XKeyEvent *ev, ActionEvent *ae,
				    PWinObj *wo, PWinObj *wo_orig)
{
	if (!ae) {
		return;
	}

	// HACK: Always close CmdDialog and SearchDialog before actions
	if (wo_orig
	    && (wo_orig->getType() == PWinObj::WO_CMD_DIALOG
		|| wo_orig->getType() == PWinObj::WO_SEARCH_DIALOG)) {
		::Action close_action;
		ActionEvent close_ae;

		close_ae.action_list.push_back(close_action);
		close_ae.action_list.back().setAction(ACTION_CLOSE);

		ActionPerformed close_ap(wo_orig, close_ae);
		pekwm::actionHandler()->handleAction(&close_ap);
	}

	ActionPerformed ap(wo, *ae);
	ap.type = ev->type;
	ap.event.key = ev;
	pekwm::actionHandler()->handleAction(&ap);
}

void
WindowManager::handleButtonPressEvent(XButtonEvent *ev)
{
	ActionEvent *ae = nullptr;
	PWinObj *wo = PWinObj::findPWinObj(ev->window);
	if (wo == pekwm::rootWo() && ev->subwindow != None) {
		wo = PWinObj::findPWinObj(ev->subwindow);
	}

	if (wo) {
		wo->setLastActivity(ev->time);

		// in case the event causes the wo to go away, update it
		// before handling the event.
		PWinObj *orig_wo = wo;
		wo = updateWoForFrameClick(ev, orig_wo);
		ae = orig_wo->handleButtonPress(ev);
	}

	if (ae) {
		ActionPerformed ap(wo, *ae);
		ap.type = ev->type;
		ap.event.button = ev;
		pekwm::actionHandler()->handleAction(&ap);
	} else {
		Harbour *harbour = pekwm::harbour();
		DockApp *da = harbour->findDockAppFromFrame(ev->window);
		if (da) {
			harbour->handleButtonEvent(ev, da);
		}
	}
}

void
WindowManager::handleButtonReleaseEvent(XButtonEvent *ev)
{
	ActionEvent *ae = nullptr;
	PWinObj *wo = PWinObj::findPWinObj(ev->window);
	if (wo == pekwm::rootWo() && ev->subwindow != None) {
		wo = PWinObj::findPWinObj(ev->subwindow);
	}

	if (wo) {
		wo->setLastActivity(ev->time);

		// in case the event causes the wo to go away, update it
		// before handling the event.
		PWinObj *orig_wo = wo;
		wo = updateWoForFrameClick(ev, orig_wo);
		ae = orig_wo->handleButtonRelease(ev);
	}

	if (ae) {
		ActionPerformed ap(wo, *ae);
		ap.type = ev->type;
		ap.event.button = ev;
		pekwm::actionHandler()->handleAction(&ap);
	} else {
		Harbour *harbour = pekwm::harbour();
		DockApp *da = harbour->findDockAppFromFrame(ev->window);
		if (da) {
			harbour->handleButtonEvent(ev, da);
		}
	}
}

PWinObj*
WindowManager::updateWoForFrameClick(XButtonEvent *ev, PWinObj *wo)
{
	if (wo->getType() != PWinObj::WO_FRAME) {
		return wo;
	}

	// update selected window object with the one clicked
	// on the titlebar (if any), subwindow check is there
	// to skip titlebar button press
	Frame *frame = static_cast<Frame*>(wo);
	if (ev->subwindow == None
	    && ev->window == frame->getTitleWindow()) {
		wo = frame->getChildFromPos(ev->x);
	} else {
		wo = frame->getActiveChild();
	}

	if (wo) {
		wo->setLastActivity(ev->time);
	}

	return wo;
}

void
WindowManager::handleConfigureRequestEvent(XConfigureRequestEvent *ev)
{
	Client *client = Client::findClientFromWindow(ev->window);

	if (client) {
		Frame* frame = static_cast<Frame*>(client->getParent());
		frame->handleConfigureRequest(ev, client);
	} else {
		DockApp *da = pekwm::harbour()->findDockApp(ev->window);
		if (da) {
			pekwm::harbour()->handleConfigureRequestEvent(ev, da);
		} else {
			// Since this window isn't yet a client lets delegate
			// the configure request back to the window so it can
			// use it.

			XWindowChanges wc;

			wc.x = ev->x;
			wc.y = ev->y;
			wc.width = ev->width;
			wc.height = ev->height;
			wc.sibling = ev->above;
			wc.stack_mode = ev->detail;

			XConfigureWindow(X11::getDpy(), ev->window,
					 ev->value_mask, &wc);
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
	PWinObj *wo = PWinObj::findPWinObj(ev->window);
	if (wo == pekwm::rootWo() && ev->subwindow != None) {
		wo = PWinObj::findPWinObj(ev->subwindow);
	}

	if (wo) {
		ActionEvent *ae = nullptr;
		if (wo->getType() == PWinObj::WO_CLIENT) {
			ae = wo->getParent()->handleMotionEvent(ev);

		} else if (wo->getType() == PWinObj::WO_FRAME) {
			ae = wo->handleMotionEvent(ev);

			// this is done so that clicking the titlebar executes
			// action on the client clicked on
			Frame* frame = static_cast<Frame*>(wo);
			if (ev->subwindow != None) {
				wo = frame->getActiveChild();
			} else {
				wo = frame->getChildFromPos(ev->x);
			}
		} else {
			ae = wo->handleMotionEvent(ev);
		}

		if (ae) {
			ActionPerformed ap(wo, *ae);
			ap.type = ev->type;
			ap.event.motion = ev;

			pekwm::actionHandler()->handleAction(&ap);
		}
	} else {
		DockApp* da =
			pekwm::harbour()->findDockAppFromFrame(ev->window);
		if (da) {
			pekwm::harbour()->handleMotionNotifyEvent(ev, da);
		}
	}
}

void
WindowManager::handleMapRequestEvent(XMapRequestEvent *ev)
{
	PWinObj *wo = PWinObj::findPWinObj(ev->window);
	if (wo) {
		wo->handleMapRequest(ev);
	} else {
		createClient(ev->window, true);
	}
}

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
		DockApp *da = pekwm::harbour()->findDockApp(ev->window);
		if (da) {
			if (ev->window == ev->event) {
				pekwm::harbour()->removeDockApp(da);
			}
		}
	}

	if (wo_type != PWinObj::WO_MENU
	    && wo_type != PWinObj::WO_CMD_DIALOG
	    && wo_type != PWinObj::WO_SEARCH_DIALOG
	    && ! PWinObj::getFocusedPWinObj()) {
		Workspaces::findWOAndFocus(wo_search);
	}
}

void
WindowManager::handleDestroyWindowEvent(XDestroyWindowEvent *ev)
{
	Client *client = Client::findClientFromWindow(ev->window);

	if (client) {
		PWinObj *wo_search = client->getParent();
		client->handleDestroyEvent(ev);

		if (! PWinObj::getFocusedPWinObj()) {
			Workspaces::findWOAndFocus(wo_search);
		}
	} else {
		DockApp *da = pekwm::harbour()->findDockApp(ev->window);
		if (da) {
			da->setAlive(false);
			pekwm::harbour()->removeDockApp(da);
		}
	}
}

void
WindowManager::handleEnterNotify(XCrossingEvent *ev)
{
	// Window being entered is marked to cause the upcoming enter
	// event to have no effect, multiple consequitive enter events on
	// this window just keeps on making the next enter event being
	// marked to be skipped.
	if (PWinObj::isSkipEnterAfter(ev->window)) {
		_skip_enter = true;
		return;
	}
	PWinObj::setSkipEnterAfter(nullptr);
	if (_skip_enter) {
		_skip_enter = false;
		return;
	}

	// Clear event queue
	while (X11::checkTypedEvent(EnterNotify,
				    reinterpret_cast<XEvent*>(ev))) {
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

			pekwm::actionHandler()->handleAction(&ap);
		}
	}
}

void
WindowManager::handleLeaveNotify(XCrossingEvent *ev)
{
	// Clear event queue
	while (X11::checkTypedEvent(LeaveNotify,
				    reinterpret_cast<XEvent*>(ev))) {
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
			pekwm::actionHandler()->handleAction(&ap);
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
	if (wo == nullptr) {
		return;
	}

	// To simplify logic, changing client in frame wouldn't update the
	// focused window because wo != focused_wo woule be true.
	if (wo->getType() == PWinObj::WO_FRAME) {
		wo = static_cast<Frame*>(wo)->getActiveChild();
	}

	if (! wo->isFocusable() || ! wo->isMapped()) {
		Workspaces::findWOAndFocus(nullptr);

	} else if (wo != PWinObj::getFocusedPWinObj()) {
		// If it's a valid FocusIn event with accepatable target lets
		// flush all EnterNotify and LeaveNotify as they can interfere
		// with focusing if Sloppy or Follow like focus model is used.
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

		PWinObj *focused_wo = PWinObj::getFocusedPWinObj();

		// unfocus last window
		if (focused_wo) {
			if (focused_wo->getType() == PWinObj::WO_CLIENT) {
				focused_wo->getParent()->setFocused(false);
			} else {
				focused_wo->setFocused(false);
			}
			Workspaces::setLastFocused(Workspaces::getActive(),
						   focused_wo);
		}

		PWinObj::setFocusedPWinObj(wo);

		if (wo->getType() == PWinObj::WO_CLIENT) {
			wo->getParent()->setFocused(true);
			pekwm::rootWo()->setEwmhActiveWindow(wo->getWindow());

			// update the MRU list (except for skip focus windows,
			// see #297)
			Client* client = static_cast<Client*>(wo);
			if (! client->isSkip(SKIP_FOCUS_TOGGLE)) {
				Frame *frame =
					static_cast<Frame*>(wo->getParent());
				Workspaces::addToMRUFront(frame);
			}
		} else {
			wo->setFocused(true);
			pekwm::rootWo()->setEwmhActiveWindow(None);
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
	P_DBG("XClientMessageEvent window: " << ev->window << " (root: "
	      << (ev->window == X11::getRoot()) << ") message format: "
	      << ev->format << " type: " << ev->message_type << " data.l[0] "
	      << ev->data.l[0]);
	if (ev->message_type == X11::getAtom(PEKWM_CMD)) {
		// _PEKWM_CMD is handled independent of client
		handlePekwmCmd(ev);
	} if (ev->window == X11::getRoot()) {
		if (ev->format == 32) {
			if (ev->message_type
			    == X11::getAtom(NET_CURRENT_DESKTOP)) {
				Workspaces::setWorkspace(ev->data.l[0], true);
			} else if (ev->message_type ==
				   X11::getAtom(NET_NUMBER_OF_DESKTOPS)) {
				if (ev->data.l[0] > 0) {
					Workspaces::setSize(ev->data.l[0]);
				}
			}
		}
	} else if (ev->message_type
		   == X11::getAtom(NET_RESTACK_WINDOW)) {
		handleNetRestackWindow(ev);
	} else if (ev->message_type
		   == X11::getAtom(NET_REQUEST_FRAME_EXTENTS)) {
		handleNetRequestFrameExtents(ev->window);
	} else {
		Client *client = Client::findClientFromWindow(ev->window);
		if (client) {
			Frame *frame =
				static_cast<Frame*>(client->getParent());
			ActionEvent *ae =
				frame->handleClientMessage(ev, client);
			if (ae) {
				ActionPerformed ap(frame, *ae);
				ap.type = ev->type;
				ap.event.client = ev;
				pekwm::actionHandler()->handleAction(&ap);
			}
		}
	}
}

/**
 * Handle _NET_RESTACK_WINDOW request:
 *
 *  window = window to restack
 *  format = 32
 *  data.l[0] = source indication
 *  data.l[1] = sibling window
 *  data.l[2] = detail
 *
 * Used by pagers to re-stack windows instead of using a ConfigureRequest
 * with CWSibling and CWStackMode as they can be ignored by the window
 * manager.
 */
void
WindowManager::handleNetRestackWindow(XClientMessageEvent *ev)
{
	if (ev->format != 32) {
		P_TRACE("invalid _NET_RESTACK_WINDOW request, expected format "
			"32, got " << ev->format);
		return;
	}

	Window sibling_win = ev->data.l[1];
	long detail = ev->data.l[2];

	P_TRACE("_NET_RESTACK_WINDOW client=" << ev->window << " sibling="
		<< sibling_win << " detail=" << detail);

	Client *client = Client::findClientFromWindow(ev->window);
	Client *sibling = sibling_win == None
		? nullptr : Client::findClientFromWindow(sibling_win);
	if (client == nullptr || (sibling_win != None && sibling == nullptr)) {
		return;
	}

	Workspaces::restack(client->getParent(),
			    sibling ? sibling->getParent() : nullptr,
			    detail);
}

void
WindowManager::handleNetRequestFrameExtents(Window win)
{
	ThemeState *state;
	MwmThemeState mwm_state;
	Client *client = Client::findClientFromWindow(win);
	if (client == nullptr) {
		MwmHints hints;
		X11Util::readMwmHints(win, hints);
		mwm_state.setHints(hints);
		state = &mwm_state;
	} else {
		state = static_cast<Frame*>(client->getParent());
	}

	// _NET_REQUEST_FRAME_EXTENTS is not a required to be correct,
	// autoproperties and such are ignored and also the decor type.
	Theme::PDecorData *data = pekwm::theme()->getPDecorData("DEFAULT");
	if (data) {
		ThemeGm theme_gm(data);

		Cardinal extents[4];
		extents[0] = theme_gm.bdLeft(state);
		extents[1] = theme_gm.bdRight(state);
		extents[2] = theme_gm.bdTop(state)
			+ theme_gm.titleHeight(state);
		extents[3] = theme_gm.bdBottom(state);

		if (Debug::isLevel(Debug::LEVEL_TRACE)) {
			std::ostringstream msg;
			msg << "setting _NET_FRAME_EXTENTS (" << extents[0];
			msg << " " << extents[1] << " " << extents[2] << " ";
			msg << extents[3] << ") on " << win;
			P_TRACE(msg.str());
		}
		X11::setCardinals(win, NET_FRAME_EXTENTS, extents, 4);
	}
}

void
WindowManager::handleColormapEvent(XColormapEvent *ev)
{
	Client *client = Client::findClient(ev->window);
	if (client) {
		Frame* frame = static_cast<Frame*>(client->getParent());
		client = static_cast<Client*>(frame->getActiveChild());
		client->handleColormapChange(ev);
	}
}

//! @brief Handles XPropertyEvents
void
WindowManager::handlePropertyEvent(XPropertyEvent *ev)
{
	if (ev->window == X11::getRoot()) {
		if (ev->atom == X11::getAtom(RESOURCE_MANAGER)) {
			doReloadResources();
		} else if (ev->atom == X11::getAtom(PEKWM_THEME_VARIANT)) {
			doReloadTheme(false);
		} else {
			return pekwm::rootWo()->handlePropertyChange(ev);
		}
	}

	Client *client = Client::findClientFromWindow(ev->window);
	if (client) {
		Frame* frame = static_cast<Frame*>(client->getParent());
		frame->handlePropertyChange(ev, client);
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

		pekwm::actionHandler()->handleAction(&ap);
	}
}

// Event handling routines stop ============================================

Client*
WindowManager::createClient(Window window, bool is_new)
{
	XWindowAttributes attr;
	X11::getWindowAttributes(window, attr);
	if (attr.override_redirect
	    || (! is_new && attr.map_state == IsUnmapped)) {
		return nullptr;
	}

	XWMHints wm_hints;
	if (X11::getWMHints(window, wm_hints)
	    && (wm_hints.flags&StateHint)
	    && (wm_hints.initial_state == WithdrawnState)) {
		pekwm::harbour()->addDockApp(new DockApp(window));
		return nullptr;
	}

	ClientInitConfig initConfig;
	Client *client = new Client(window, initConfig, is_new);
	if (! client->isAlive()) {
		delete client;
		return nullptr;
	}

	if (initConfig.parent_is_new) {
		PWinObj *wo = client->getParent();
		Workspaces::handleFullscreenBeforeRaise(wo);
		Workspaces::insert(wo, true);
	}
	if (! pekwm::isStarting()) {
		// Skip updating client list while starting, it will be done
		// once after all windows hae been scanned
		Workspaces::updateClientList();
	}

	// Make sure the window is mapped, this is done after it has been
	// added to the decor/frame as otherwise IsViewable state won't
	// be correct and we don't know whether or not to place the window
	if (initConfig.map) {
		client->mapWindow();
	}

	// Focus was requested by the configuration, look out for
	// focus stealing.
	if (initConfig.focus) {
		PWinObj *wo = PWinObj::getFocusedPWinObj();
		Config *cfg = pekwm::config();
		Time time_protect =
			static_cast<Time>(cfg->getFocusStealProtect());

		if (wo != nullptr
		    && wo->isMapped()
		    && wo->isKeyboardInput()
		    && time_protect
		    && time_protect >= (X11::getLastEventTime()
					- wo->getLastActivity())) {
			// WO exists, is mapped, and time protect is
			// active and within it's limits.
		} else {
			client->getParent()->giveInputFocus();
		}
	}

	return client;
}

void
WindowManager::handlePekwmCmd(XClientMessageEvent *ev)
{
	if (! recvPekwmCmd(ev)) {
		return;
	}

	Action action;
	P_TRACE("received _PEKWM_CMD: " << _pekwm_cmd_buf);
	if (ActionConfig::parseAction(_pekwm_cmd_buf, action, CMD_OK)) {
		ActionEvent ae;
		ae.action_list.push_back(action);

		PWinObj *wo = nullptr;
		if (ev->window != X11::getRoot()) {
			wo = Client::findClient(ev->window);
		}

		ActionPerformed ap(wo, ae);
		pekwm::actionHandler()->handleAction(&ap);
	}

	_pekwm_cmd_buf = "";
}

/**
 * Receive data from XClientMessage building up the _pekwm_cmd_buf,
 * command can be split up in multiple messages due to size
 * restrictions.
 *
 * @return true on complete message, false on error and incomplete message.
 */
bool
WindowManager::recvPekwmCmd(XClientMessageEvent *ev)
{
	size_t last = sizeof(ev->data.b) - 1;
	enum PekwmCmdBuf op = static_cast<enum PekwmCmdBuf>(ev->data.b[last]);
	switch (op) {
	case PEKWM_CMD_SINGLE:
		_pekwm_cmd_buf = ev->data.b;
		return true;
	case PEKWM_CMD_MULTI_FIRST:
		ev->data.b[last] = 0;
		_pekwm_cmd_buf = ev->data.b;
		return false;
	case PEKWM_CMD_MULTI_CONT:
	case PEKWM_CMD_MULTI_END:
		if (_pekwm_cmd_buf.empty()) {
			P_DBG("invalid _PEKWM_CMD, continuation on empty "
			      "buffer");
			_pekwm_cmd_buf = "";
			return false;
		}

		// multi-message command, continuation.
		_pekwm_cmd_buf.append(ev->data.b,
				      std::min(std::strlen(ev->data.b), last));
		if (_pekwm_cmd_buf.size() > 1024) {
			P_DBG("maximum _PEKWM_CMD message size reached, drop");
			_pekwm_cmd_buf = "";
			return false;
		}
		return op == PEKWM_CMD_MULTI_END;
	default:
		// invalid data
		P_DBG("invalid _PEKMW_CMD, last byte " << op
		      << " not in range 0-3");
		_pekwm_cmd_buf = "";
		return false;
	}
}

/**
 * Show dialog with the set title and message using pekwm_dialog.
 */
void
WindowManager::showDialog(const std::string &title, const std::string &msg)
{
	std::vector<std::string> args;
	args.push_back(BINDIR "/pekwm_dialog");
	args.push_back(msg);
	_os->processExec(args);
}
