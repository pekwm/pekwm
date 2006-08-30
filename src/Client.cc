//
// Client.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// client.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowObject.hh"
#include "Client.hh"

#include "ScreenInfo.hh"
#include "Config.hh"
#include "KeyGrabber.hh"
#include "Theme.hh"
#include "AutoProperties.hh"
#include "Frame.hh"
#include "FrameWidget.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"

#ifdef MENUS
#include "BaseMenu.hh"
#include "ActionMenu.hh"
#endif // MENUS

extern "C" {
#include <X11/Xatom.h>
#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::vector;
using std::list;

Client::Client(WindowManager *w, Window new_client) :
WindowObject(w->getScreen()->getDisplay()),
_wm(w),
_size(NULL), _transient(None),
_frame(NULL), _strut(NULL),
_class_hint(NULL),
_alive(false), _marked(false),
_titlebar(true), _border(true),
_extended_net_name(false),
#ifdef SHAPE
_shaped(false)
#endif // SHAPE
{
	// Construct the client
	_wm->getScreen()->grabServer();

	// WindowObject attributes
	_window = new_client;

	if (!validate()) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Couldn't validate client." << endl;
#endif // DEBUG
		_wm->getScreen()->ungrabServer(true);
		return;
	}

	XWindowAttributes attr;
	if (!XGetWindowAttributes(_dpy, _window, &attr)) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Client died." << endl;
#endif // DEBUG
		_wm->getScreen()->ungrabServer(true);
		return;
	}
	_gm.x = attr.x;
	_gm.y = attr.y;
	_gm.width = attr.width;
	_gm.height = attr.height;

	_cmap = attr.colormap;
	_size = XAllocSizeHints();

	// get trans window and the window attributes
	getTransientForHint();

	XSetWindowAttributes sattr;
	sattr.event_mask =
		PropertyChangeMask|StructureNotifyMask|FocusChangeMask;
	sattr.do_not_propagate_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;

	// We don't want these masks to be propagated down to the frame
	XChangeWindowAttributes(_dpy, _window, CWEventMask|CWDontPropagate, &sattr);

#ifdef SHAPE
	if (_wm->getScreen()->hasShape())
		XShapeSelectInput(_dpy, _window, ShapeNotifyMask);
#endif // SHAPE

	XAddToSaveSet(_dpy, _window);
	XSetWindowBorderWidth(_dpy, _window, 0);

	// Load the Class hint before loading the autoprops and
	// getting client title as we search for TitleRule in the autoprops
	_class_hint = new ClassHint();

	XClassHint class_hint;
	if (XGetClassHint(_dpy, _window, &class_hint)) {
		_class_hint->h_name = class_hint.res_name;
		_class_hint->h_class = class_hint.res_class;
		XFree(class_hint.res_name);
		XFree(class_hint.res_class);
	}

	getWMNormalHints();
	getXClientName();
	getXIconName();

	readMwmHints(); // read atoms
	readEwmhHints();

	XWMHints* hints;
	if ((hints = XGetWMHints(_dpy, _window))) {
		if (hints->flags&StateHint) {
			setWmState(hints->initial_state);
		} else {
			setWmState(NormalState);
		}
		XFree(hints);
	}

	// Are we iconified
	if (getWmState() == IconicState)
		_iconified = true;

	// Grab keybindings and mousebutton actions
#ifdef KEYS
	_wm->getKeyGrabber()->grabKeys(_window);
#endif // KEYS
	grabButtons();

	// find a frame based on the clients _PEKWM_FRAME_ID, if any
	if (!_frame && !_wm->isStartup()) {
		long id =
			_wm->getAtomLongValue(_window,
													 _wm->getPekwmAtoms()->getAtom(PEKWM_FRAME_ID));
		if (id && (id != -1)) {
			_frame = _wm->findFrameFromId(id);
			if (_frame)
				_frame->insertClient(this);
		}
	}

	// find frame based on tagged info
	if (_wm->isStartup()) {
		bool activate;
		_frame = _wm->findTaggedFrame(activate);
		if (_frame)
			_frame->insertClient(this, activate, activate);
	}

	// We load the autoprops at the end, so that we "overwrite" properties
	// with the autoprops.
	AutoProperty *ap_data;
	if (attr.map_state == IsViewable)
		ap_data = readAutoprops(APPLY_ON_START);
	else
		ap_data = readAutoprops(APPLY_ON_NEW);

	// Moved autogrouping ( Should be moved to FrameClient handler or something
	// in the future ) because of saner focus handling
	bool focus_after_map = false;

	if (!_frame && ap_data && (ap_data->group_size > 1)) {
		if ((_frame = _wm->findGroup(_class_hint, ap_data))) {
			if (ap_data->group_behind) {
				_frame->insertClient(this, false, false);
			} else {
				_frame->insertClient(this);

				// if we have focus on new set to false and we don't group behind
				// we will want to give the new client focus if the frame has focus
				// as otherwise the non active client will have it
				if (_frame->isFocused() &&
						!(_wm->getConfig()->getFocusMask()&FOCUS_NEW))
					focus_after_map = true;
			}
		}
	}

	// Tell the world about our state
	updateWmStates();

	_wm->getScreen()->ungrabServer(true);

	// if we don't have a frame allready, create a new one
	if (!_frame) {
		// a hack for now, the Frame constructor calls setWorkspace which then
		// calls mapWindow and then Client::mapWindow which sets _mapped to true
		// which later on will brake the WindowObject::mapWindow call
		_mapped = true;
		_frame = new Frame(_wm, this);
		_mapped = false;
	}

	// make sure the window is mapped after it has got it's frame.
	WindowObject::mapWindow();

	if (focus_after_map)
		_frame->giveInputFocus();

	// finished creating the client, so now adding it to the client list
	_wm->addToClientList(this);

	_alive = true;
}

Client::~Client()
{
	_wm->removeFromClientList(this);

	_wm->getScreen()->grabServer();

#ifdef MENUS
	ActionMenu* menu = (ActionMenu*) _wm->getMenu(WINDOWMENU_TYPE);
	if (this == menu->getClient())
		menu->setClient(NULL);
#endif // MENUS

	// removes gravity and moves it back to root if we are alive
	bool focus = false;
	if (_frame) {
		focus = _frame->isFocused();
		_frame->removeClient(this);
	}

	// Focus the parent if we had focus before
	if (focus && _transient) {
		Client *parent = _wm->findClientFromWindow(_transient);
		if (parent && parent->getFrame()->isActiveClient(parent))
			parent->getFrame()->giveInputFocus();
	}

	// Clean up if the client still is alive, it'll be dead all times
	// except when we exit pekwm
	if (_alive) {
		XUngrabButton(_dpy, AnyButton, AnyModifier, _window);
#ifdef KEYS
		_wm->getKeyGrabber()->ungrabKeys(_window);
#endif // KEYS
		XRemoveFromSaveSet(_dpy, _window);
	}

	// free names and size hint
	if (_size)
		XFree(_size);

	if (_strut) {
		_wm->getScreen()->removeStrut(_strut);
		_wm->setEwmhWorkArea(); // this needs to be done after removing a strut
		delete _strut;
		_strut = NULL;
	}

	if (_class_hint)
		delete _class_hint;

	_wm->getScreen()->ungrabServer(true);
}

// START - WindowObject interface.

//! @fn    void mapWindow(void)
//! @brief Maps the window.
void
Client::mapWindow(void)
{
	if (_mapped)
		return;
	_mapped = true; // needed as I don't call WindowObject::mapWindow()

	if (_iconified) {
		_iconified = false;
		updateWmStates();
	}

	setWmState(NormalState); // update the state and wm hints

	if(!_transient) // unmap our transient windows if we have any
		_wm->findTransientsToMapOrUnmap(_window, false);

	//	XSelectInput(_dpy, _window, NoEventMask);
	// 	WindowObject::mapWindow();
	//	XSelectInput(_dpy, _window,
	//							 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}


//! @fn    void unmapWindow(void)
//! @brief Sets the client to WithdrawnState and unmaps it.
void
Client::unmapWindow(void)
{
	if (!_mapped)
		return;
	_mapped = false; // needed as I don't call WindowObject::unmapWindow()

	if (_iconified) { // set the state of the window
		setWmState(IconicState);
		updateWmStates();
	} else {
		setWmState(WithdrawnState);
	}

	// TO-DO: This breaks attach* behaviour if the client is on another
	// workspace so I'll comment it for now. Will have to make sure the attach
	// behaviour works before I unbreak hpanel again.
	//	XSelectInput(_dpy, _window, NoEventMask);
	// WindowObject::unmapWindow();
	//	XSelectInput(_dpy, _window,
	//								 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}

//! @fn    void iconify(void)
//! @brief Iconifies the client and adds it to the iconmenu
void
Client::iconify(void)
{
	if (_iconified)
		return;
	_iconified = true;

	if (!_transient)
		_wm->findTransientsToMapOrUnmap(_window, true);

	unmapWindow();
}

//! @fn    void move(int x, int y)
//! @brief Update the position variables.
void
Client::move(int x, int y)
{
	bool request = false;
	if ((_gm.x != x) || (_gm.y != y))
		request = true;

	_gm.x = x;
	_gm.y = y;

	if (request)
		sendConfigureRequest();
}

//! @fn    void resize(unsigned int w, unsigned int h)
//! @breif Resizes the client window to specified size.
void
Client::resize(unsigned int width, unsigned int height)
{
	bool request = false;
	if ((_gm.width != width) || (_gm.height != height))
		request = true;

	WindowObject::resize(width, height);

	if (request)
		sendConfigureRequest();
}

//! @fn    void setWorkspace(unsigned int workspace)
//! @brief Sets the workspce and updates the _NET_WM_DESKTOP hint.
void
Client::setWorkspace(unsigned int workspace)
{
	if (workspace >= _wm->getWorkspaces()->getNumber())
		return;
	_workspace = workspace;

	// set the desktop we're on, if we are sticky we are on _all_ desktops
	if (_sticky) {
		_wm->setAtomLongValue(_window, _wm->getEwmhAtoms()->getAtom(NET_WM_DESKTOP),
												 NET_WM_STICKY_WINDOW);
	} else {
		_wm->setAtomLongValue(_window, _wm->getEwmhAtoms()->getAtom(NET_WM_DESKTOP),
												 _workspace);
	}
}

// END - WindowObject interface.

//! @fn    bool validate(void)
//! @brief Checks if the window has any Destroy or Unmap notifys.
bool
Client::validate(void)
{
	XSync(_dpy, false);

	XEvent ev;
	if (XCheckTypedWindowEvent(_dpy, _window, DestroyNotify, &ev) ||
			XCheckTypedWindowEvent(_dpy, _window, UnmapNotify, &ev)) {
		XPutBackEvent(_dpy, &ev);
		return false;
	}

	return true;
}

//! @fn    void getGeometry(Geometry &gm)
//! @brief Sets the Geometry on gm.
//! @return Returns true if the window is mapped, else false.
bool
Client::getGeometry(Geometry &gm)
{
	XWindowAttributes attr;
	XGetWindowAttributes(_dpy, _window, &attr);

	gm = _gm;

	if (attr.map_state == IsViewable)
		return true;
	return false;
}

//! @fn    void grabButtons(void)
//! @brief Grabs all the mouse button actions on the client.
void
Client::grabButtons(void)
{
	// Make sure we don't have any buttons grabbed.
	XUngrabButton(_dpy, AnyButton, AnyModifier, _window);

	list<ActionEvent> *actions = _wm->getConfig()->getMouseClientList();
	list<ActionEvent>::iterator it = actions->begin();
	for (; it != actions->end(); ++it) {
		if ((it->type == BUTTON_PRESS) || (it->type == BUTTON_RELEASE)) {
			// No need to grab mod less events, replied with the frame
			if (!it->mod)
				continue;

			grabButton(it->sym, it->mod,
								 ButtonPressMask|ButtonReleaseMask,
								 _window, None);
		} else if (it->type == BUTTON_MOTION) {
			grabButton(it->sym, it->mod,
								 ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
								 _window, None);
		}
	}
}

//! @fn    void readEwmhHints(void)
//! @brief Loads the Clients state from EWMH atoms.
void
Client::readEwmhHints(void)
{
	EwmhAtoms *ewmh = _wm->getEwmhAtoms(); // convenience

	// which workspace do we belong to?
	int workspace = _wm->getAtomLongValue(_window, ewmh->getAtom(NET_WM_DESKTOP));
	if (workspace < 0) {
		_workspace = _wm->getWorkspaces()->getActive();
		_wm->setAtomLongValue(_window, ewmh->getAtom(NET_WM_DESKTOP), _workspace);
	} else
		_workspace = workspace;

 // do we want a strut?
	getStrutHint();

	// try to figure out what kind of window we are and alter it acordingly
	int items;
	Atom *atoms = NULL;

	bool found_window_type = false;
	atoms = (Atom*) _wm->getEwmhPropertyData(_window, ewmh->getAtom(WINDOW_TYPE),
																					XA_ATOM, &items);
	if (atoms) {
		for (int i = 0; i < items; ++i) {
			found_window_type = true;

			if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_DESKTOP)) {
				// desktop windows, make it the same size as the screen and place
				// it below all windows withouth any decorations, also make it sticky
				_gm.x = _gm.y = 0;
				_gm.width = _wm->getScreen()->getWidth();
				_gm.height = _wm->getScreen()->getHeight();
				_titlebar = false;
				_border = false;
				_sticky = true;
				_state.placed = true;
				_state.skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_FRAME_SNAP;
				_layer = LAYER_DESKTOP;
				break;
			} else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_DOCK)) {
				_titlebar = false;
				_border = false;
				_sticky = true;
				_layer = LAYER_DOCK;
				break;
			} else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_TOOLBAR)) {
				_titlebar = false;
				_border = false;
				_state.skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE;
				break;
			} else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_MENU)) {
				_layer = LAYER_MENU;
				_state.skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_FRAME_SNAP;
				break;
			} else {
				found_window_type = false;
			}
		}

		XFree(atoms);
	}

	if (!found_window_type) {
		Atom type = ewmh->getAtom(WINDOW_TYPE_NORMAL);
		XChangeProperty(_dpy, _window, ewmh->getAtom(WINDOW_TYPE), XA_ATOM, 32,
										PropModeReplace, (unsigned char *) &type, 1);
	}

	// The _NET_WM_STATE overides the _NET_WM_TYPE
	NetWMStates win_states;
	_wm->getEwmhStates(_window, win_states);

	if (win_states.hidden) _iconified = true;
	if (win_states.shaded) _state.shaded = true;
	if (win_states.max_vert) _state.maximized_vert = true;
	if (win_states.max_horz) _state.maximized_horz = true;
	if (win_states.skip_taskbar) _state.skip_taskbar = true;
	if (win_states.skip_pager) _state.skip_pager = true;
	if (win_states.sticky) _sticky = true;
	if (win_states.above) _layer = LAYER_BELOW;
	if (win_states.below) _layer = LAYER_ABOVE_DOCK;

	// TO-DO: Add support for net_wm_state_fullscreen
}

//! @fn    void readMwmHints(void)
//! @brief Loads the Clients state from MWM atoms.
void
Client::readMwmHints(void)
{
	MwmHints *mwm_hints = getMwmHints(_window);

	if (!mwm_hints)
		return;

	if (mwm_hints->flags&MWM_HINTS_FUNCTIONS) {
    if (!mwm_hints->functions&MWM_FUNC_RESIZE)
      _actions.resize = false;
    if (!mwm_hints->functions&MWM_FUNC_MOVE)
      _actions.move = false;
    if (!mwm_hints->functions&MWM_FUNC_ICONIFY)
      _actions.minimize = false;
    if (!mwm_hints->functions&MWM_FUNC_CLOSE)
      _actions.close = false;
		if (!(mwm_hints->functions&MWM_FUNC_MAXIMIZE)) {
			_actions.maximize_vert = false;
			_actions.maximize_horz = false;
		}
	}

	if (mwm_hints->flags&MWM_HINTS_DECORATIONS) {
		if (!mwm_hints->decorations&MWM_DECOR_ALL) {
			_titlebar = mwm_hints->decorations&MWM_DECOR_TITLE;
			_border = mwm_hints->decorations&MWM_DECOR_BORDER;
		}
	}

	XFree(mwm_hints);
}

//! @fn    void readAutoprops(int type)
//! @brief Tries to find a AutoProp for the current client.
//! @param type Defaults to 0
AutoProperty*
Client::readAutoprops(unsigned int type)
{
	_class_hint->title = _name;
	AutoProperty *data =
		_wm->getAutoProperties()->findAutoProperty(_class_hint, _workspace, type);
	_class_hint->title = "";

	if (!data || (_transient
			? !data->isApplyOn(APPLY_ON_TRANSIENT|APPLY_ON_TRANSIENT_ONLY)
			: data->isApplyOn(APPLY_ON_TRANSIENT_ONLY)))
		return data;

	// Set the correct group of the window
	_class_hint->group = data->group_name;

	// We only apply grouping if it's a new client or if we are restarting
	// and have APPLY_ON_START set
	if (data->isMask(AP_STICKY))
		_sticky = data->sticky;
	if (data->isMask(AP_SHADED))
		_state.shaded = data->shaded;
	if (data->isMask(AP_MAXIMIZED_VERTICAL))
		_state.maximized_vert = data->maximized_vertical;
	if (data->isMask(AP_MAXIMIZED_HORIZONTAL))
		_state.maximized_horz = data->maximized_horizontal;
	if (data->isMask(AP_ICONIFIED))
		_iconified = data->iconified;
	if (data->isMask(AP_BORDER))
		_border = data->border;
	if (data->isMask(AP_TITLEBAR))
		_titlebar = data->titlebar;
	if (data->isMask(AP_LAYER) && (data->layer <= LAYER_MENU))
		_layer = data->layer;
	if (data->isMask(AP_SKIP))
		_state.skip = data->skip;

	if (data->isMask(AP_GEOMETRY)) {
		// Read size before position so negative position works
		if (data->gm_mask&WidthValue)
			_gm.width = data->gm.width;
		if (data->gm_mask&HeightValue)
			_gm.height = data->gm.height;
		// Read position
		if (data->gm_mask&(XValue|YValue)) {
			if (data->gm_mask&XValue) {
				_gm.x = data->gm.x;
				if (data->gm_mask&XNegative)
					_gm.x += _wm->getScreen()->getWidth() - _gm.width;
			}
			if (data->gm_mask&YValue) {
				_gm.y = data->gm.y;
				if (data->gm_mask&YNegative)
					_gm.y += _wm->getScreen()->getHeight() - _gm.height;
			}

			_state.placed = true;
		}
	}

	if (data->isMask(AP_WORKSPACE)) {
		if (data->workspace < _wm->getWorkspaces()->getNumber()) {
			// no need to do anything about this here, it'll be set later
			// in constructClient
			_workspace = data->workspace;
		}
	}

	return data;
}

//! @fn    void getXClientName(void)
//! @brief Tries to get the NET_WM name, else fall back to XA_WM_NAME
void
Client::getXClientName(void)
{
	_name = "";

	XTextProperty text_property;
	if (!XGetWMName(_dpy, _window, &text_property) ||
			!text_property.value || !text_property.nitems)
		return;

	if (text_property.encoding == XA_STRING) {
		_name = (const char*) text_property.value;
	} else {
		char **list;
		int num;

		XmbTextPropertyToTextList(_dpy, &text_property, &list, &num);
		if (num && *list) {
			_name = *list;
			XFreeStringList(list);
		}
	}

	// Search for a TitleRule
	_class_hint->title = _name;
	TitleProperty *data =
		_wm->getAutoProperties()->findTitleProperty(_class_hint);
	_class_hint->title = "";

	if (data)
		data->getTitleRule().replace(_name);

	XFree (text_property.value);
}

//! @fn    void getXIconName(void)
//! @brief Tries to get Client's icon name, and puts it in _icon_name.
void
Client::getXIconName(void)
{
	_icon_name = "";

	XTextProperty text_property;
	if (!XGetWMIconName(_dpy, _window, &text_property) ||
			!text_property.value || !text_property.nitems)
		return;

	if (text_property.encoding == XA_STRING) {
		_icon_name = (const char*) text_property.value;
	} else {
		char **list;
		int num;

		XmbTextPropertyToTextList(_dpy, &text_property, &list, &num);
		if (num && *list) {
			_icon_name = *list;
			XFreeStringList(list);
		}
	}

	XFree (text_property.value);
}

//! @fn    void setWmState(unsigned long state)
//! @brief Sets the WM_STATE of the client to state
//! @param state State to set.
void
Client::setWmState(unsigned long state)
{
	unsigned long data[2];

	data[0] = state;
	data[1] = None; // No Icon

	XChangeProperty(_dpy, _window,
									_wm->getIcccmAtoms()->getAtom(WM_STATE),
									_wm->getIcccmAtoms()->getAtom(WM_STATE),
									32, PropModeReplace, (unsigned char *) data, 2);
}

// If we can't find a _wm->wm_state we're going to have to assume
// Withdrawn. This is not exactly optimal, since we can't really
// distinguish between the case where no WM has run yet and when the
// state was explicitly removed (Clients are allowed to either set the
// atom to Withdrawn or just remove it... yuck.)
long
Client::getWmState(void)
{
	Atom real_type;
	int real_format;
	long *data, state = WithdrawnState;
	unsigned long items_read, items_left;

	int status =
		XGetWindowProperty(_dpy, _window, _wm->getIcccmAtoms()->getAtom(WM_STATE),
											 0L, 2L, False, _wm->getIcccmAtoms()->getAtom(WM_STATE),
											 &real_type, &real_format, &items_read, &items_left,
											 (unsigned char **) &data);
	if ((status  == Success) && items_read) {
		state = *data;
		XFree(data);
	}

	return state;
}

//! @fn    void sendConfigureRequest(void)
//! @brief Send XConfigureEvent, letting the client know about changes
void
Client::sendConfigureRequest(void)
{
	XConfigureEvent e;

	e.type = ConfigureNotify;
	e.event = e.window = _window;
	e.x = _gm.x;
	e.y = _gm.y;
	e.width = _gm.width;
	e.height = _gm.height;
	e.border_width = 0;
	e.above = Below;
	e.override_redirect = false;

	XSendEvent(_dpy, _window, false, StructureNotifyMask, (XEvent *) &e);
}

//! @fn void grabButton(int button, int mod, int mask, Window win, Cursor curs)
//! @brief Grabs a button on the window win
//! Grabs the button button, with the mod mod and mask mask on the window win
//! and cursor curs with "all" possible extra modifiers
void
Client::grabButton(int button, int mod, int mask, Window win, Cursor curs)
{
	unsigned int num_lock = _wm->getScreen()->getNumLock();
	unsigned int scroll_lock = _wm->getScreen()->getScrollLock();

	XGrabButton(_dpy, button, mod,
							win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	XGrabButton(_dpy, button, mod|LockMask,
							win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);

	if (num_lock) {
		XGrabButton(_dpy, button, mod|num_lock,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
		XGrabButton(_dpy, button, mod|num_lock|LockMask,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	}
	if (scroll_lock) {
		XGrabButton(_dpy, button, mod|scroll_lock,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
		XGrabButton(_dpy, button, mod|scroll_lock|LockMask,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	}
	if (num_lock && scroll_lock) {
		XGrabButton(_dpy, button, mod|num_lock|scroll_lock,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
		XGrabButton(_dpy, button, mod|num_lock|scroll_lock|LockMask,
								win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
	}
}

//! @fn    void stick(void)
//! @breif Toggle client sticky state
void
Client::stick(void) // TO-DO: WindowObject setSticky() ?
{
	_sticky = !_sticky;
	updateWmStates();
}

//! @fn    void alwaysOnTop(bool top)
//! @breif Toggles the clients always on top state
void
Client::alwaysOnTop(bool top)
{
	if (top)
		_layer = LAYER_ONTOP;
	else
		_layer = LAYER_NORMAL;

	updateWmStates();
}

//! @fn    void alwaysBelow(bool below)
//! @brief Toggles the clients always below state.
void
Client::alwaysBelow(bool below)
{
	if (below)
		_layer = LAYER_BELOW;
	else
		_layer = LAYER_NORMAL;

	updateWmStates();
}

//! @fn    void close(void)
//! @brief Sends an WM_DELETE message to the client, else kills it.
void
Client::close(void)
{
	int count;
	bool found = false;
	Atom *protocols;

	if (XGetWMProtocols(_dpy, _window, &protocols, &count)) {
		for (int i = 0; i < count; ++i) {
			if (protocols[i] == _wm->getIcccmAtoms()->getAtom(WM_DELETE_WINDOW)) {
				found = true;
				break;
			}
		}

		XFree(protocols);
	}

	if (found)
		sendXMessage(_window, _wm->getIcccmAtoms()->getAtom(WM_PROTOCOLS),
								 NoEventMask, _wm->getIcccmAtoms()->getAtom(WM_DELETE_WINDOW));
	else
		kill();
}

//! @fn    void kill(void)
//! @breif Kills the client using XKillClient
void
Client::kill(void)
{
	XKillClient(_dpy, _window);
}

//! @fn    bool setPUPosition(Geometry &gm)
//! @brief Sets the positoin based on P or U position.
//! @return Returns true on success, else false.
bool
Client::setPUPosition(Geometry &gm)
{
	if ((_size->flags&PPosition) || (_size->flags&USPosition)) {
		if (!gm.x)
			gm.x = _size->x;
		if (!gm.y)
			gm.y = _size->y;
		return true;
	}
	return false;
}

//! @fn    bool getIncSize(unsigned int *r_w, unsigned int *r_h, unsigned int w, unsigned int h)
//! @brief Get the size closest to the ResizeInc incremeter
//! @param r_w Pointer to put the new width in
//! @param r_h Pointer to put the new height in
//! @param w Width to calculate from
//! @param h Height to calculate from
bool
Client::getIncSize(unsigned int *r_w, unsigned int *r_h,
									 unsigned int w, unsigned int h)
{
	int basex, basey;

	if (_size->flags&PResizeInc) {
		basex = (_size->flags&PBaseSize)
			? _size->base_width
			: (_size->flags&PMinSize) ? _size->min_width : 0;

		basey = (_size->flags&PBaseSize)
			? _size->base_height
			: (_size->flags&PMinSize) ? _size->min_height : 0;

		*r_w = w - ((w - basex) % _size->width_inc);
		*r_h = h - ((h - basey) % _size->height_inc);

		return true;
	}

	return false;
}

//! @fn    MwmHints* getMwmHints(Window win)
//! @brief Gets a MwmHint structure from a window. Doesn't free memory.
Client::MwmHints*
Client::getMwmHints(Window win)
{
	Atom real_type; int real_format;
	unsigned long items_read, items_left;
	MwmHints *data;

	int status =
		XGetWindowProperty(_dpy, win, _wm->getMwmHintsAtom(),
											 0L, 20L, False, _wm->getMwmHintsAtom(),
											 &real_type, &real_format,
											 &items_read, &items_left,
											 (unsigned char **) &data);

	if ((status == Success) && (items_read >= MWM_HINT_ELEMENTS))
		return data;
	else
		return NULL;
}

//! @fn    void handleMapRequest(XMapRequestEvent *e)
//! @brief Handles map request
void
Client::handleMapRequest(XMapRequestEvent *e)
{
#ifdef DEBUG
	cerr << __FILE__ << "@" << __LINE__ << ": "
			 << "handleMapRequest(" << this << ")" << endl;
#endif // DEBUG

	if (!_sticky && (_workspace != _wm->getWorkspaces()->getActive())) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Ignoring MapRequest, not on current workspace!" << endl;
#endif // DEBUG
		return;
	}

	// TO-DO: FIX THIS FRAME
	if (_frame->isActiveClient(this))
		_frame->mapWindow();
}

//! @fn    void handleUnmapEvent(XUnmapEvent *e)
//! @brief
//! @param e XUnmapEvent to handle, this isn't used though
//! @bug When the client (only transients it seems) won't unmap if they aren't on the same workspace as the active one.
void
Client::handleUnmapEvent(XUnmapEvent *e)
{
	if (e->window != e->event)
		return;

#ifdef DEBUG
			cerr << __FILE__ << "@" << __LINE__ << ": "
					 << "Unmapping client: " << this << endl;
#endif // DEBUG

	//	_is_alive = false; // TO-DO: How to make this work?
	delete this;
}

// This happens when a window is iconified and destroys itself. An
// Unmap event wouldn't happen in that case because the window is
// already unmapped.
void
Client::handleDestroyEvent(XDestroyWindowEvent *e)
{
	_alive = false;
	delete this;
}

void
Client::handleColormapChange(XColormapEvent *e)
{
	if (e->c_new) {
		_cmap = e->colormap;
		XInstallColormap(_dpy, _cmap);
	}
}

int
Client::sendXMessage(Window window, Atom atom, long mask, long value)
{
	XEvent e;

	e.type = e.xclient.type = ClientMessage;
	e.xclient.display = _dpy;
	e.xclient.message_type = atom;
	e.xclient.window = window;
	e.xclient.format = 32;
	e.xclient.data.l[0] = value;
	e.xclient.data.l[1] = CurrentTime;
	e.xclient.data.l[2] = e.xclient.data.l[3] = e.xclient.data.l[4] = 0l;

	return XSendEvent(_dpy, window, false, mask, &e);
}

//! @fn    void updateWmStates(void)
//! @brief Tells the world about our states, such as shaded etc.
void
Client::updateWmStates(void)
{
	EwmhAtoms *ewmh = _wm->getEwmhAtoms();

	Atom wm_states[NET_WM_MAX_STATES];
	for (unsigned int i = 0; i < NET_WM_MAX_STATES; ++i)
		wm_states[i] = None;

	if (false) // we don't yet support modal state
		wm_states[0] = ewmh->getAtom(STATE_MODAL);
	if (_sticky)
		wm_states[1] = ewmh->getAtom(STATE_STICKY);
	if (_state.maximized_vert)
		wm_states[2] = ewmh->getAtom(STATE_MAXIMIZED_VERT);
	if (_state.maximized_horz)
		wm_states[3] = ewmh->getAtom(STATE_MAXIMIZED_HORZ);
	if (_state.shaded)
		wm_states[4] = ewmh->getAtom(STATE_SHADED);
	if (_state.skip_taskbar)
		wm_states[5] = ewmh->getAtom(STATE_SKIP_TASKBAR);
	if (_state.skip_pager)
		wm_states[6] = ewmh->getAtom(STATE_SKIP_PAGER);
	if (_iconified)
		wm_states[7] = ewmh->getAtom(STATE_HIDDEN);
	if (false) // we don't yet support fullscreen state
		wm_states[8] = ewmh->getAtom(STATE_FULLSCREEN);
	if (_layer == LAYER_ABOVE_DOCK)
		wm_states[9] = ewmh->getAtom(STATE_ABOVE);
	if (_layer == LAYER_BELOW)
		wm_states[10] = ewmh->getAtom(STATE_BELOW);

	XChangeProperty(_dpy, _window, ewmh->getAtom(STATE), XA_ATOM,
									32, PropModeReplace, (unsigned char *) wm_states,
									NET_WM_MAX_STATES);
}

//! @fn    void getWMNormalHints(void)
//! @brief
void
Client::getWMNormalHints(void)
{
	long dummy;
	XGetWMNormalHints(_dpy, _window, _size, &dummy);
}

//! @fn    void getTransientForHint(void)
//! @brief
void
Client::getTransientForHint(void)
{
	if (!_transient)
		XGetTransientForHint(_dpy, _window, &_transient);
}

//! @fn    void getStrutHint(void)
//! @brief
void
Client::getStrutHint(void)
{
	int num=0;
	CARD32 *strut = NULL;
	strut = (CARD32*)
		_wm->getEwmhPropertyData(_window,
														 _wm->getEwmhAtoms()->getAtom(NET_WM_STRUT),
														 XA_CARDINAL, &num);

	if (_strut) {
		_wm->getScreen()->removeStrut(_strut);
		delete _strut;
		_strut = NULL;
	}

	if (strut) {
		if (!_strut)
			_strut = new Strut();

		*_strut = strut;
		_wm->getScreen()->addStrut(_strut);

		XFree(strut);
	}

	_wm->setEwmhWorkArea(); // this needs to be done after removing a strut
}

//! @fn    void filteredReparent(Window parent, int x, int y);
//! @brief Reparents the client withouth causing unmap events
//! @param parent Window to reparent the client to
//! @param x X offset
//! @param y Y offset
void
Client::filteredReparent(Window parent, int x, int y)
{
	XSelectInput(_dpy, _window, NoEventMask);
	XReparentWindow(_dpy, _window, parent, x, y);
	XSelectInput(_dpy, _window,
							 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}
