//
// WindowObject.cc for pekwm
// Copyright (C) 2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowObject.hh"

//! @fn    WindowObject(Display *dpy)
//! @brief WindowObject constructor.
WindowObject::WindowObject(Display *dpy) :
_dpy(dpy), _window(None),
_type(WO_NO_TYPE),
_workspace(0), _layer(LAYER_NORMAL),
_mapped(false), _iconified(false),
_focused(false), _sticky(false)
{
}

//! @fn    ~WindowObject()
//! @brief WindowObject destructor.
WindowObject::~WindowObject()
{
}

//! @fn    void mapWindow(void)
//! @brief Maps the window and sets _mapped to true.
void
WindowObject::mapWindow(void)
{
	if (_mapped)
		return;
	_mapped = true;
	_iconified = false;

	XMapWindow(_dpy, _window);
}

//! @fn    void unmapWindow(void)
//! @brief Unmaps the window and sets _mapped to false.
void
WindowObject::unmapWindow(void)
{
	if (!_mapped)
		return;
	_mapped = false;

	XUnmapWindow(_dpy, _window);
}

//! @fn    void iconify(void)
//! @brief Only sets _iconified to true.
void
WindowObject::iconify(void)
{
	if (_iconified)
		return;
	_iconified = true;
}

//! @fn    void move(int x, int y)
//! @brief Moves the window and updates _gm.
void
WindowObject::move(int x, int y)
{
	_gm.x = x;
	_gm.y = y;

	XMoveWindow(_dpy, _window, _gm.x, _gm.y);
}

//! @fn    void resize(unsigned int width, unsigned int height)
//! @brief Resizes the window and updates _gm.
void
WindowObject::resize(unsigned int width, unsigned int height)
{
	_gm.width = width;
	_gm.height = height;

	XResizeWindow(_dpy, _window, _gm.width, _gm.height);
}

//! @fn    void setWorkspace(unsigned int workspace)
//! @brief Only sets _workspace to workspace.
void
WindowObject::setWorkspace(unsigned int workspace)
{
	_workspace = workspace;
}

//! @fn    void setLayer(unsigned int layer)
//! @brief Only sets _layer to layer.
void
WindowObject::setLayer(unsigned int layer)
{
	_layer = layer;
}

//! @fn    void setFocused(bool focused)
//! @brief Only sets _focused to focused.
void
WindowObject::setFocused(bool focused)
{
	_focused = focused;
}

//! @fn    void setSticky(bool sticky)
//! @brief Only sets _sticky to sticky.
void
WindowObject::setSticky(bool sticky)
{
	_sticky = sticky;
}

//! @fn    void giveInputFocus(void)
//! @brief Executes XSetInputFocus on the appropriate window.
void
WindowObject::giveInputFocus(void)
{
	if (!_mapped)
		return;

	XSetInputFocus(_dpy, _window, RevertToPointerRoot, CurrentTime);
}
