//
// Dockapp.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HARBOUR

#include "WindowObject.hh"
#include "DockApp.hh"

#include "ScreenInfo.hh"
#include "Theme.hh"
#include "Image.hh"

extern "C" {
#include <X11/Xutil.h>
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

const unsigned int DOCKAPP_DEFAULT_SIDE = 64;
const unsigned int DOCKAPP_BORDER_WIDTH = 2;

DockApp::DockApp(ScreenInfo *s, Theme *t, Window win) :
WindowObject(s->getDisplay()),
_scr(s), _theme(t),
_dockapp_window(None),
_client_window(win), _icon_window(None),
_background(None),
_is_alive(true)
{
	// WindowObject attributes.
	_dockapp_window = win;
	_type = WO_DOCKAPP;
	_iconified = true; // We set ourself iconified for workspace switching.
	_sticky = true;

	// First, we need to figure out which window that actually belongs to the
	// dockapp. This we do by checking if it has the IconWindowHint set in it's
	// WM Hint.
	XWMHints *wm_hints = XGetWMHints(_dpy, _dockapp_window);
	if (wm_hints) {
		if ((wm_hints->flags&IconWindowHint) &&
				(wm_hints->icon_window != None)) {
			// let us hide the m_client_window window, as we won't use it.
			XUnmapWindow(_dpy, _client_window);

			_icon_window = wm_hints->icon_window;
			_dockapp_window = wm_hints->icon_window;
		}
		XFree(wm_hints);
	}

	// Now, when we now what window id we should use, set the size up.
	XWindowAttributes attr;
	if (XGetWindowAttributes(_dpy, _dockapp_window, &attr)) {
		_gm.width = attr.width;
		_gm.height = attr.height;
	} else {
		_gm.width = DOCKAPP_DEFAULT_SIDE;
		_gm.height =  DOCKAPP_DEFAULT_SIDE;
	}

	// Add the border to the size
	_gm.width += DOCKAPP_BORDER_WIDTH * 2;
	_gm.height += DOCKAPP_BORDER_WIDTH * 2;

	// Okie, now lets create it's parent window which is going to hold the border
	XSetWindowAttributes sattr;
	sattr.override_redirect = true;
	sattr.event_mask = SubstructureRedirectMask|ButtonPressMask|ButtonMotionMask;

	_window =
		XCreateWindow(_dpy, _scr->getRoot(),
									_gm.x, _gm.y, _gm.width, _gm.height, 0,
									CopyFromParent, InputOutput, CopyFromParent,
									CWOverrideRedirect|CWEventMask, &sattr);

	// Initial makeup
	repaint();
	XSetWindowBorderWidth(_dpy, _dockapp_window, 0);

	// Move the dockapp to it's new parent, we filter this so that we won't
	// get any UnmapEvents
	XSelectInput(_dpy, _dockapp_window, NoEventMask);
	XReparentWindow(_dpy, _dockapp_window, _window,
									DOCKAPP_BORDER_WIDTH, DOCKAPP_BORDER_WIDTH);
	XSelectInput(_dpy, _dockapp_window,
							 StructureNotifyMask|SubstructureNotifyMask);
}

DockApp::~DockApp()
{
	// if the client still is alive, we should reparent it to the root
	// window, else we don't have to care about that.
	if (_is_alive) {
		if (_icon_window != None)
			XUnmapWindow(_dpy, _icon_window);

		XReparentWindow(_dpy, _dockapp_window, _scr->getRoot(), _gm.x, _gm.y);
		XMapWindow(_dpy, _client_window);
	}

	// Clean up
	XDestroyWindow(_dpy, _window);
	if (_background != None)
		XFreePixmap(_dpy, _background);
}

// START - WindowObject interface.

//! @fn    void mapWindow(void)
//! @brief Maps the DockApp
void
DockApp::mapWindow(void)
{
	if (_mapped)
		return;
	_mapped = true;

	XMapWindow(_dpy, _window);

	XSelectInput(_dpy, _dockapp_window, NoEventMask);
	XMapWindow(_dpy, _dockapp_window);
	XSelectInput(_dpy, _dockapp_window,
							 StructureNotifyMask|SubstructureNotifyMask);
}

//! @fn    void unmapWindow(void)
//! @brief Unmaps the DockApp
void
DockApp::unmapWindow(void)
{
	if (!_mapped)
		return;
	_mapped = true;

	XSelectInput(_dpy, _dockapp_window, NoEventMask);
	XUnmapWindow(_dpy, _dockapp_window);
	XSelectInput(_dpy, _dockapp_window,
							 StructureNotifyMask|SubstructureNotifyMask);

	XUnmapWindow(_dpy, _window);
}

// END - WindowObject interface.

//! @fn    int getRX(void) const
//! @brief Convinince method getting the right x position.
int
DockApp::getRX(void) const
{
	return (_gm.x + _gm.width);
}

//! @fn    int getRY(void) const
//! @brief Convinince method getting the bottom y position.
int
DockApp::getRY(void) const
{
	return (_gm.y + _gm.height);
}

//! @fn    void kill(void)
//! @brief Kills the DockApp
void
DockApp::kill(void)
{
	XKillClient(_dpy, _dockapp_window);
}

//! @fn    void move(int x, int y)
//! @brief Moves the DockApp to x,y
void
DockApp::move(int x, int y)
{
	_gm.x = x;
	_gm.y = y;

	XMoveWindow(_dpy, _window, _gm.x, _gm.y);
}

//! @fn    void resize(unsigned int width, unsigned int height)
//! @breif Resizes the DockApp, size excludes the border.
//! @todo Make sure it's inside the screen!
void
DockApp::resize(unsigned int width, unsigned int height)
{
	if ((_gm.width == width) && (_gm.height == height))
		return;

	_gm.width = width + (DOCKAPP_BORDER_WIDTH * 2);
	_gm.height = height + (DOCKAPP_BORDER_WIDTH * 2);

	XResizeWindow(_dpy, _window, _gm.width, _gm.height);
	XResizeWindow(_dpy, _dockapp_window, width, height);

	repaint();
}

//! @fn    void loadTheme(void)
//! @brief Loads the current theme and repaints the DockApp.
void
DockApp::loadTheme(void)
{
	// Note, here we are going to check if the DockApp borderwidth have
	// changed etc in the future but for now we'll only have to repaint it
	repaint();
}

//! @fn    void repaint(void)
//! @brief Repaints the DockApp's background.
void
DockApp::repaint(void)
{
	if (_background != None)
		XFreePixmap(_dpy, _background);

	_background = XCreatePixmap(_dpy, _scr->getRoot(),
															_gm.width, _gm.height, _scr->getDepth());
	_theme->getHarbourImage()->draw(_background, 0, 0, _gm.width, _gm.height);

	XSetWindowBackgroundPixmap(_dpy, _window, _background);
	XClearWindow(_dpy, _window);
}

#endif // HARBOUR
