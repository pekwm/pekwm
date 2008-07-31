//
// Dockapp.cc for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HARBOUR

#include "PWinObj.hh"
#include "DockApp.hh"

#include "Config.hh"
#include "PScreen.hh"
#include "PTexture.hh"
#include "PDecor.hh"
#include "ScreenResources.hh"
#include "Theme.hh"
#include "PixmapHandler.hh"
#include "AutoProperties.hh"

extern "C" {
#include <X11/Xutil.h>
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

const uint DOCKAPP_DEFAULT_SIDE = 64;
const uint DOCKAPP_BORDER_WIDTH = 2;

//! @brief DockApp constructor
DockApp::DockApp(PScreen *s, Theme *t, Window win) :
        PWinObj(s->getDpy()),
        _scr(s), _theme(t),
        _dockapp_window(win),
        _client_window(win), _icon_window(None),
        _position(0), _background(None),
        _is_alive(true)
{
    Config *cfg = Config::instance();

    // PWinObj attributes.
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
            // let us hide the _client_window window, as we won't use it.
            XUnmapWindow(_dpy, _client_window);

            _icon_window = wm_hints->icon_window;
            _dockapp_window = wm_hints->icon_window;
        }
        XFree(wm_hints);
    }

    // Now, when we now what window id we should use, set the size up.
    XWindowAttributes attr;
    if (XGetWindowAttributes(_dpy, _dockapp_window, &attr)) {
        _c_gm.width = attr.width;
        _c_gm.height = attr.height;

        _gm.width = attr.width;
        _gm.height = attr.height;

    } else {
        // Didn't get any size from the dockapp, use default
        if (cfg->getHarbourDAMinSide() > 0) {
            _c_gm.width = cfg->getHarbourDAMinSide();
            _c_gm.height = cfg->getHarbourDAMinSide();
        } else {
            _c_gm.width = DOCKAPP_DEFAULT_SIDE - DOCKAPP_BORDER_WIDTH * 2;
            _c_gm.height = DOCKAPP_DEFAULT_SIDE - DOCKAPP_BORDER_WIDTH * 2;
        }

        _gm.width = _c_gm.width;
        _gm.height = _c_gm.height;
    }

    // make sure size is valid and position the dockapp
    updateSize();

    // Okie, now lets create it's parent window which is going to hold the border
    XSetWindowAttributes sattr;
    sattr.override_redirect = True;
    sattr.event_mask = SubstructureRedirectMask|ButtonPressMask|ButtonMotionMask;

    _window =
        XCreateWindow(_dpy, _scr->getRoot(),
                      _gm.x, _gm.y, _gm.width, _gm.height, 0,
                      CopyFromParent, InputOutput, CopyFromParent,
                      CWOverrideRedirect|CWEventMask, &sattr);

    // initial makeup
    repaint();
    XSetWindowBorderWidth(_dpy, _dockapp_window, 0);

    // move the dockapp to it's new parent, making sure we don't
    // get any UnmapEvents
    XSelectInput(_dpy, _dockapp_window, NoEventMask);
    XReparentWindow(_dpy, _dockapp_window, _window, _c_gm.x, _c_gm.y);
    XSelectInput(_dpy, _dockapp_window, SubstructureNotifyMask);

    readClassHint();
    readAutoProperties();
}

//! @brief DockApp destructor
DockApp::~DockApp(void)
{
    // if the client still is alive, we should reparent it to the root
    // window, else we don't have to care about that.
    if (_is_alive) {
        _scr->grabServer();

        if (_icon_window != None) {
            XUnmapWindow(_dpy, _icon_window);
        }

        // move the dockapp back to the root window, making sure we don't
        // get any UnmapEvents
        XSelectInput(_dpy, _dockapp_window, NoEventMask);
        XReparentWindow(_dpy, _dockapp_window, _scr->getRoot(), _gm.x, _gm.y);
        XMapWindow(_dpy, _client_window);

        _scr->ungrabServer(false);
    }

    // clean up
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_background);
    XDestroyWindow(_dpy, _window);
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

    XSelectInput(_dpy, _dockapp_window, NoEventMask);
    XMapWindow(_dpy, _window);
    XMapWindow(_dpy, _dockapp_window);
    XSelectInput(_dpy, _dockapp_window,
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

    XSelectInput(_dpy, _dockapp_window, NoEventMask);
    XUnmapWindow(_dpy, _dockapp_window);
    XUnmapWindow(_dpy, _window);
    XSelectInput(_dpy, _dockapp_window,
                 StructureNotifyMask|SubstructureNotifyMask);
}

// END - PWinObj interface.

//! @brief Kills the DockApp
void
DockApp::kill(void)
{
    XKillClient(_dpy, _dockapp_window);
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

    XMoveResizeWindow(_dpy, _window,
                      _gm.x, _gm.y, _gm.width, _gm.height);
    XMoveResizeWindow(_dpy, _dockapp_window,
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
    PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();

    pm->returnPixmap(_background);
    _background = pm->getPixmap(_gm.width, _gm.height, _scr->getDepth());

    _theme->getHarbourTexture()->render(_background, 0, 0, _gm.width, _gm.height);

    XSetWindowBackgroundPixmap(_dpy, _window, _background);
    XClearWindow(_dpy, _window);
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
    Config *cfg = Config::instance(); // convenience

    if (cfg->getHarbourDAMinSide() > 0) {
        if (_gm.width < static_cast<uint>(cfg->getHarbourDAMinSide())) {
            _gm.width = cfg->getHarbourDAMinSide();
        }
        if (_gm.height < static_cast<uint>(cfg->getHarbourDAMinSide())) {
            _gm.height = cfg->getHarbourDAMinSide();
        }
    }

    if (cfg->getHarbourDAMaxSide() > 0) {
        if (_gm.width > static_cast<uint>(cfg->getHarbourDAMaxSide())) {
            _gm.width = cfg->getHarbourDAMaxSide();
        }
        if (_gm.height > static_cast<uint>(cfg->getHarbourDAMaxSide())) {
            _gm.height = cfg->getHarbourDAMaxSide();
        }
    }
}

//! @brief Reads XClassHint of client.
void
DockApp::readClassHint(void)
{
    XClassHint x_class_hint;
    if (XGetClassHint(_dpy, _client_window, &x_class_hint)) {
        _class_hint.h_name = Util::to_wide_str(x_class_hint.res_name);
        _class_hint.h_class = Util::to_wide_str(x_class_hint.res_class);
        XFree(x_class_hint.res_name);
        XFree(x_class_hint.res_class);
    }
}

//! @brief Reads DockApp AutoProperties.
void
DockApp::readAutoProperties(void)
{
    DockAppProperty *prop =
        AutoProperties::instance()->findDockAppProperty(&_class_hint);
    if (prop) {
        _position = prop->getPosition();
    }
}

#endif // HARBOUR
