//
// PWinObj.cc for pekwm
// Copyright (C) 2003-2023 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <algorithm>
#include <iostream>

#include "Debug.hh"
#include "PWinObj.hh"

PWinObj::PWinObjDeleted PWinObj::pwin_obj_deleted;

Window PWinObj::_win_skip_enter_after = None;
PWinObj* PWinObj::_skip_enter_after = nullptr;
PWinObj* PWinObj::_focused_wo = nullptr;
PWinObj* PWinObj::_root_wo = nullptr;
std::vector<PWinObj*> PWinObj::_wo_list = std::vector<PWinObj*>();
std::map<Window, PWinObj*> PWinObj::_wo_map = std::map<Window, PWinObj*>();

//! @brief PWinObj constructor.
PWinObj::PWinObj(bool keyboard_input)
	: _window(None),
	  _parent(0),
	  _type(WO_NO_TYPE),
	  _lastActivity(X11::getLastEventTime()),
	  _opaque(true),
	  _workspace(0),
	  _layer(LAYER_NORMAL),
	  _mapped(false),
	  _iconified(false),
	  _hidden(false),
	  _focused(false),
	  _sticky(false),
	  _focusable(true),
	  _shape_bounding(false),
	  _keyboard_input(keyboard_input)
{
}

//! @brief PWinObj destructor.
PWinObj::~PWinObj(void)
{
	if (_focused_wo == this) {
		_focused_wo = 0;
	}
	if (_skip_enter_after == this) {
		_skip_enter_after = nullptr;
		_win_skip_enter_after = _window;
	}

	pekwm::observerMapping()->notifyObservers(this, &pwin_obj_deleted);
}

//! @brief Sets the desired opacity values for focused/unfocused states
void
PWinObj::setOpacity(uint focused, uint unfocused, bool enabled)
{
	_opacity.focused = focused;
	_opacity.unfocused = unfocused;
	_opaque = !enabled;
	updateOpacity();
}

//! @brief Updates the opacity hint based on focused state
void
PWinObj::updateOpacity(void)
{
	Cardinal opacity;
	if (_opaque) {
		opacity = EWMH_OPAQUE_WINDOW;
	} else {
		opacity = isFocused()?_opacity.focused:_opacity.unfocused;
	}

	if (_opacity.current != opacity) {
		_opacity.current = opacity;
		X11::setCardinal(_window, NET_WM_WINDOW_OPACITY, opacity);
	}
}

//! @brief Maps the window and sets _mapped to true.
void
PWinObj::mapWindow(void)
{
	if (_mapped) {
		return;
	}
	_mapped = true;
	_iconified = false;

	X11::mapWindow(_window);
}

//! @brief Maps the window and raises it
void
PWinObj::mapWindowRaised(void)
{
	if (_mapped) {
		return;
	}
	_mapped = true;
	_iconified = false;

	X11::mapRaised(_window);
}

//! @brief Unmaps the window and sets _mapped to false.
void
PWinObj::unmapWindow(void)
{
	if (! _mapped) {
		return;
	}

	_mapped = false;

	// Make sure unmapped windows drops focus
	setFocused(false);

	X11::unmapWindow(_window);
}

//! @brief Only sets _iconified to true.
void
PWinObj::iconify(void)
{
	if (_iconified) {
		return;
	}

	_iconified = true;
}

/**
 * Toggle sticky state.
 */
void
PWinObj::toggleSticky()
{
	_sticky = !_sticky;
}

//! @brief Returns head PWinObj is on.
uint
PWinObj::getHead(void)
{
	int x = _gm.x + (_gm.width / 2);
	int y = _gm.y + (_gm.height / 2);
	return X11::getNearestHead(x, y);
}

//! @brief Moves the window and updates _gm.
//! @param x X position
//! @param y Y position
void
PWinObj::move(int x, int y)
{
	_gm.x = x;
	_gm.y = y;

	X11::moveWindow(_window, _gm.x, _gm.y);
}

//! @brief Resizes the window and updates _gm.
void
PWinObj::resize(uint width, uint height)
{
	if (! width || ! height) {
		P_WARN("width " << width << " height " << height
		       << ", invalid geometry");
		return;
	}

	_gm.width = width;
	_gm.height = height;

	X11::resizeWindow(_window, _gm.width, _gm.height);
}

//! @brief Move and resize window in one call.
//! @param x X position.
//! @param y Y Position.
//! @param width Width.
//! @param height Height.
void
PWinObj::moveResize(int x, int y, uint width, uint height)
{
	if (! width || ! height) {
		P_WARN("Invalid geometry, width/height not set");
		return;
	}

	_gm.x = x;
	_gm.y = y;
	_gm.width = width;
	_gm.height = height;

	X11::moveResizeWindow(_window, _gm.x, _gm.y, _gm.width, _gm.height);
}

//! @brief Only sets _workspace to workspace.
void
PWinObj::setWorkspace(uint workspace)
{
	_workspace = workspace;
}

//! @brief Only sets _layer to layer.
void
PWinObj::setLayer(Layer layer)
{
	_layer = layer;
}

//! @brief Sets _focused to focused and updates opacity as needed.
void
PWinObj::setFocused(bool focused)
{
	_focused = focused;
	updateOpacity();
}

//! @brief Only sets _sticky to sticky.
void
PWinObj::setSticky(bool sticky)
{
	_sticky = sticky;
}

//! @brief Updates opaque state
void
PWinObj::setOpaque(bool opaque)
{
	_opaque = opaque;
	updateOpacity();
}

//! @brief Executes XSetInputFocus on the appropriate window.
void
PWinObj::giveInputFocus(void)
{
	if (! _mapped  || ! _focusable) {
		P_WARN("trying to focus non focusable window. mapped "
		       << _mapped << " focusable " << _focusable);
		return;
	}

	X11::setInputFocus(_window);
}

//! @brief Reparents and sets _parent member
void
PWinObj::reparent(PWinObj *wo, int x, int y)
{
	_parent = wo;
	X11::reparentWindow(_window, wo->getWindow(), x, y);
}

//! @brief Get required size to hold content for window
//! @param request Geometry filled with size request.
//! @return true if geometry is filled in, else false
bool
PWinObj::getSizeRequest(Geometry &request)
{
	return false;
}

//! @brief Adds PWinObj to _wo_list.
void
PWinObj::woListAdd(PWinObj *wo)
{
	_wo_list.push_back(wo);
}

//! @brief Remove PWinObj from _wo_list.
void
PWinObj::woListRemove(PWinObj *wo)
{
	std::vector<PWinObj*>::iterator it
		= std::find(_wo_list.begin(), _wo_list.end(), wo);
	if (it != _wo_list.end()) {
		_wo_list.erase(it);
	}
}
