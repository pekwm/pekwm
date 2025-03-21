//
// DockApp.cc for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2003-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "DockApp.hh"

#include "Charset.hh"
#include "Config.hh"
#include "X11.hh"
#include "PDecor.hh"
#include "AutoProperties.hh"

#include "tk/PTexture.hh"
#include "tk/PWinObj.hh"
#include "tk/Theme.hh"

extern "C" {
#include <X11/Xutil.h>
}

const uint DOCKAPP_DEFAULT_SIDE = 64;
const uint DOCKAPP_BORDER_WIDTH = 2;

//! @brief DockApp constructor
DockApp::DockApp(Window win) :
	PWinObj(false),
	_dockapp_window(win),
	_client_window(win), _icon_window(None),
	_position(0), _background(None),
	_is_alive(true)
{
	Config *cfg = pekwm::config();

	// PWinObj attributes.
	_type = WO_DOCKAPP;
	_iconified = true; // We set ourself iconified for workspace switching.
	_sticky = true;

	// First, we need to figure out which window that actually belongs to
	// the dockapp. This we do by checking if it has the IconWindowHint
	// set in it's WM Hint.
	XWMHints wm_hints;
	if (X11::getWMHints(_dockapp_window, wm_hints)
	    && (wm_hints.flags&IconWindowHint)
	    && (wm_hints.icon_window != None)) {
		// hide the _client_window window, as it will not be used
		X11::unmapWindow(_client_window);

		_icon_window = wm_hints.icon_window;
		_dockapp_window = wm_hints.icon_window;
	}

	// Setup size after detected the window to use
	XWindowAttributes attr;
	if (X11::getWindowAttributes(_dockapp_window, attr)) {
		_c_gm.width = attr.width;
		_c_gm.height = attr.height;

		_gm.width = attr.width;
		_gm.height = attr.height;
	} else {
		if (cfg->getHarbourDAMinSide() > 0) {
			_c_gm.width = cfg->getHarbourDAMinSide();
			_c_gm.height = cfg->getHarbourDAMinSide();
		} else {
			_c_gm.width = DOCKAPP_DEFAULT_SIDE
				- DOCKAPP_BORDER_WIDTH * 2;
			_c_gm.height = DOCKAPP_DEFAULT_SIDE
				- DOCKAPP_BORDER_WIDTH * 2;
		}
		_gm.width = _c_gm.width;
		_gm.height = _c_gm.height;
	}

	// make sure size is valid and position the dockapp
	updateSize();

	// create parent window which is going to hold the border
	_window = X11::createWmWindow(X11::getRoot(),
				      _gm.x, _gm.y, _gm.width, _gm.height,
				      InputOutput,
				      SubstructureRedirectMask|ButtonPressMask|
				      ButtonMotionMask);

	// initial makeup
	repaint();
	XSetWindowBorderWidth(X11::getDpy(), _dockapp_window, 0);

	// move the dockapp to it's new parent, making sure we don't
	// get any UnmapEvents
	X11::selectInput(_dockapp_window, NoEventMask);
	XReparentWindow(X11::getDpy(), _dockapp_window, _window,
			_c_gm.x, _c_gm.y);
	X11::selectInput(_dockapp_window, SubstructureNotifyMask);

	readClassHint();
	readAutoProperties();
}

//! @brief DockApp destructor
DockApp::~DockApp(void)
{
	// if the client still is alive, we should reparent it to the root
	// window, else we don't have to care about that.
	if (_is_alive) {
		X11::grabServer();

		if (_icon_window != None) {
			X11::unmapWindow(_icon_window);
		}

		// move the dockapp back to the root window, making sure we
		// don't get any UnmapEvents
		X11::selectInput(_dockapp_window, NoEventMask);
		XReparentWindow(X11::getDpy(), _dockapp_window, X11::getRoot(),
				_gm.x, _gm.y);
		X11::mapWindow(_client_window);

		X11::ungrabServer(false);
	}

	X11::freePixmap(_background);
	X11::destroyWindow(_window);
}

// START - PWinObj interface.

//! @brief Maps the DockApp
void
DockApp::mapWindow(void)
{
	if (_mapped) {
		return;
	}
	_mapped = true;

	X11::selectInput(_dockapp_window, NoEventMask);
	X11::mapWindow(_window);
	X11::mapWindow(_dockapp_window);
	X11::selectInput(_dockapp_window,
			 StructureNotifyMask|SubstructureNotifyMask);
}

//! @brief Unmaps the DockApp
void
DockApp::unmapWindow(void)
{
	if (! _mapped) {
		return;
	}
	_mapped = false;

	X11::selectInput(_dockapp_window, NoEventMask);
	X11::unmapWindow(_dockapp_window);
	X11::unmapWindow(_window);
	X11::selectInput(_dockapp_window,
			 StructureNotifyMask|SubstructureNotifyMask);
}

// END - PWinObj interface.

//! @brief Kills the DockApp
void
DockApp::kill(void)
{
	XKillClient(X11::getDpy(), _dockapp_window);
}

//! @brief Resizes the DockApp, size excludes the border.
//! @todo Make sure it's inside the screen!
void
DockApp::resize(uint width, uint height)
{
	if ((_c_gm.width == width) && (_c_gm.height == height)) {
		return;
	}

	_c_gm.width = width;
	_c_gm.height = height;

	updateSize();

	X11::moveResizeWindow(_window,
			      _gm.x, _gm.y, _gm.width, _gm.height);
	X11::moveResizeWindow(_dockapp_window,
			      _c_gm.x, _c_gm.y, _c_gm.width, _c_gm.height);

	repaint();
}

//! @brief Loads the current theme and repaints the DockApp.
void
DockApp::loadTheme(void)
{
	// Note, here we are going to check if the DockApp borderwidth have
	// changed etc in the future but for now we'll only have to repaint it
	repaint();
}

//! @brief Repaints the DockApp's background.
void
DockApp::repaint(void)
{
	X11::freePixmap(_background);
	_background = X11::createPixmap(_gm.width, _gm.height);

	Theme::HarbourData *hd = pekwm::theme()->getHarbourData();
	hd->getTexture()->render(_background, 0, 0, _gm.width, _gm.height);

	X11::setWindowBackgroundPixmap(_window, _background);
	X11::clearWindow(_window);
}

//! @brief Validates geometry and centers the window
void
DockApp::updateSize(void)
{
	// resize the window holding the dockapp
	_gm.width = _c_gm.width + DOCKAPP_BORDER_WIDTH * 2;
	_gm.height = _c_gm.height + DOCKAPP_BORDER_WIDTH * 2;

	// resize
	validateSize();

	// position the dockapp
	_c_gm.x = (_gm.width - _c_gm.width) / 2;
	_c_gm.y = (_gm.height - _c_gm.height) / 2;
}

//! @brief Makes sure DockApp conforms to SideMin and SideMax
void
DockApp::validateSize(void)
{
	Config *cfg = pekwm::config(); // convenience

	if (cfg->getHarbourDAMinSide() > 0) {
		if (_gm.width
		    < static_cast<uint>(cfg->getHarbourDAMinSide())) {
			_gm.width = cfg->getHarbourDAMinSide();
		}
		if (_gm.height
		    < static_cast<uint>(cfg->getHarbourDAMinSide())) {
			_gm.height = cfg->getHarbourDAMinSide();
		}
	}

	if (cfg->getHarbourDAMaxSide() > 0) {
		if (_gm.width
		    > static_cast<uint>(cfg->getHarbourDAMaxSide())) {
			_gm.width = cfg->getHarbourDAMaxSide();
		}
		if (_gm.height
		    > static_cast<uint>(cfg->getHarbourDAMaxSide())) {
			_gm.height = cfg->getHarbourDAMaxSide();
		}
	}
}

//! @brief Reads XClassHint of client.
void
DockApp::readClassHint(void)
{
	XClassHint x_class_hint;
	if (XGetClassHint(X11::getDpy(), _client_window, &x_class_hint)) {
		_class_hint.h_name = x_class_hint.res_name;
		_class_hint.h_class = x_class_hint.res_class;
		X11::free(x_class_hint.res_name);
		X11::free(x_class_hint.res_class);
	}
}

//! @brief Reads DockApp AutoProperties.
void
DockApp::readAutoProperties(void)
{
	DockAppProperty *prop =
		pekwm::autoProperties()->findDockAppProperty(_class_hint);
	if (prop) {
		_position = prop->getPosition();
	}
}
