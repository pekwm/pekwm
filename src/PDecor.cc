//
// PDecor.cc for pekwm
// Copyright (C) 2004-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "Config.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "PScreen.hh"
#include "PTexture.hh"
#include "PTexturePlain.hh" // PTextureSolid
#include "ActionHandler.hh"
#include "ScreenResources.hh"
#include "StatusWindow.hh"
#include "KeyGrabber.hh"
#include "Theme.hh"
#include "PixmapHandler.hh"
#include "Workspaces.hh"
#include "Viewport.hh"

#include <functional>
#include <cstdio>
#include <cstdlib>

extern "C" {
#ifdef HAVE_SHAPE
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif // HAVE_SHAPE
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::find;
using std::list;
using std::string;
using std::mem_fun;

// PDecor::Button

//! @brief PDecor::Button constructor
PDecor::Button::Button(Display *dpy, PWinObj *parent, Theme::PDecorButtonData *data) : PWinObj(dpy),
_data(data), _state(BUTTON_STATE_UNFOCUSED),
_left(_data->isLeft())
{
	_parent = parent;

	_gm.width = _data->getWidth();
	_gm.height = _data->getHeight();

	XSetWindowAttributes attr;
	attr.override_redirect = True;
	_window =
		XCreateWindow(_dpy, _parent->getWindow(),
									-_gm.width, -_gm.height, _gm.width, _gm.height, 0,
									CopyFromParent, InputOutput, CopyFromParent,
									CWOverrideRedirect, &attr);

	_bg = ScreenResources::instance()->getPixmapHandler()->getPixmap(_gm.width, _gm.height, PScreen::instance()->getDepth(), true);

	setBackgroundPixmap(_bg);
	setState(_state);
}

//! @brief PDecor::Button destructor
PDecor::Button::~Button(void)
{
	XDestroyWindow(_dpy, _window);
	ScreenResources::instance()->getPixmapHandler()->returnPixmap(_bg);
}

//! @brief Searches the PDecorButtonData for an action matching ev
ActionEvent*
PDecor::Button::findAction(XButtonEvent *ev)
{
	list<ActionEvent>::iterator it(_data->begin());
	for (; it != _data->end(); ++it) {
		if (it->mod == ev->state && it->sym == ev->button)
			return &*it;
	}
	return NULL;
}

//! @brief Sets the state of the button
void
PDecor::Button::setState(ButtonState state)
{
	if (state == BUTTON_STATE_NO) {
		return;
	}

	// only update if we don't hoover, we want to be able to turn back
	if (state != BUTTON_STATE_HOOVER) {
		_state = state;
	}

	_data->getTexture(state)->render(_bg, 0, 0, _gm.width, _gm.height);
	clear();
}

// PDecor

const string PDecor::DEFAULT_DECOR_NAME = string("DEFAULT");
const string PDecor::DEFAULT_DECOR_NAME_BORDERLESS = string("BORDERLESS");
const string PDecor::DEFAULT_DECOR_NAME_TITLEBARLESS = string("TITLEBARLESS");

//! @brief PDecor constructor
//! @param dpy Display
//! @param theme Theme
//! @param decor_name String, if not DEFAULT_DECOR_NAME sets _decor_name_override
PDecor::PDecor(Display *dpy, Theme *theme, const std::string decor_name) : PWinObj(dpy),
_theme(theme), _child(NULL), _button(NULL),
_pointer_x(0), _pointer_y(0),
_click_x(0), _click_y(0),
_decor_cfg_keep_empty(false), _decor_cfg_child_move_overloaded(false),
_decor_cfg_bpr_replay_pointer(false),
_decor_cfg_bpr_al_child(MOUSE_ACTION_LIST_CHILD_OTHER),
_decor_cfg_bpr_al_title(MOUSE_ACTION_LIST_TITLE_OTHER),
_data(NULL), _decor_name(decor_name),
_border(true), _titlebar(true), _shaded(false),
_need_shape(false), _need_client_shape(false), _real_height(1),
_title_wo(dpy), _title_bg(None),
_title_active(0), _titles_left(0), _titles_right(1)
{
	if (_decor_name != PDecor::DEFAULT_DECOR_NAME) {
		_decor_name_override = _decor_name;
	}

	// we be reset in loadDecor later on, inlines using the _data used before
	// loadDecor needs this though
	_data = _theme->getPDecorData(_decor_name);
	if (_data == NULL) {
		_data = _theme->getPDecorData(DEFAULT_DECOR_NAME);
	}

	XSetWindowAttributes attr;
	attr.override_redirect = True;
	attr.event_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		EnterWindowMask|SubstructureRedirectMask|SubstructureNotifyMask;
	_window =
		XCreateWindow(_dpy, PScreen::instance()->getRoot(),
									_gm.x, _gm.y, _gm.width, _gm.height, 0,
									CopyFromParent, InputOutput, CopyFromParent,
									CWOverrideRedirect|CWEventMask, &attr);

	attr.override_redirect = True;
	attr.event_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask;
	_title_wo.setWindow(XCreateWindow(_dpy, _window,
									borderLeft(), borderTop(), 1, 1, 0, // don't know our size yet
									CopyFromParent, InputOutput, CopyFromParent,
									CWOverrideRedirect|CWEventMask, &attr));

  // create border windows
  for (uint i = 0; i < BORDER_NO_POS; ++i) {
    attr.cursor =
			ScreenResources::instance()->getCursor(ScreenResources::CursorType(i));

    _border_win[i] =
      XCreateWindow(_dpy, _window,
										-1, -1, 1, 1, 0,
										CopyFromParent, InputOutput, CopyFromParent,
										CWEventMask|CWCursor, &attr);
  }

	// sets buttons etc up
	loadDecor();

	// map title and border windows
	XMapSubwindows(_dpy, _window);
}

//! @brief PDecor destructor
PDecor::~PDecor(void)
{
	if (_child_list.size() > 0) {
		while (_child_list.size() != 0) {
			removeChild(_child_list.back(), false); // Don't call delete this.
		}
	}

	// make things look smoother, buttons will be noticed as deleted otherwise
	unmapWindow();

	// free buttons
	unloadDecor();

	// free border pixmaps
	list<Pixmap>::iterator it(_border_pix_list.begin());
	for (; it != _border_pix_list.end(); ++it) {
		ScreenResources::instance()->getPixmapHandler()->returnPixmap(*it);
	}
	ScreenResources::instance()->getPixmapHandler()->returnPixmap(_title_bg);

	XDestroyWindow(_dpy, _title_wo.getWindow());
	for (uint i = 0; i < BORDER_NO_POS; ++i) {
		XDestroyWindow(_dpy, _border_win[i]);
	}
	XDestroyWindow(_dpy, _window);
}

// START - PWinObj interface.

//! @brief Map decor and all children
void
PDecor::mapWindow(void)
{
	if (_mapped != true) {
		PWinObj::mapWindow();
		for_each(_child_list.begin(), _child_list.end(),
						 mem_fun(&PWinObj::mapWindow));
	}
}

//! @brief Maps the window raised
void
PDecor::mapWindowRaised(void)
{
	if (_mapped == true) {
		return;
	}
	mapWindow();

	raise(); // XMapRaised wouldn't preserver layers
}

//! @brief Unmaps decor and all children
void
PDecor::unmapWindow(void)
{
	if (_mapped == true) {
		if (_iconified == true) {
			for_each(_child_list.begin(), _child_list.end(),
							 mem_fun(&PWinObj::iconify));
		} else {
			for_each(_child_list.begin(), _child_list.end(),
							 mem_fun(&PWinObj::unmapWindow));
		}
		PWinObj::unmapWindow();
	}
}

//! @brief Moves the decor
void
PDecor::move(int x, int y, bool do_virtual)
{
	// update real position
	PWinObj::move(x, y);
	if ((_child != NULL) && (_decor_cfg_child_move_overloaded)) {
		_child->move(x + borderLeft(), y + borderTop() + getTitleHeight());
	}

	// update virtual position
	if (do_virtual == true) {
		moveVirtual(x + Workspaces::instance()->getActiveViewport()->getX(),
								y + Workspaces::instance()->getActiveViewport()->getY());
	}
}

//! @brief Virtually moves the decor, and all children if any
void
PDecor::moveVirtual(int x, int y)
{
	PWinObj::moveVirtual(x, y);
	list<PWinObj*>::iterator it(_child_list.begin());
	for (; it != _child_list.end(); ++it) {
		(*it)->moveVirtual(x + borderLeft(), y + borderTop() + getTitleHeight());
	}
}

//! @brief Resizes the decor, and active child if any
void
PDecor::resize(uint width, uint height)
{
	// if shaded, don't resize to the size specified just update width
	// and set _real_height to height
	if (_shaded) {
		_real_height = height;

		height = getTitleHeight();
		// shading in non full width title mode will make border go away
		if (!_data->getTitleWidthMin())
			height += borderTop() + borderBottom();
	}

	PWinObj::resize(width, height);

	// do before placing and shaping the rest as shaping depends on the
	// child window
	if (_child != NULL) {
      _child->resize(getChildWidth(), getChildHeight());
	}

	// place / resize title and border
	resizeTitle();
	placeBorder();

	// render title and border
	renderTitle();
	renderBorder();
}

//! @brief
void
PDecor::resizeTitle(void)
{
	if (getTitleHeight()) {
		_title_wo.resize(calcTitleWidth(), getTitleHeight());
		calcTabsWidth();		
	}

	// place buttons, also updates title information
	placeButtons();	
}

//! @brief Raises the window, taking _layer into account
void
PDecor::raise(void)
{
	Workspaces::instance()->raise(this);
	Workspaces::instance()->updateClientStackingList(false, true);
}

//! @brief Lowers the window, taking _layer into account
void
PDecor::lower(void)
{
	Workspaces::instance()->lower(this);
	Workspaces::instance()->updateClientStackingList(false, true);
}

//! @brief
void
PDecor::setFocused(bool focused)
{
	if (_focused != focused) { // save repaints
		PWinObj::setFocused(focused);

		renderTitle();
		renderButtons();

		renderBorder();
		setBorderShape();
		applyBorderShape();
	}
}

//! @brief
void
PDecor::setWorkspace(uint workspace)
{
  if (workspace != NET_WM_STICKY_WINDOW)
    {
      if (workspace >= Workspaces::instance ()->size ())
        {
#ifdef DEBUG
          cerr << __FILE__ << "@" << __LINE__ << ": "
               << "PDecor(" << this << ")::setWorkspace(" << workspace << ")"
               << endl << " *** workspace > number of workspaces:"
               << Workspaces::instance ()->size () << endl;
#endif // DEBUG
          workspace = Workspaces::instance()->size() - 1;
        }
      _workspace = workspace;
    }

  list<PWinObj*>::iterator it (_child_list.begin ());
  for (; it != _child_list.end (); ++it)
    (*it)->setWorkspace (workspace);

  if (!_mapped && !_iconified)
    {
      if (_sticky || (_workspace == Workspaces::instance ()->getActive ()))
        mapWindow();
    }
  else if (!_sticky && (_workspace != Workspaces::instance ()->getActive ()))
    unmapWindow();
}

//! @brief Gives decor input focus, fails if not mapped or not visible
bool
PDecor::giveInputFocus(void)
{
	if ((_mapped == true) && (_child != NULL) &&
			(Workspaces::instance()->getActiveViewport()->isInside(this))) {
		return _child->giveInputFocus();

	} else {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PDecor::giveInputFocus(" << this << ")" << endl
				 << " *** reverting to root" << endl;
#endif // DEBUG
		PWinObj::getRootPWinObj()->giveInputFocus();
		return false;
	}
}

//! @brief
ActionEvent*
PDecor::handleButtonPress(XButtonEvent *ev)
{
	ActionEvent *ae = NULL;
	MouseEventType mb = MOUSE_EVENT_PRESS;

	ev->state &= ~Button1Mask & ~Button2Mask & ~Button3Mask
		& ~Button4Mask & ~Button5Mask;
	ev->state &= ~PScreen::instance()->getNumLock() &
							 ~PScreen::instance()->getScrollLock() & ~LockMask;

	// try to do something about frame bunttons
	if (ev->subwindow != None)
		_button = findButton(ev->subwindow);

	if (_button) {
		_button->setState(BUTTON_STATE_PRESSED);

		ae = _button->findAction(ev);

		// if the button is used for resizing, we don't want to wait for release
		if ((ae != NULL) && ae->isOnlyAction(ACTION_RESIZE)) {
			_button->setState(_focused ? BUTTON_STATE_FOCUSED : BUTTON_STATE_UNFOCUSED);
			_button = NULL;
		} else {
			ae = NULL;
		}

	} else {
		// used in the motion handler when doing a window move.
		_click_x = _gm.x;
		_click_y = _gm.y;
		_pointer_x = ev->x_root;
		_pointer_y = ev->y_root;

		// allow us to get clicks from anywhere on the window.
		if (_decor_cfg_bpr_replay_pointer == true) {
			XAllowEvents(_dpy, ReplayPointer, CurrentTime);
		}

		// clicks on the child window
		if ((ev->window == _child->getWindow()) ||
				((ev->state == 0) && (ev->subwindow == _child->getWindow()))) {

			// NOTE: If the we're matching against the subwindow we need to make
			// sure that the state is 0, meaning we didn't have any modifier
			// because if we don't care about the modifier we'll get two actions
			// performed when using modifiers.

			ae = ActionHandler::findMouseAction(ev->button, ev->state, mb,
																					Config::instance()->getMouseActionList(_decor_cfg_bpr_al_child));

		// clicks on the decor title
		} else if (ev->window == getTitleWindow()) {

			ae = ActionHandler::findMouseAction(ev->button, ev->state, mb,
																					Config::instance()->getMouseActionList(_decor_cfg_bpr_al_title));

		// clicks on the decor border
		} else if ((ev->subwindow != None) && (_title_wo != ev->subwindow) &&
							 (ev->window == _window)) {
			uint pos = getBorderPosition(ev->subwindow);
			if (pos != BORDER_NO_POS) {
				list<ActionEvent> *bl = Config::instance()->getBorderListFromPosition(pos);
				ae = ActionHandler::findMouseAction(ev->button, ev->state, mb, bl);
			}
		}
	}

	return ae;
}

//! @brief
ActionEvent*
PDecor::handleButtonRelease(XButtonEvent *ev)
{
	ActionEvent *ae = NULL;
	MouseEventType mb = MOUSE_EVENT_RELEASE;

	ev->state &= ~Button1Mask & ~Button2Mask & ~Button3Mask
		& ~Button4Mask & ~Button5Mask;
	ev->state &= ~PScreen::instance()->getNumLock() &
							 ~PScreen::instance()->getScrollLock() & ~LockMask;

	// handle titlebar buttons
	if (_button) {
		// first restore the pressed buttons state
		_button->setState(_focused ? BUTTON_STATE_FOCUSED : BUTTON_STATE_UNFOCUSED);

		// then see if the button was released over ( to execute an action )
		if (*_button == ev->subwindow) {
			ae = _button->findAction(ev);

			// this is a little hack, resizing isn't wanted on both press and release
			if ((ae != NULL) && ae->isOnlyAction(ACTION_RESIZE)) {
				ae = NULL;
			}
		}

		_button = NULL;

	} else {

		// allow us to get clicks from anywhere on the window.
		if (_decor_cfg_bpr_replay_pointer == true) {
			XAllowEvents(_dpy, ReplayPointer, CurrentTime);
		}

		// clicks on the child window
		if ((ev->window == _child->getWindow()) ||
				((ev->state == 0) && (ev->subwindow == _child->getWindow()))) {

			// NOTE: If the we're matching against the subwindow we need to make
			// sure that the state is 0, meaning we didn't have any modifier
			// because if we don't care about the modifier we'll get two actions
			// performed when using modifiers.

			ae = ActionHandler::findMouseAction(ev->button, ev->state, mb,
																					Config::instance()->getMouseActionList(_decor_cfg_bpr_al_child));

		// handle clicks on the decor title
		} else if (_title_wo == ev->window) {
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

			ae = ActionHandler::findMouseAction(ev->button, ev->state, mb,
																					Config::instance()->getMouseActionList(_decor_cfg_bpr_al_title));

		// clicks on the decor border
		} else if ((ev->subwindow != None) && (_title_wo != ev->subwindow) &&
							 (ev->window == _window)) {
			uint pos = getBorderPosition(ev->subwindow);
			if (pos != BORDER_NO_POS) {
				list<ActionEvent> *bl = Config::instance()->getBorderListFromPosition(pos);
				ae = ActionHandler::findMouseAction(ev->button, ev->state, mb, bl);
			}
		}
	}

	return ae;
}

//! @brief
ActionEvent*
PDecor::handleMotionEvent(XMotionEvent *ev)
{
	uint button = PScreen::instance()->getButtonFromState(ev->state);
	return ActionHandler::findMouseAction(button, ev->state,
																				MOUSE_EVENT_MOTION,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_OTHER));
}

//! @brief
ActionEvent*
PDecor::handleEnterEvent(XCrossingEvent *ev)
{
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
																				MOUSE_EVENT_ENTER,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_OTHER));
}

//! @brief
ActionEvent*
PDecor::handleLeaveEvent(XCrossingEvent *ev)
{
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
																				MOUSE_EVENT_LEAVE,
																				Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_OTHER));
}

// END - PWinObj interface.

//! @brief Adds a children in another decor to this decor
void
PDecor::addDecor(PDecor *decor)
{
	if (this == decor) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PDecor(" << this << ")::addDecor(" << decor << ")"
				 << " *** this == decor" << endl;
#endif // DEBUG
		return;
	}

	list<PWinObj*>::iterator it(decor->begin());
	for (; it != decor->end(); ++it) {
		addChild(*it);
		(*it)->setWorkspace(_workspace);
	}

	decor->_child_list.clear();
	delete decor;
}

//! @brief Sets prefered decor_name
//! @return True on change, False if unchanged
bool
PDecor::setDecor(const std::string &name)
{
	string new_name(_decor_name_override);
	if (new_name.size() == 0) {
		new_name = name;
	}

	if (_decor_name == new_name) {
		return false;
	}

	_decor_name = new_name;
	loadDecor();

	return true;
}

//! @brief Sets override decor name
void
PDecor::setDecorOverride(StateAction sa, const std::string &name)
{
	if ((sa == STATE_SET) ||
			((sa == STATE_TOGGLE) && (_decor_name_override.size() == 0))) {
		_decor_name_override = name;
		setDecor("");

	} else if ((sa == STATE_UNSET) ||
						 ((sa == STATE_TOGGLE) && (_decor_name_override.size() > 0))) {
		_decor_name_override = ""; // .clear() doesn't work with old g++
		updateDecorName();
	}
}

//! @brief Load and update decor.
void
PDecor::loadDecor(void)
{
	unloadDecor();

	// Get decordata with name.
	_data = _theme->getPDecorData(_decor_name);
	if (_data == NULL) {
		_data = _theme->getPDecorData(DEFAULT_DECOR_NAME);
	}

	// Load decor.
	list<Theme::PDecorButtonData*>::iterator b_it(_data->buttonBegin());
	for (; b_it != _data->buttonEnd(); ++b_it) {
		_button_list.push_back(new PDecor::Button(_dpy, &_title_wo, *b_it));
		_button_list.back()->mapWindow();
	}

	// Update title position.
	if (_data->getTitleWidthMin() == 0) {
		_title_wo.move(borderTopLeft(), borderTop());
		_need_shape = false;
	} else {
		_title_wo.move(0, 0);
		_need_shape = true;
	}

	// Update child positions.
	list<PWinObj*>::iterator c_it(_child_list.begin());
	for (; c_it != _child_list.end(); ++c_it) {
		alignChild(*c_it);
	}

	// Make sure it gets rendered correctly.
	_focused = !_focused;
	setFocused(!_focused);

	// Update the dimension of the frame.
	if (_child != NULL) {
		resizeChild(_child->getWidth(), _child->getHeight());
	}

	// Child theme change.
	loadTheme();
}

//! @brief
void
PDecor::unloadDecor(void)
{
	list<PDecor::Button*>::iterator it(_button_list.begin());
	for (; it != _button_list.end(); ++it) {
		delete *it;
	}
	_button_list.clear();
}

//! @brief
PDecor::Button*
PDecor::findButton(Window win)
{
	list<PDecor::Button*>::iterator it(_button_list.begin());
	for (; it != _button_list.end(); ++it) {
		if (**it == win) {
			return *it;
		}
	}

	return NULL;
}

//! @brief
PWinObj*
PDecor::getChildFromPos(int x)
{
	if (!_child_list.size() || (_child_list.size() != _title_list.size()))
		return NULL;
	if (_child_list.size() == 1)
		return _child_list.front();

	if (x < static_cast<int>(_titles_left)) {
		return _child_list.front();
	} else if (x > static_cast<int>(_title_wo.getWidth() - _titles_right)) {
		return _child_list.back();
	}

	list<PWinObj*>::iterator c_it(_child_list.begin());
	list<PDecor::TitleItem*>::iterator t_it(_title_list.begin());

	// FIXME: make getChildFromPos separator aware!
	uint pos = _titles_left, xx = x;
	for (uint i = 0; i < _title_list.size(); ++i, ++t_it, ++c_it) {
		if ((xx >= pos) && (xx <= (pos + (*t_it)->getWidth()))) {
			return *c_it;
		}
		pos += (*t_it)->getWidth();
	}

  return NULL;
}

//! @brief Moves making the child be positioned at x y
void
PDecor::moveChild(int x, int y)
{
	move(x - borderLeft(), y - borderTop() - getTitleHeight());
}

//! @brief Resizes the decor, giving width x height space for the child
void
PDecor::resizeChild(uint width, uint height)
{
	resize(width + borderLeft() + borderRight(),
				 height + borderTop() + borderBottom() + getTitleHeight());
}

//! @brief Sets border state of the decor
void
PDecor::setBorder(StateAction sa)
{
	if (!ActionUtil::needToggle(sa, _border))
		return;

	// If we are going to remove the border, we need to check carefully
	// that we don't try to make the window disappear.
	if (!_border) {
		_border = true;
	} else if (!_shaded || _titlebar) {
		_border = false;
	}

	restackBorder();
	if (!updateDecorName() && _child)
		resizeChild(_child->getWidth(), _child->getHeight());
}

//! @brief Sets titlebar state of the decor
void
PDecor::setTitlebar(StateAction sa)
{
	if (!ActionUtil::needToggle(sa, _titlebar))
		return;

	// If we are going to remove the titlebar, we need to check carefully
	// that we don't try to make the window disappear.
	if (!_titlebar) {
		_title_wo.mapWindow();
		_titlebar = true;
	} else if (!_shaded || _border) {
		_title_wo.unmapWindow();
		_titlebar = false;
	}

	// If updateDecorName returns true, it allready loaded decor stuff for us.
	if (!updateDecorName() && _child) {
		alignChild(_child);
		resizeChild(_child->getWidth(), _child->getHeight());
	}
}

//! @brief Adds a child to the decor, reparenting the window
void
PDecor::addChild(PWinObj *child)
{
	child->reparent(this, borderLeft(), borderRight() + getTitleHeight());
	_child_list.push_back(child);

	updatedChildOrder();

	// if it's the first child, we'll want to initialize decor with
	// shaping etc
	if (_child_list.size() == 1) {
		_focused = !_focused;
		setFocused(!_focused);
	}
}

//! @brief Removes PWinObj from this PDecor.
//! @param child PWinObj to remove from the this PDecor.
//! @param do_delete Wheter to call delete this when empty. (Defaults to true)
void
PDecor::removeChild(PWinObj *child, bool do_delete)
{
	child->reparent(PWinObj::getRootPWinObj(),
									_gm.x + borderLeft(),
									_gm.y + borderTop() + getTitleHeight());

	list<PWinObj*>::iterator it(find(_child_list.begin(), _child_list.end(), child));
	if (it == _child_list.end()) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PDecor(" << this << ")::removeChild(" << child << ")" << endl
				 << " *** child not in _child_list, bailing out." << endl;
#endif // DEBUG
		return;
	}

	it = _child_list.erase(it);

	if (_child_list.size() > 0) {
		if (_child == child) {
			if (it == _child_list.end()) {
				--it;
			}

			activateChild(*it);
		}

		updatedChildOrder();

	} else if (_decor_cfg_keep_empty) {
		updatedChildOrder();

	} else if (do_delete) {
		delete this; // no children, and we don't want empty PDecors
	}
}

//! @brief Activates the child, updating size and position
void
PDecor::activateChild(PWinObj *child)
{
	// sync state
	_focusable = child->isFocusable();

	alignChild(child); // place correct acording to border and title
	child->resize(getChildWidth(), getChildHeight());
	child->raise();
	_child = child;

	restackBorder();

	updatedChildOrder();
}

//! @brief
void
PDecor::getDecorInfo(char *buf, uint size)
{
	snprintf(buf, size, "%dx%d+%d+%d", _gm.width, _gm.height, _gm.x, _gm.y);
}

//! @brief
void
PDecor::activateChildNum(uint num)
{
	if (num >= _child_list.size()) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PDecor(" << this << ")::activeChildNum(" << num << ")" << endl
				 << " *** num > _child_list.size()" << endl;
#endif // DEBUG
		return;
	}

	list<PWinObj*>::iterator it(_child_list.begin());
	for (uint i = 0; i < num; ++i, ++it);

	activateChild(*it);
}

//! @brief
void
PDecor::activateChildRel(int off)
{
	PWinObj *child = getChildRel(off);
	if (child == NULL) {
		child = _child_list.front();
	}
	activateChild(child);
}

//! @brief Moves the current child off steps
//! @param off - for left and + for right
void
PDecor::moveChildRel(int off)
{
	PWinObj *child = getChildRel(off);
	if ((child == NULL) || (child == _child)) {
		return;
	}

	bool at_begin = (_child == _child_list.front());
	bool at_end = (_child == _child_list.back());

	// switch place
	_child_list.remove(_child);
	list<PWinObj*>::iterator it(find(_child_list.begin(), _child_list.end(), child));
	if (off > 0) {
		// when wrapping, we want to be able to insert at first place
		if ((at_begin == true) || (*it != _child_list.front())) {
			++it;
		}

		_child_list.insert(it, _child);
	} else {
		if ((at_end != true) && (*it == _child_list.back())) {
			++it;
		}
		_child_list.insert(it, _child);
	}

	updatedChildOrder();
}

//! @brief
PWinObj*
PDecor::getChildRel(int off)
{
	if ((off == 0) || (_child_list.size() < 2)) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PDecor(" << this << ")::getChildRel(" << off << ")" << endl
				 << " *** off == 0 or not enough children" << endl;
#endif // DEBUG
		return NULL;
	}

	list<PWinObj*>::iterator it(find(_child_list.begin(), _child_list.end(), _child));
	// if no active child, use first
	if (it == _child_list.end()) {
		it = _child_list.begin();
	}

	int dir = (off > 0) ? 1 : -1;
	off = abs(off);

	for (int i = 0; i < off; ++i) {
		if (dir == 1) { // forward
			if (++it == _child_list.end()) {
				it = _child_list.begin();
			}

		} else { // backward
			if (it == _child_list.begin()) {
				it = _child_list.end();
			}
			--it;
		}
	}

	return *it;
}

//! @brief Pointer enabled moving
void
PDecor::doMove(XMotionEvent *ev)
{
	PScreen *scr = PScreen::instance(); // convenience
	StatusWindow *sw = StatusWindow::instance(); // convenience

	if (scr->grabPointer(scr->getRoot(), ButtonMotionMask|ButtonReleaseMask,
											 ScreenResources::instance()->getCursor(ScreenResources::CURSOR_MOVE)) == false) {
		return;
	}

	uint x = 0, y = 0;
	if (ev != NULL) {
		x = ev->x + borderLeft();
		y = ev->y + borderTop();
		if ((_child != NULL) && (*_child == ev->window)) {
			y += getTitleHeight();
		}
	}

	bool outline = (Config::instance()->getOpaqueMove() == false);
	EdgeType edge;

	// grab server, we don't want invert traces
	if (outline) {
		scr->grabServer();
	}

	char buf[128];
	getDecorInfo(buf, sizeof(buf)/sizeof(char));

	if (Config::instance()->isShowStatusWindow()) {
		sw->draw(buf, true);
		sw->mapWindowRaised();
		sw->draw(buf);
	}

	XEvent e;
	bool exit = false;
	while (exit != true) {
		if (outline) {
			drawOutline(_gm);
		}
		XMaskEvent(_dpy, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, &e);
		if (outline) {
			drawOutline(_gm); // clear
		}

		switch (e.type) {
		case MotionNotify:
			_gm.x = e.xmotion.x_root - x;
			_gm.y = e.xmotion.y_root - y;
			checkSnap();

			if (outline == false) {
				move(_gm.x, _gm.y);
			}

			edge = doMoveEdgeFind(e.xmotion.x_root, e.xmotion.y_root);
			if (edge != SCREEN_EDGE_NO) {
				doMoveEdgeAction(&e.xmotion, edge);
			}

			getDecorInfo(buf, sizeof(buf)/sizeof(char));
			if (Config::instance()->isShowStatusWindow()) {
				sw->draw(buf, true);
			}

			break;
		case ButtonRelease:
			exit = true;
			break;
		}
	}

	if (Config::instance()->isShowStatusWindow()) {
		sw->unmapWindow();
	}

	// ungrab the server
	if (outline) {
		move(_gm.x, _gm.y);
		scr->ungrabServer(true);
	}

	scr->ungrabPointer();
}

//! @brief Matches cordinates against screen edge
EdgeType
PDecor::doMoveEdgeFind(int x, int y)
{
	if (Config::instance()->getScreenEdgeSize() == 0) {
		return SCREEN_EDGE_NO;
	}

	EdgeType edge = SCREEN_EDGE_NO;
	if (x <= signed(Config::instance()->getScreenEdgeSize())) {
		edge = SCREEN_EDGE_LEFT;
	} else if (x >= signed(PScreen::instance()->getWidth() -
												 Config::instance()->getScreenEdgeSize())) {
		edge = SCREEN_EDGE_RIGHT;
	} else if (y <= signed(Config::instance()->getScreenEdgeSize())) {
		edge = SCREEN_EDGE_TOP;
	} else if (y >= signed(PScreen::instance()->getHeight() -
												 Config::instance()->getScreenEdgeSize())) {
		edge = SCREEN_EDGE_BOTTOM;
	}

	return edge;
}

//! @brief Finds and executes action if any
void
PDecor::doMoveEdgeAction(XMotionEvent *ev, EdgeType edge)
{
	ActionEvent *ae;

	uint button = PScreen::instance()->getButtonFromState(ev->state);
	ae = ActionHandler::findMouseAction(button, ev->state,
																			MOUSE_EVENT_ENTER_MOVING,
																			Config::instance()->getEdgeListFromPosition(edge));

	if (ae != NULL) {
		ActionPerformed ap(this, *ae);
		ap.type = ev->type;
		ap.event.motion = ev;

		ActionHandler::instance()->handleAction(ap);
	}
}

//! @brief
void
PDecor::doKeyboardMoveResize(void)
{
	PScreen *scr = PScreen::instance(); // convenience
	StatusWindow *sw = StatusWindow::instance(); // convenience

	if (scr->grabPointer(scr->getRoot(), NoEventMask,
											 ScreenResources::instance()->getCursor(ScreenResources::CURSOR_MOVE)) == false) {
		return;
	}
	if (scr->grabKeyboard(scr->getRoot()) == false) {
		scr->ungrabPointer();
		return;
	}

	Geometry old_gm = _gm; // backup geometry if we cancel
	bool outline = ((Config::instance()->getOpaqueMove() == false) ||
								  (Config::instance()->getOpaqueResize() == false));
	ActionEvent *ae;
	list<Action>::iterator it;

	char buf[128];
	getDecorInfo(buf, sizeof(buf)/sizeof(char));

	if (Config::instance()->isShowStatusWindow()) {
		sw->draw(buf, true);
		sw->mapWindowRaised();
		sw->draw(buf);
	}

	if (outline) {
		PScreen::instance()->grabServer();
	}

	XEvent e;
	bool exit = false;
	while (exit != true) {
		if (outline) {
			drawOutline(_gm);
		}
		XMaskEvent(_dpy, KeyPressMask, &e);
		if (outline) {
			drawOutline(_gm); // clear
		}

		if ((ae = KeyGrabber::instance()->findMoveResizeAction(&e.xkey)) != NULL) {
			for (it = ae->action_list.begin(); it != ae->action_list.end(); ++it) {
				switch (it->getAction()) {
				case MOVE_HORIZONTAL:
					_gm.x += it->getParamI(0);
					if (outline == false) {
						move(_gm.x, _gm.y);
					}
					break;
				case MOVE_VERTICAL:
					_gm.y += it->getParamI(0);
					if (outline == false) {
						move(_gm.x, _gm.y);
					}
					break;
				case RESIZE_HORIZONTAL:
					_gm.width += resizeHorzStep(it->getParamI(0));
					if (outline == false) {
						resize(_gm.width, _gm.height);
					}
					break;
				case RESIZE_VERTICAL:
					_gm.height += resizeVertStep(it->getParamI(0));
					if (outline == false) {
						resize(_gm.width, _gm.height);
					}
					break;
				case MOVE_SNAP:
					checkSnap();
					if (outline == false) {
						move(_gm.x, _gm.y);
					}
					break;
				case MOVE_CANCEL:
					_gm = old_gm; // restore position

					if (outline == false) {
						move(_gm.x, _gm.y);
						resize(_gm.width, _gm.height);
					}

					exit = true;
					break;
				case MOVE_END:
					if (outline) {
						if ((_gm.x != old_gm.x) || (_gm.y != old_gm.y))
							move(_gm.x, _gm.y);
						if ((_gm.width != old_gm.width) || (_gm.height != old_gm.height))
							resize(_gm.width, _gm.height);
					}
					exit = true;
					break;
				default:
					// do nothing
					break;
				}

				getDecorInfo(buf, sizeof(buf)/sizeof(char));
				if (Config::instance()->isShowStatusWindow()) {
					sw->draw(buf, true);
				}
			}
		}
	}

	if (Config::instance()->isShowStatusWindow()) {
		sw->unmapWindow();
	}

	if (outline) {
		scr->ungrabServer(true);
	}

	scr->ungrabKeyboard();
	scr->ungrabPointer();
}

//! @brief Sets shaded state
void
PDecor::setShaded(StateAction sa)
{
	if (!ActionUtil::needToggle(sa, _shaded))
		return;

	// If we are going to shade the window, we need to check carefully
	// that we don't try to make the window disappear.
	if (_shaded) {
		_gm.height = _real_height;
		_shaded = false;
	// If we have a titlebar OR border (border will only be visible in
	// full-width mode only)
	} else if (_titlebar || (_border && !_data->getTitleWidthMin())) {
		_real_height = _gm.height;
		_gm.height = getTitleHeight();
		// shading in non full width title mode will make border go away
		if (!_data->getTitleWidthMin())
			_gm.height += borderTop() + borderBottom();
		_shaded = true;
	}

	PWinObj::resize(_gm.width, _gm.height);
	placeBorder();
	restackBorder();
}

//! @brief Renders and sets title background
void
PDecor::renderTitle(void)
{
	if (getTitleHeight() == 0) {
		return;
	}

	if (_data->getTitleWidthMin()) {
		resizeTitle();
		applyBorderShape(); // update title shape
	} else {
		calcTabsWidth();
	}

	PFont *font = getFont(getFocusedState(false));
	PTexture *t_sep = _data->getTextureSeparator(getFocusedState(false));

	// get new title pixmap
	PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();
	pm->returnPixmap(_title_bg);
	_title_bg = pm->getPixmap(_title_wo.getWidth(), _title_wo.getHeight(),
														PScreen::instance()->getDepth(), false);

	// render main background on pixmap
	_data->getTextureMain(getFocusedState(false))->render(_title_bg, 0, 0,
																												_title_wo.getWidth(),
																												_title_wo.getHeight());

	bool sel;
	uint x = _titles_left;

	list<PDecor::TitleItem*>::iterator it(_title_list.begin());
	for (uint i = 0; it != _title_list.end(); ++i, ++it) {
		sel = (_title_active == i);

		// render tab
		_data->getTextureTab(getFocusedState(sel))->render(_title_bg, x, 0,
																											 (*it)->getWidth(),
																											 _title_wo.getHeight());

		font = getFont(getFocusedState(sel));
		font->setColor(_data->getFontColor(getFocusedState(sel)));

		// this is a icky one, draws the text in the tab depending on justify
		// and wheter or not the title has a TitleRule applied
		if ((*it) != NULL) {
			font->draw(_title_bg,
								 x + ((font->getJustify() == FONT_JUSTIFY_CENTER) ? 0 : _data->getPad(DIRECTION_LEFT)),
								 _data->getPad(DIRECTION_UP),
								 (*it)->getVisible().c_str(), 0, (*it)->getWidth(),
								 (*it)->isRuleApplied() ? PFont::FONT_TRIM_END : PFont::FONT_TRIM_MIDDLE);
		}

		// move to next tab (or separator if any)
		x += (*it)->getWidth();

		// draw separator
		if ((_title_list.size() > 1) && (i < (_title_list.size() - 1))) {
			t_sep->render(_title_bg, x, 0, 0, 0);
			x += t_sep->getWidth();
		}
	}

	_title_wo.setBackgroundPixmap(_title_bg);
	_title_wo.clear();
}

//! @brief
void
PDecor::renderButtons(void)
{
	list<PDecor::Button*>::iterator it(_button_list.begin());
	for (; it != _button_list.end(); ++it) {
		(*it)->setState(_focused ? BUTTON_STATE_FOCUSED : BUTTON_STATE_UNFOCUSED);
	}
}

//! @brief
void
PDecor::renderBorder(void)
{
	if (!_border)
		return;

	PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();

	// return pixmaps used for the border
	list<Pixmap>::iterator it(_border_pix_list.begin());
	for (; it != _border_pix_list.end(); ++it) {
		pm->returnPixmap(*it);
	}
	_border_pix_list.clear();

	Pixmap pix;
	PTexture *tex;
	FocusedState state = getFocusedState(false);
	uint width, height;

	for (uint i = 0; i < BORDER_NO_POS; ++i) {
		tex = _data->getBorderTexture(state, BorderPosition(i));

		// solid texture, get the color and set as bg, no need to render pixmap
		if (tex->getType() == PTexture::TYPE_SOLID) {
			XSetWindowBackground(_dpy, _border_win[i],
													 static_cast<PTextureSolid*>(tex)->getColor()->pixel);

		} else {
		// not a solid texture, get pixmap, render, set as bg
			getBorderSize(BorderPosition(i), width, height);

			if ((width > 0) && (height > 0)) {
				pix = pm->getPixmap(width, height, PScreen::instance()->getDepth(), false);
				tex->render(pix, 0, 0, width, height);
				_border_pix_list.push_back(pix);

				XSetWindowBackgroundPixmap(_dpy, _border_win[i], pix);
			}
		}
	}

	// refresh painted data
	for (uint i = 0; i < BORDER_NO_POS; ++i) {
		XClearWindow(_dpy, _border_win[i]);
	}
}

//! @brief
void
PDecor::setBorderShape(void)
{
#ifdef HAVE_SHAPE
  uint pos[] = { BORDER_TOP_LEFT, BORDER_TOP_RIGHT,
                 BORDER_BOTTOM_LEFT, BORDER_BOTTOM_RIGHT };
  bool f;
  Pixmap pix;
  FocusedState state = getFocusedState(false);

  XRectangle rect;
  rect.x = 0;
  rect.y = 0;

  for (uint i = pos[0]; i < (sizeof(pos)/sizeof(pos[0])); ++i) {
    pix =
      _data->getBorderTexture(state, BorderPosition(pos[i]))->getMask(0, 0, f);
    if (pix != None) {
      _need_shape = true;
      XShapeCombineMask(_dpy, _border_win[pos[i]],
                        ShapeBounding, 0, 0, pix, ShapeSet);
      if (f) {
        ScreenResources::instance()->getPixmapHandler()->returnPixmap(pix);
      }
    } else {
      rect.width =
        _data->getBorderTexture(state, BorderPosition(pos[i]))->getWidth();
      rect.height =
        _data->getBorderTexture(state, BorderPosition(pos[i]))->getHeight();
      XShapeCombineRectangles(_dpy, _border_win[pos[i]], ShapeBounding,
                              0, 0, &rect, 1, ShapeSet, YXBanded);
    }
  }
#endif // HAVE_SHAPE
}

//! @brief Finds the Head closest to x y from the center of the decor
uint
PDecor::getNearestHead(void)
{
	return PScreen::instance()->getNearestHead(_gm.x + (_gm.width / 2),
																						 _gm.y + (_gm.height / 2));
}

//! @brief
void
PDecor::checkSnap(void)
{
	if ((Config::instance()->getWOAttract() > 0) ||
			(Config::instance()->getWOResist() > 0)) {
		checkWOSnap();
	}
	if ((Config::instance()->getEdgeAttract() > 0) ||
			(Config::instance()->getEdgeResist() > 0)) {
		checkEdgeSnap();
	}
}

inline bool // FIXME: hmm
isBetween(int x1, int x2, int t1, int t2)
{
	if (x1 > t1) {
		if (x1 < t2) {
			return true;
		}
	} else if (x2 > t1) {
		return true;
	}
	return false;
}

//! @brief
//! @todo PDecor/PWinObj doesn't have _skip property
void
PDecor::checkWOSnap(void)
{
	Geometry gm = _gm;

	int x = getRX();
	int y = getBY();

	int attract = Config::instance()->getWOAttract();
	int resist = Config::instance()->getWOResist();

	bool snapped;

	list<PWinObj*>::reverse_iterator it = _wo_list.rbegin();
	for (; it != _wo_list.rend(); ++it) {
		if (((*it) == this) || ((*it)->isMapped() == false) ||
				((*it)->getType() != PWinObj::WO_FRAME)) {
			continue;
		}

		snapped = false;

		// check snap
		if ((x >= ((*it)->getX() - attract)) && (x <= ((*it)->getX() + resist))) {
			if (isBetween(gm.y, y, (*it)->getY(), (*it)->getBY())) {
				_gm.x = (*it)->getX() - gm.width;
				snapped = true;
			}
		} else if ((gm.x >= signed((*it)->getRX() - resist)) &&
							 (gm.x <= signed((*it)->getRX() + attract))) {
			if (isBetween(gm.y, y, (*it)->getY(), (*it)->getBY())) {
				_gm.x = (*it)->getRX();
				snapped = true;
			}
		}

		if (y >= ((*it)->getY() - attract) && (y <= (*it)->getY() + resist)) {
			if (isBetween(gm.x, x, (*it)->getX(), (*it)->getRX())) {
				_gm.y = (*it)->getY() - gm.height;
				if (snapped)
					break;
			}
		} else if ((gm.y >= signed((*it)->getBY() - resist)) &&
							 (gm.y <= signed((*it)->getBY() + attract))) {
			if (isBetween(gm.x, x, (*it)->getX(), (*it)->getRX())) {
				_gm.y = (*it)->getBY();
				if (snapped)
					break;
			}
		}
	}
}

//! @brief Snaps decor agains head edges. Only updates _gm, no real move.
//! @todo Add support for checking for harbour and struts
void
PDecor::checkEdgeSnap(void)
{
	int attract = Config::instance()->getEdgeAttract();
	int resist = Config::instance()->getEdgeResist();

	Geometry head;
	PScreen::instance()->getHeadInfo(PScreen::instance()->getNearestHead(_gm.x, _gm.y), head);

	if ((_gm.x >= (head.x - resist)) && (_gm.x <= (head.x + attract))) {
		_gm.x = head.x;
	} else if ((_gm.x + _gm.width) >= (head.x + head.width - attract) &&
						 ((_gm.x + _gm.width) <= (head.x + head.width + resist))) {
		_gm.x = head.x + head.width - _gm.width;
	}
	if ((_gm.y >= (head.y - resist)) && (_gm.y <= (head.y + attract))) {
		_gm.y = head.y;
	} else if (((_gm.y + _gm.height) >= (head.y + head.height - attract)) &&
						 ((_gm.y + _gm.height) <= (head.y + head.height + resist))) {
		_gm.y = head.y + head.height - _gm.height;
	}
}

#ifdef HAVE_SHAPE
//! @brief
bool
PDecor::setShape(bool remove)
{
	if (!_child)
		return false;

	int num, t;
	XRectangle *rect;

	rect = XShapeGetRectangles(_dpy, _child->getWindow(), ShapeBounding,
														 &num, &t);
	// If there is more than one Rectangle it has an irregular shape.
	_need_client_shape = (num > 1);

	// Will set or unset shape depending on _need_shape and _need_client_shape.
	applyBorderShape();

	XFree(rect);

	return (num > 1);
}
#endif // HAVE_SHAPE

//! @brief
void
PDecor::alignChild(PWinObj *child)
{
	if (child == NULL) {
		return;
	}

	XMoveWindow(_dpy, child->getWindow(),
							borderLeft(), borderTop() + getTitleHeight());
}

//! @brief Draws outline of the decor with geometry gm
void
PDecor::drawOutline(const Geometry &gm)
{
	XDrawRectangle(_dpy, PScreen::instance()->getRoot(),
								 _theme->getInvertGC(),
								 gm.x, gm.y, gm.width,
								 _shaded ? _gm.height : gm.height);
}

//! @brief Places decor buttons
void
PDecor::placeButtons(void)
{
	_titles_left = 0;
	_titles_right = 0;

	list<PDecor::Button*>::iterator it(_button_list.begin());
	for (; it != _button_list.end(); ++it) {
		if ((*it)->isLeft()) {
			(*it)->move(_titles_left, 0);
			_titles_left += (*it)->getWidth();
		} else {
			_titles_right += (*it)->getWidth();
			(*it)->move(_title_wo.getWidth() - _titles_right, 0);
		}
	}
}

//! @brief Places border windows
void
PDecor::placeBorder(void)
{
	// if we have tab min == 0 then we have full width title and place the
	// border ontop, else we put the border under the title
	uint bt_off = 0;
	if (_data->getTitleWidthMin() > 0)
		bt_off = getTitleHeight();

  if (borderTop() > 0) {
    XMoveResizeWindow(_dpy, _border_win[BORDER_TOP],
                      borderTopLeft(), bt_off,
                      _gm.width - borderTopLeft() - borderTopRight(),
                      borderTop());

    XMoveResizeWindow(_dpy, _border_win[BORDER_TOP_LEFT],
                      0, bt_off,
                      borderTopLeft(), borderTopLeftHeight());
    XMoveResizeWindow(_dpy, _border_win[BORDER_TOP_RIGHT],
                      _gm.width - borderTopRight(), bt_off,
                      borderTopRight(), borderTopRightHeight());

    if (borderLeft()) {
      XMoveResizeWindow(_dpy, _border_win[BORDER_LEFT],
                        0, borderTopLeftHeight() + bt_off,
                        borderLeft(),
                        _gm.height - borderTopLeftHeight() - borderBottomLeftHeight());
    }

    if (borderRight()) {
      XMoveResizeWindow(_dpy, _border_win[BORDER_RIGHT],
                        _gm.width - borderRight(), borderTopRightHeight() + bt_off,
                        borderRight(),
                        _gm.height - borderTopRightHeight() - borderBottomRightHeight());
    }
  } else {
    if (borderLeft()) {
      XMoveResizeWindow(_dpy, _border_win[BORDER_LEFT],
                        0, getTitleHeight(),
                        borderLeft(),
                        _gm.height - getTitleHeight() - borderBottom());
    }

    if (borderRight()) {
      XMoveResizeWindow(_dpy, _border_win[BORDER_RIGHT],
                        _gm.width - borderRight(), getTitleHeight(),
                        borderRight(),
                        _gm.height - getTitleHeight() - borderBottom());
    }
  }

  if (borderBottom()) {
		XMoveResizeWindow(_dpy, _border_win[BORDER_BOTTOM],
											borderBottomLeft(),
											_gm.height - borderBottom(),
											_gm.width - borderBottomLeft() - borderBottomRight(),
											borderBottom());
		XMoveResizeWindow(_dpy, _border_win[BORDER_BOTTOM_LEFT],
											0,
											_gm.height - borderBottomLeftHeight(),
											borderBottomLeft(),
											borderBottomLeftHeight());
		XMoveResizeWindow(_dpy, _border_win[BORDER_BOTTOM_RIGHT],
											_gm.width - borderBottomRight(),
											_gm.height - borderBottomRightHeight(),
											borderBottomRight(),
											borderBottomRightHeight());
  }

	applyBorderShape();
}

//! @brief
void
PDecor::applyBorderShape(void)
{
#ifdef HAVE_SHAPE
  if (_need_shape || _need_client_shape) {
    // if we have tab min == 0 then we have full width title and place the
    // border ontop, else we put the border under the title
    uint bt_off = 0;
    if (_data->getTitleWidthMin() > 0)
      bt_off = getTitleHeight();

    Window shape;
    shape = XCreateSimpleWindow(_dpy, PScreen::instance()->getRoot(),
                                0, 0, _gm.width, _gm.height, 0, 0, 0);

    if (_child != NULL) {
      XShapeCombineShape(_dpy, shape, ShapeBounding,
                         borderLeft(), borderTop() + getTitleHeight(),
                         _child->getWindow(), ShapeBounding, ShapeSet);
    }

    // Apply border shape. Need to be carefull wheter or not to include it.
    if (_border
        && !(_shaded && bt_off) // Shaded in non-full-width mode removes border.
        && !(_need_client_shape)) { // Shaped clients should appear bordeless.
      // top
      if (borderTop() > 0) {
        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           0, bt_off,
                           _border_win[BORDER_TOP_LEFT],
                           ShapeBounding, ShapeUnion);

        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           borderTopLeft(), bt_off,
                           _border_win[BORDER_TOP],
                           ShapeBounding, ShapeUnion);

        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           _gm.width - borderTopRight(), bt_off,
                           _border_win[BORDER_TOP_RIGHT],
                           ShapeBounding, ShapeUnion);
      }

      // y position for left and right border
      int side_y;
      if (!bt_off && !borderTop())
        side_y = getTitleHeight();
      else
        side_y = bt_off + borderTopLeftHeight();

      // left
      if (borderLeft() > 0) {
        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           0, side_y,
                           _border_win[BORDER_LEFT],
                           ShapeBounding, ShapeUnion);
      }

      // right
      if (borderRight() > 0) {
        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           _gm.width - borderRight(), side_y,
                           _border_win[BORDER_RIGHT],
                           ShapeBounding, ShapeUnion);
      }

      // bottom
      if (borderBottom() > 0) {
        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           0, _gm.height - borderBottomLeftHeight(),
                           _border_win[BORDER_BOTTOM_LEFT],
                           ShapeBounding, ShapeUnion);

        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           borderBottomLeft(), _gm.height - borderBottom(),
                           _border_win[BORDER_BOTTOM],
                           ShapeBounding, ShapeUnion);

        XShapeCombineShape(_dpy, shape, ShapeBounding,
                           _gm.width - borderBottomRight(), _gm.height - borderBottomRightHeight(),
                           _border_win[BORDER_BOTTOM_RIGHT],
                           ShapeBounding, ShapeUnion);
      }
    }
    if (_titlebar) {
      // apply title shape
      XShapeCombineShape(_dpy, shape, ShapeBounding,
                         _title_wo.getX(), _title_wo.getY(),
                         _title_wo.getWindow(),
                         ShapeBounding, ShapeUnion);
    }

    // apply the shape mask to the window
    XShapeCombineShape(_dpy, _window, ShapeBounding, 0, 0, shape,
                       ShapeBounding, ShapeSet);

    XDestroyWindow(_dpy, shape);
  } else {
    XRectangle rect_square;
    rect_square.x = 0;
    rect_square.y = 0;
    rect_square.width = _gm.width;
    rect_square.height = _gm.height;

    XShapeCombineRectangles(_dpy, _window, ShapeBounding,
                            0, 0, &rect_square, 1, ShapeSet, YXBanded);
  }
#endif // HAVE_SHAPE
}

//! @brief Restacks child, title and border windows.
void
PDecor::restackBorder(void)
{
	list<Window> window_list(_border_win, _border_win + BORDER_NO_POS);

	// Add title if any
	if (_titlebar) {
		window_list.push_front(_title_wo.getWindow());
	}

	// Only put the Child over if not shaded.
	if (_child && !_shaded) {
		window_list.push_front(_child->getWindow());
	}

	Window *windows = new Window[window_list.size()];
	copy(window_list.begin(), window_list.end(), windows);

	// Raise the top window so actual restacking is done.
	XRaiseWindow(_dpy, windows[0]);
	XRestackWindows(_dpy, windows, window_list.size());

	delete [] windows;
}

//! @brief  Updates _decor_name to represent decor state
//! @return true if decor was changed
bool
PDecor::updateDecorName(void)
{
	string name;

	if (_titlebar && _border) {
		name = PDecor::DEFAULT_DECOR_NAME;
	} else if (_titlebar) {
		name = PDecor::DEFAULT_DECOR_NAME_BORDERLESS;
	} else if (_border) {
		name = PDecor::DEFAULT_DECOR_NAME_TITLEBARLESS;
	}

	return setDecor(name);
}

//! @brief
void
PDecor::getBorderSize(BorderPosition pos, uint &width, uint &height)
{
	FocusedState state = getFocusedState(false); // convenience

	switch (pos) {
	case BORDER_TOP_LEFT:
	case BORDER_TOP_RIGHT:
	case BORDER_BOTTOM_LEFT:
	case BORDER_BOTTOM_RIGHT:
		width = _data->getBorderTexture(state, pos)->getWidth();
		height = _data->getBorderTexture(state, pos)->getHeight();
		break;
	case BORDER_TOP:
		if ((borderTopLeft() + borderTopRight()) < _gm.width) {
			width = _gm.width - borderTopLeft() - borderTopRight();
		} else {
			width = 1;
		} 
		height = _data->getBorderTexture(state, pos)->getHeight();
		break;
	case BORDER_BOTTOM:
		if ((borderBottomLeft() + borderBottomRight()) < _gm.width) {
			width = _gm.width - borderBottomLeft() - borderBottomRight();
		} else {
			width = 1;
		}
		height = _data->getBorderTexture(state, pos)->getHeight();
		break;
	case BORDER_LEFT:
		width = _data->getBorderTexture(state, pos)->getWidth();
		if ((borderTopLeftHeight() + borderBottomLeftHeight()) < _gm.height) {
			height = _gm.height - borderTopLeftHeight() - borderBottomLeftHeight();
		} else {
			height = 1;
		}
		break;
	case BORDER_RIGHT:
		width = _data->getBorderTexture(state, pos)->getWidth();
		if ((borderTopRightHeight() + borderBottomRightHeight()) < _gm.height) {
			height = _gm.height - borderTopRightHeight() - borderBottomRightHeight();
		} else {
			height = 1;
		}
		break;
	default:
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PDecor(" << this << ")::getBorderSize()" << endl
				 << " *** invalid border position" << endl;
#endif // DEBUG
		width = 1;
		height = 1;
		break;
	};
}

//! @brief
uint
PDecor::calcTitleWidth(void)
{
	uint width = 0;

	if (_data->getTitleWidthMin() == 0) {
		width = _gm.width;
		if (width > (borderTopLeft() + borderTopRight()))
			width -= borderTopLeft() + borderTopRight();

	} else {
		uint width_max = 0;
		// FIXME: what about selected tabs?
		PFont *font = getFont(getFocusedState(false));

		// symetric mode, get max tab width, mutliply with num
		if (_data->isTitleWidthSymetric()) {
			list<PDecor::TitleItem*>::iterator it(_title_list.begin());
			for (; it != _title_list.end(); ++it) {
				width = font->getWidth((*it)->getVisible().c_str());
				if (width > width_max)
					width_max = width;
			}

			width = width_max
				+ _data->getPad(DIRECTION_LEFT)	+ _data->getPad(DIRECTION_RIGHT);
			width *= _title_list.size();

		// asymetric mode, get individual widths
		} else {
			list<PDecor::TitleItem*>::iterator it(_title_list.begin());
			for (; it != _title_list.end(); ++it) {
				width += font->getWidth((*it)->getVisible().c_str())
					+ _data->getPad(DIRECTION_LEFT)	+ _data->getPad(DIRECTION_RIGHT);
			}
		}

		// add width of all separatorns
		width += (_title_list.size() - 1)
			* _data->getTextureSeparator(getFocusedState(false))->getWidth();

		// add width of buttons
		width += _titles_left + _titles_right;

		// validate size
		if (width < static_cast<uint>(_data->getTitleWidthMin()))
			width = _data->getTitleWidthMin();
		if (width > (_gm.width * _data->getTitleWidthMax() / 100))
			width = _gm.width * _data->getTitleWidthMax() / 100;
	}

	return width;
}

//! @brief
void
PDecor::calcTabsWidth(void)
{
	if (!_title_list.size())
		return;

	if (_data->isTitleWidthSymetric()) {
		calcTabsWidthSymetric();
 	} else {
 		calcTabsWidthAsymetric();
 	}
}

//! @brief
void
PDecor::calcTabsWidthSymetric(void)
{
	// include separators in calculation if needed
	uint sep_width = (_title_list.size() - 1)
		* _data->getTextureSeparator(getFocusedState(false))->getWidth();

	// calculate width
	uint width, width_avail, off;
	if (_title_wo.getWidth() > (_titles_left + _titles_right))
		width_avail = _title_wo.getWidth() - _titles_left - _titles_right;
	else
		width_avail = _title_wo.getWidth();

	width = (width_avail - sep_width) / _title_list.size();
	off = (width_avail - sep_width) % _title_list.size();

	// assign width to elements
	list<PDecor::TitleItem*>::iterator it(_title_list.begin());
	for (; it != _title_list.end(); ++it) {
		(*it)->setWidth(width + ((off > 0) ? off-- : 0));
	}
}

//! @brief
void
PDecor::calcTabsWidthAsymetric(void)
{
	calcTabsWidthSymetric();
}
