//
// PWinObj.cc for pekwm
// Copyright (C)  2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "PWinObj.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::list;

PWinObj* PWinObj::_focused_wo = (PWinObj*) NULL;
PWinObj* PWinObj::_root_wo = (PWinObj*) NULL;
list<PWinObj*> PWinObj::_wo_list = list<PWinObj*>();

//! @brief PWinObj constructor.
PWinObj::PWinObj(Display *dpy) :
_dpy(dpy), _window(None),
_parent(NULL), _type(WO_NO_TYPE),
_workspace(0), _layer(LAYER_NORMAL),
_mapped(false), _iconified(false), _hidden(false),
_focused(false), _sticky(false),
_focusable(true)
{
}

//! @brief PWinObj destructor.
PWinObj::~PWinObj(void)
{
	if (_focused_wo == this) {
		_focused_wo = NULL;
	}
}

//! @brief Maps the window and sets _mapped to true.
void
PWinObj::mapWindow(void)
{
	if (_mapped == true) {
		return;
	}
	_mapped = true;
	_iconified = false;

	XMapWindow(_dpy, _window);
}

//! @brief Maps the window and raises it
void
PWinObj::mapWindowRaised(void)
{
	if (_mapped == true) {
		return;
	}
	_mapped = true;
	_iconified = false;

	XMapRaised(_dpy, _window);
}

//! @brief Unmaps the window and sets _mapped to false.
void
PWinObj::unmapWindow(void)
{
	if (!_mapped)
		return;
	_mapped = false;

  // Make sure unmapped windows drops focus
  setFocused(false);

	XUnmapWindow(_dpy, _window);
}

//! @brief Only sets _iconified to true.
void
PWinObj::iconify(void)
{
	if (_iconified)
		return;
	_iconified = true;
}

//! @brief Toggles _sticky
void
PWinObj::stick(void)
{
	_sticky = !_sticky;
}

//! @brief Moves the window and updates _gm.
//! @param x X position
//! @param y Y position
//! @param do_virtual Update virtual postion, defaults to true
void
PWinObj::move(int x, int y, bool do_virtual)
{
	(&do_virtual);

	_gm.x = x;
	_gm.y = y;

	XMoveWindow(_dpy, _window, _gm.x, _gm.y);
}

//! @brief Interface method for virtual desktop placing
void
PWinObj::moveVirtual(int x, int y)
{
	_v_x = x;
	_v_y = y;
}

//! @brief Resizes the window and updates _gm.
void
PWinObj::resize(uint width, uint height)
{
#ifdef DEBUG
	if ((width == 0) || (height == 0)) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PWinObj(" << this << ")::resize(" << width << "," << height << ")"
				 << endl << " *** invalid geometry, _window = " << _window << endl;
		return;
	}
#endif // DEBUG

	_gm.width = width;
	_gm.height = height;

	XResizeWindow(_dpy, _window, _gm.width, _gm.height);
}

//! @brief Only sets _workspace to workspace.
void
PWinObj::setWorkspace(uint workspace)
{
	_workspace = workspace;
}

//! @brief Only sets _layer to layer.
void
PWinObj::setLayer(uint layer)
{
	_layer = layer;
}

//! @brief Only sets _focused to focused.
void
PWinObj::setFocused(bool focused)
{
	_focused = focused;
}

//! @brief Only sets _sticky to sticky.
void
PWinObj::setSticky(bool sticky)
{
	_sticky = sticky;
}

//! @brief Only sets _hidden to hidden.
void
PWinObj::setHidden(bool hidden)
{
	_hidden = hidden;
}

//! @brief Executes XSetInputFocus on the appropriate window.
bool
PWinObj::giveInputFocus(void)
{
	if ((_mapped == false) || (_focusable == false)) {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PWinObj(" << this << ")::giveInputFocus()" << endl
				 << " *** non focusable window."
				 << " mapped: " << _mapped  << " focusable: " << _focusable << endl;
#endif // DEBUG
		return false;
	}

	XSetInputFocus(_dpy, _window, RevertToPointerRoot, CurrentTime);

	return true;
}

//! @brief Reparents and sets _parent member
void
PWinObj::reparent(PWinObj *wo, int x, int y)
{
	_parent = wo;
	XReparentWindow(_dpy, _window, wo->getWindow(), x, y);
}
