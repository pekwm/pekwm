//
// ManagerWindows.cc for pekwm
// Copyright (C) 2009-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Config.hh"
#include "Debug.hh"
#include "ActionHandler.hh"
#include "ManagerWindows.hh"
#include "Workspaces.hh"
#include "Util.hh"

#include <string>

extern "C" {
#include <unistd.h>
}

// Static initializers
const std::string HintWO::WM_NAME = "pekwm";
const unsigned int HintWO::DISPLAY_WAIT = 10;

const unsigned long RootWO::EVENT_MASK =
	StructureNotifyMask|PropertyChangeMask|
	SubstructureNotifyMask|SubstructureRedirectMask|
	ColormapChangeMask|FocusChangeMask|EnterWindowMask|
	ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
const unsigned long RootWO::EXPECTED_DESKTOP_NAMES_LENGTH = 256;

/**
 * Hint window constructor, creates window and sets supported
 * protocols.
 */
HintWO::HintWO(Window root)
	: PWinObj(false)
{
	_type = WO_SCREEN_HINT;
	setLayer(LAYER_NONE);

	_window = X11::createWmWindow(X11::getRoot(), -200, -200, 5, 5,
				      InputOutput, PropertyChangeMask);

	// Set hints not being updated
	X11::setUtf8String(_window, NET_WM_NAME, WM_NAME);
	X11::setWindow(_window, NET_SUPPORTING_WM_CHECK, _window);
}

/**
 * Hint WO destructor, destroy hint window.
 */
HintWO::~HintWO(void)
{
	X11::destroyWindow(_window);
}

/**
 * Get current time of server by generating an event and reading the
 * timestamp on it.
 *
 * @return Time on server.
 */
Time
HintWO::getTime(void)
{
	XEvent event;

	// Generate event on ourselves
	X11::changeProperty(_window,
			    X11::getAtom(WM_CLASS), X11::getAtom(STRING),
			    8, PropModeAppend, 0, 0);
	XWindowEvent(X11::getDpy(), _window, PropertyChangeMask, &event);

	return event.xproperty.time;
}

/**
 * Claim ownership over the current display.
 *
 * @param replace Replace current running window manager.
 */
bool
HintWO::claimDisplay(bool replace)
{
	bool status = true;

	// Get atom for the current screen and it's owner
	std::string default_str = std::to_string(DefaultScreen(X11::getDpy()));
	std::string session_name("WM_S" + default_str);
	Atom session_atom = X11::getAtomId(session_name);
	Window session_owner =
		XGetSelectionOwner(X11::getDpy(), session_atom);

	if (session_owner && session_owner != _window) {
		if (! replace) {
			P_LOG("window manager already running");
			return false;
		}

		X11::sync(False);
		setXErrorsIgnore(true);
		uint errors_before = xerrors_count;

		// Select event to get notified when current owner dies.
		X11::selectInput(session_owner, StructureNotifyMask);

		X11::sync(False);
		setXErrorsIgnore(false);
		if (errors_before != xerrors_count) {
			session_owner = None;
		}
	}

	Time timestamp = getTime();

	XSetSelectionOwner(X11::getDpy(), session_atom, _window, timestamp);
	if (XGetSelectionOwner(X11::getDpy(), session_atom) == _window) {
		if (session_owner) {
			// Wait for the previous window manager to go away and
			// update owner.
			status = claimDisplayWait(session_owner);
			if (status) {
				claimDisplayOwner(session_atom, timestamp);
			}
		}
	} else {
		std::cerr << "pekwm: unable to replace current window manager."
			  << std::endl;
		status = false;
	}

	return status;
}


/**
 * After claiming the display, wait for the previous window manager to
 * go away.
 */
bool
HintWO::claimDisplayWait(Window session_owner)
{
	XEvent event;

	P_LOG("waiting for previous window manager to exit");

	for (uint waited = 0; waited < HintWO::DISPLAY_WAIT; ++waited) {
		if (XCheckWindowEvent(X11::getDpy(), session_owner,
				      StructureNotifyMask, &event)
		    && event.type == DestroyNotify) {
			return true;
		}

		sleep(1);
	}

	P_LOG("previous window manager did not exit");

	return false;
}

/**
 * Send message updating the owner of the screen.
 */
void
HintWO::claimDisplayOwner(Window session_atom, Time timestamp)
{
	XEvent event;
	Window root = X11::getRoot();

	event.xclient.type = ClientMessage;
	event.xclient.message_type = X11::getAtom(MANAGER);
	event.xclient.display = X11::getDpy();
	event.xclient.window = root;
	event.xclient.format = 32;
	event.xclient.data.l[0] = timestamp;
	event.xclient.data.l[1] = session_atom;
	event.xclient.data.l[2] = _window;
	event.xclient.data.l[3] = 0;

	XSendEvent(X11::getDpy(), root, false, SubstructureNotifyMask, &event);
}

/**
 * Root window constructor, reads geometry and sets basic atoms.
 */
RootWO::RootWO(Window root, HintWO *hint_wo, Config *cfg)
	: PWinObj(false),
	  _hint_wo(hint_wo),
	  _cfg(cfg)
{
	_type = WO_SCREEN_ROOT;
	setLayer(LAYER_NONE);
	_mapped = true;

	_window = root;
	_gm.width = X11::getWidth();
	_gm.height = X11::getHeight();

	X11::sync(False);
	setXErrorsIgnore(true);
	uint errors_before = xerrors_count;

	// Select window events
	X11::selectInput(_window, RootWO::EVENT_MASK);

	X11::sync(False);
	setXErrorsIgnore(false);
	if (errors_before != xerrors_count) {
		std::cerr << "pekwm: root window unavailable, can't start!"
			  << std::endl;
		throw StopException("stop");
	}

	// Set hits on the hint window, these are not updated so they are
	// set in the constructor.
	X11::setCardinal(_window, NET_WM_PID, static_cast<Cardinal>(getpid()));
	X11::setString(_window, WM_CLIENT_MACHINE, Util::getHostname());

	X11::setWindow(_window, NET_SUPPORTING_WM_CHECK, _hint_wo->getWindow());
	X11::setEwmhAtomsSupport(_window);
	X11::setCardinal(_window, NET_NUMBER_OF_DESKTOPS,
			 static_cast<Cardinal>(_cfg->getWorkspaces()));
	X11::setCardinal(_window, NET_CURRENT_DESKTOP, 0);

	Cardinal desktop_geometry[2];
	desktop_geometry[0] = static_cast<Cardinal>(_gm.width);
	desktop_geometry[1] = static_cast<Cardinal>(_gm.height);
	X11::setCardinals(_window, NET_DESKTOP_GEOMETRY, desktop_geometry, 2);

	woListAdd(this);
	_wo_map[_window] = this;

	initStrutHead();
}

/**
 * Root window destructor, clears atoms set.
 */
RootWO::~RootWO(void)
{
	// Remove atoms, PID will not be valid on shutdown.
	X11::unsetProperty(_window, NET_WM_PID);
	X11::unsetProperty(_window, WM_CLIENT_MACHINE);

	_wo_map.erase(_window);
	woListRemove(this);
}

/**
 * Button press event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleButtonPress(XButtonEvent *ev)
{
	std::vector<ActionEvent>* el =
		_cfg->getMouseActionList(MOUSE_ACTION_LIST_ROOT);
	return ActionHandler::findMouseAction(ev->button, ev->state,
					      MOUSE_EVENT_PRESS, el);
}

/**
 * Button release event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleButtonRelease(XButtonEvent *ev)
{
	MouseEventType mb = MOUSE_EVENT_RELEASE;

	// first we check if it's a double click
	if (X11::isDoubleClick(ev->window, ev->button - 1, ev->time,
			       _cfg->getDoubleClickTime())) {
		X11::setLastClickID(ev->window);
		X11::setLastClickTime(ev->button - 1, 0);

		mb = MOUSE_EVENT_DOUBLE;

	} else {
		X11::setLastClickID(ev->window);
		X11::setLastClickTime(ev->button - 1, ev->time);
	}

	std::vector<ActionEvent>* el =
		_cfg->getMouseActionList(MOUSE_ACTION_LIST_ROOT);
	return ActionHandler::findMouseAction(ev->button, ev->state, mb, el);
}

/**
 * Motion event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleMotionEvent(XMotionEvent *ev)
{
	unsigned int button = X11::getButtonFromState(ev->state);
	std::vector<ActionEvent>* el =
		_cfg->getMouseActionList(MOUSE_ACTION_LIST_ROOT);
	return ActionHandler::findMouseAction(button, ev->state,
					      MOUSE_EVENT_MOTION, el);
}

/**
 * Enter event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleEnterEvent(XCrossingEvent *ev)
{
	std::vector<ActionEvent>* el =
		_cfg->getMouseActionList(MOUSE_ACTION_LIST_ROOT);
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
					      MOUSE_EVENT_ENTER, el);
}

/**
 * Leave event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleLeaveEvent(XCrossingEvent *ev)
{
	std::vector<ActionEvent>* el =
		_cfg->getMouseActionList(MOUSE_ACTION_LIST_ROOT);
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
					      MOUSE_EVENT_LEAVE, el);
}

/**
 * Makes sure the Geometry is inside the screen.
 */
void
RootWO::placeInsideScreen(Geometry& gm, bool without_edge, bool fullscreen,
			  bool maximized_virt, bool maximized_horz)
{
	uint head_nr = X11::getNearestHead(gm.x + gm.width / 2,
					   gm.y + gm.height / 2);
	Geometry head;
	if (fullscreen) {
		X11::getHeadInfo(head_nr, gm);
		return;
	}
	if (without_edge) {
		X11::getHeadInfo(head_nr, head);
	} else {
		getHeadInfoWithEdge(head_nr, head);
	}

	if (maximized_horz) {
		gm.x = head.x;
		gm.width = head.width;
	}
	if (gm.x + gm.width > head.x + head.width) {
		gm.x = head.x + head.width - gm.width;
	}
	if (gm.x < head.x) {
		gm.x = head.x;
	}

	if (maximized_virt) {
		gm.y = head.y;
		gm.height = head.height;
	}
	if (gm.y + gm.height > head.y + head.height) {
		gm.y = head.y + head.height - gm.height;
	}
	if (gm.y < head.y) {
		gm.y = head.y;
	}
}

/**
 * Fill information about head and the strut.
 */
void
RootWO::getHeadInfoWithEdge(uint num, Geometry &head)
{
	if (! X11::getHeadInfo(num, head)) {
		return;
	}

	int strut_val;
	const Strut &strut = _strut_head[num];

	// Remove the strut area from the head info
	strut_val = (head.x == 0)
		? std::max(_strut.left, strut.left) : strut.left;
	head.x += strut_val;
	head.width -= strut_val;

	strut_val = ((head.x + head.width) == _gm.width)
		? std::max(_strut.right, strut.right) : strut.right;
	head.width -= strut_val;

	strut_val = (head.y == 0)
		? std::max(_strut.top, strut.top) : strut.top;
	head.y += strut_val;
	head.height -= strut_val;

	strut_val = (head.y + head.height == _gm.height)
		? std::max(_strut.bottom, strut.bottom) : strut.bottom;
	head.height -= strut_val;
}


void
RootWO::updateGeometry(uint width, uint height)
{
	if (! X11::updateGeometry(width, height)) {
		return;
	}

	initStrutHead();

	resize(width, height);
	updateStrut();
}

/**
 * Adds a strut to the strut list, updating _NET_WORKAREA
 */
void
RootWO::addStrut(Strut *strut)
{
	P_TRACE("adding strut "
		<< " l: " << strut->left << " r: " << strut->right
		<< " t: " << strut->top << " b: " << strut->bottom);
	_struts.push_back(strut);
	updateStrut();
}

/**
 * Removes a strut from the strut list, updating _NET_WORKAREA
 */
void
RootWO::removeStrut(Strut *strut)
{
	P_TRACE("removing strut "
		<< " l: " << strut->left << " r: " << strut->right
		<< " t: " << strut->top << " b: " << strut->bottom);

	std::vector<Strut*>::iterator it =
		std::find(_struts.begin(), _struts.end(), strut);
	if (it != _struts.end()) {
		_struts.erase(it);
	}
	updateStrut();
}

/**
 * Updating _NET_WORKAREA using the list of registered struts.
 */
void
RootWO::updateStrut(void)
{
	// Reset strut data.
	_strut.left = 0;
	_strut.right = 0;
	_strut.top = 0;
	_strut.bottom = 0;

	std::vector<Strut>::iterator hit = _strut_head.begin();
	for (; hit != _strut_head.end(); ++hit) {
		hit->left = 0;
		hit->right = 0;
		hit->top = 0;
		hit->bottom = 0;
	}

	std::vector<Strut*>::iterator it = _struts.begin();
	for (; it != _struts.end(); ++it) {
		updateMaxStrut(&_strut, *it);

		uint head = static_cast<uint>((*it)->head);
		if (head < _strut_head.size()) {
			updateMaxStrut(&_strut_head[head], *it);
		}
	}

	// Update hints on the root window
	Geometry workarea(_strut.left, _strut.top,
			  _gm.width - _strut.left - _strut.right,
			  _gm.height - _strut.top - _strut.bottom);

	setEwmhWorkarea(workarea);
}

void
RootWO::updateMaxStrut(Strut *max_strut, const Strut *strut)
{
	if (max_strut->left < strut->left) {
		max_strut->left = strut->left;
	}
	if (max_strut->right < strut->right) {
		max_strut->right = strut->right;
	}
	if (max_strut->top < strut->top) {
		max_strut->top = strut->top;
	}
	if (max_strut->bottom < strut->bottom) {
		max_strut->bottom = strut->bottom;
	}
}

/**
 * Handling of XPropertyEvent on the Root window. Entry point for EWMH
 * message handling.
 */
void
RootWO::handlePropertyChange(XPropertyEvent *ev)
{
	if (ev->atom == X11::getAtom(NET_DESKTOP_NAMES)) {
		readEwmhDesktopNames();
		Workspaces::setNames();
	}
}

/**
 * Update _NET_WORKAREA property.
 *
 * @param workarea Geometry with work area.
 */
void
RootWO::setEwmhWorkarea(const Geometry &workarea)
{
	Cardinal workarea_array[4];
	workarea_array[0] = static_cast<Cardinal>(workarea.x);
	workarea_array[1] = static_cast<Cardinal>(workarea.y);
	workarea_array[2] = static_cast<Cardinal>(workarea.width);
	workarea_array[3] = static_cast<Cardinal>(workarea.height);

	P_TRACE("update _NET_WORKAREA "
		<< " x: " << workarea.x << " y: " << workarea.y
		<< " w: " << workarea.width << " h: " << workarea.height);

	X11::setCardinals(_window, NET_WORKAREA, workarea_array, 4);
}

/**
 * Update _NET_ACTIVE_WINDOW property.
 *
 * @param win Window to set as active window.
 */
void
RootWO::setEwmhActiveWindow(Window win)
{
	X11::setWindow(X11::getRoot(), NET_ACTIVE_WINDOW, win);
}

/**
 * Reads the _NET_DESKTOP_NAMES hint and sets the workspaces names accordingly.
 */
void
RootWO::readEwmhDesktopNames(void)
{
	uchar *data;
	ulong data_length;
	if (X11::getProperty(X11::getRoot(), X11::getAtom(NET_DESKTOP_NAMES),
			     X11::getAtom(UTF8_STRING),
			     EXPECTED_DESKTOP_NAMES_LENGTH,
			     &data, &data_length)) {
		_cfg->setDesktopNamesUTF8(reinterpret_cast<char*>(data),
					  data_length);

		X11::free(data);
	}
}

/**
 * Update _NET_DESKTOP_NAMES property on the root window.
 */
void
RootWO::setEwmhDesktopNames(void)
{
	unsigned char *desktopnames = 0;
	unsigned int length = 0;
	_cfg->getDesktopNamesUTF8(&desktopnames, &length);

	if (desktopnames) {
		X11::setUtf8StringArray(X11::getRoot(), NET_DESKTOP_NAMES,
					desktopnames, length);
		delete [] desktopnames;
	}
}

/**
 * Update _NET_DESKTOP_LAYOUT property on the root window.
 */
void
RootWO::setEwmhDesktopLayout(void)
{
	// This property is defined to be set by the pager, however, as pekwm
	// displays a "pager" when changing workspaces and other applications
	// might want to read this information set it anyway.
	Cardinal desktop_layout[] = {
		NET_WM_ORIENTATION_HORZ,
		static_cast<Cardinal>(Workspaces::getPerRow()),
		static_cast<Cardinal>(Workspaces::getRows()),
		NET_WM_TOPLEFT
	};
	X11::setCardinals(X11::getRoot(), NET_DESKTOP_LAYOUT, desktop_layout,
			  sizeof(desktop_layout)/sizeof(desktop_layout[0]));
}

/**
 * Edge window constructor, create window, setup strut and register
 * window.
 */
EdgeWO::EdgeWO(RootWO* root, EdgeType edge, bool set_strut,
	       Config* cfg)
	: PWinObj(false),
	  _root(root),
	  _edge(edge),
	  _cfg(cfg)
{
	_type = WO_SCREEN_EDGE;
	setLayer(LAYER_NONE); // hack, goes over LAYER_MENU
	_sticky = true; // don't map/unmap
	_iconified = true; // hack, to be ignored when placing
	_focusable = false; // focusing input only windows crashes X

	_window = X11::createWmWindow(root->getWindow(), 0, 0, 1, 1,
				      InputOnly,
				      EnterWindowMask|LeaveWindowMask|
				      ButtonPressMask|ButtonReleaseMask);

	configureStrut(set_strut);
	_root->addStrut(&_strut);

	woListAdd(this);
	_wo_map[_window] = this;
}

/**
 * Edge window destructor, remove strut and destroy window resources.
 */
EdgeWO::~EdgeWO(void)
{
	_root->removeStrut(&_strut);
	_wo_map.erase(_window);
	woListRemove(this);

	X11::destroyWindow(_window);
}

/**
 * Configure strut on edge window.
 *
 * @param set_strut If true, set actual values on the strut, false sets all to
 * 0.
 */
void
EdgeWO::configureStrut(bool set_strut)
{
	// Reset value, on strut to zero.
	_strut.left = _strut.right = _strut.top = _strut.bottom = 0;

	// Set strut if requested.
	if (set_strut) {
		switch (_edge) {
		case SCREEN_EDGE_TOP:
			_strut.top = _gm.height;
			break;
		case SCREEN_EDGE_BOTTOM:
			_strut.bottom = _gm.height;
			break;
		case SCREEN_EDGE_LEFT:
			_strut.left = _gm.width;
			break;
		case SCREEN_EDGE_RIGHT:
			_strut.right = _gm.width;
			break;
		case SCREEN_EDGE_NO:
		default:
			// do nothing
			break;
		}
	}
}

/**
 * Edge version of mapped window, makes sure the iconified state is
 * set at all times in order to avoid counting the edge windows when
 * snapping windows etc.
 */
void
EdgeWO::mapWindow(void)
{
	if (_mapped) {
		return;
	}

	PWinObj::mapWindow();
	_iconified = true;
}

/**
 * Enter event handler, gets actions from EdgeList on _edge.
 */
ActionEvent*
EdgeWO::handleEnterEvent(XCrossingEvent *ev)
{
	std::vector<ActionEvent>* el = _cfg->getEdgeListFromPosition(_edge);
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
					      MOUSE_EVENT_ENTER, el);
}

/**
 * Button press event handler, gets actions from EdgeList on _edge.
 */
ActionEvent*
EdgeWO::handleButtonPress(XButtonEvent *ev)
{
	std::vector<ActionEvent>* el = _cfg->getEdgeListFromPosition(_edge);
	return ActionHandler::findMouseAction(ev->button, ev->state,
					      MOUSE_EVENT_PRESS, el);
}

/**
 * Button release event handler, gets actions from EdgeList on _edge.
 */
ActionEvent*
EdgeWO::handleButtonRelease(XButtonEvent *ev)
{
	// Make sure the release is on the actual window. This probably
	// could be done smarter.
	if (ev->x_root < _gm.x
	    || ev->x_root > static_cast<int>(_gm.x + _gm.width)
	    || ev->y_root < _gm.y
	    || ev->y_root > static_cast<int>(_gm.y + _gm.height)) {
		return 0;
	}

	MouseEventType mb = MOUSE_EVENT_RELEASE;

	// first we check if it's a double click
	if (X11::isDoubleClick(ev->window, ev->button - 1, ev->time,
			       _cfg->getDoubleClickTime())) {
		X11::setLastClickID(ev->window);
		X11::setLastClickTime(ev->button - 1, 0);

		mb = MOUSE_EVENT_DOUBLE;
	} else {
		X11::setLastClickID(ev->window);
		X11::setLastClickTime(ev->button - 1, ev->time);
	}

	std::vector<ActionEvent>* el = _cfg->getEdgeListFromPosition(_edge);
	return ActionHandler::findMouseAction(ev->button, ev->state, mb, el);
}

void
RootWO::initStrutHead()
{
	_strut_head.clear();
	for (int i = 0; i < X11::getNumHeads(); i++) {
		_strut_head.push_back(Strut(0, 0, 0, 0, i));
	}
}
