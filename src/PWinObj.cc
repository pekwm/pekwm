//
// PWinObj.cc for pekwm
// Copyright © 2003-2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <algorithm>
#include <iostream>

#include "PWinObj.hh"

#ifdef OPACITY
#include "Atoms.hh"
#endif // OPACITY

using std::cerr;
using std::endl;
using std::find;
using std::vector;
using std::map;

Display *PWinObj::_dpy;
PWinObj* PWinObj::_focused_wo = (PWinObj*) 0;
PWinObj* PWinObj::_root_wo = (PWinObj*) 0;
vector<PWinObj*> PWinObj::_wo_list = vector<PWinObj*>();
map<Window, PWinObj*> PWinObj::_wo_map = map<Window, PWinObj*>();

//! @brief PWinObj constructor.
PWinObj::PWinObj()
    : _window(None),
      _parent(0), _type(WO_NO_TYPE),
      _workspace(0), _layer(LAYER_NORMAL),
      _mapped(false), _iconified(false), _hidden(false),
      _focused(false), _sticky(false),
      _focusable(true)
#ifdef OPACITY
     ,_opaque(true)
#endif // OPACITY
{
}

//! @brief PWinObj destructor.
PWinObj::~PWinObj(void)
{
    if (_focused_wo == this) {
        _focused_wo = 0;
    }
    notifyObservers(0);
}

//! @brief Associates Window with PWinObj
void
PWinObj::addChildWindow(Window win)
{
    _wo_map[win] = this;
}

//! @brief Removes association of Window with PWinObj
void
PWinObj::removeChildWindow(Window win)
{
    map<Window, PWinObj*>::iterator it(_wo_map.find(win));
    if (it != _wo_map.end()) {
        _wo_map.erase(it);
    }
}

#ifdef OPACITY
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
    uint opacity;
    if (_opaque) {
        opacity = EWMH_OPAQUE_WINDOW;
    } else {
        opacity = isFocused()?_opacity.focused:_opacity.unfocused;
    }

    if (_opacity.current != opacity) {
        _opacity.current = opacity;
        AtomUtil::setLong(_window, Atoms::getAtom(NET_WM_WINDOW_OPACITY), opacity);
    }
}
#endif // OPACITY

//! @brief Maps the window and sets _mapped to true.
void
PWinObj::mapWindow(void)
{
    if (_mapped) {
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
    if (_mapped) {
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
    if (! _mapped) {
        return;
    }
    
    _mapped = false;

    // Make sure unmapped windows drops focus
    setFocused(false);

    XUnmapWindow(_dpy, _window);
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

//! @brief Toggles _sticky
void
PWinObj::stick(void)
{
    _sticky = !_sticky;
}

//! @brief Moves the window and updates _gm.
//! @param x X position
//! @param y Y position
void
PWinObj::move(int x, int y)
{
    _gm.x = x;
    _gm.y = y;

    XMoveWindow(_dpy, _window, _gm.x, _gm.y);
}

//! @brief Resizes the window and updates _gm.
void
PWinObj::resize(uint width, uint height)
{
    if (! width || ! height) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PWinObj(" << this << ")::resize(" << width << "," << height
             << ")" << endl << " *** invalid geometry, _window = "
             << _window << endl;
#endif // DEBUG
        return;
    }

    _gm.width = width;
    _gm.height = height;

    XResizeWindow(_dpy, _window, _gm.width, _gm.height);
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
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PWinObj(" << this << ")::moveResize(" << width << "," << height
             << ")" << endl << " *** invalid geometry, _window = "
             << _window << endl;
#endif // DEBUG
        return;
    }

    _gm.x = x;
    _gm.y = y;
    _gm.width = width;
    _gm.height = height;

    XMoveResizeWindow(_dpy, _window, _gm.x, _gm.y, _gm.width, _gm.height);
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

//! @brief Sets _focused to focused and updates opacity as needed.
void
PWinObj::setFocused(bool focused)
{
    _focused = focused;
#ifdef OPACITY
    updateOpacity();
#endif // OPACITY
}

//! @brief Only sets _sticky to sticky.
void
PWinObj::setSticky(bool sticky)
{
    _sticky = sticky;
}

#ifdef OPACITY
//! @brief Updates opaque state
void
PWinObj::setOpaque(bool opaque)
{
    _opaque = opaque;
    updateOpacity();
}
#endif // OPACITY

//! @brief Only sets _hidden to hidden.
void
PWinObj::setHidden(bool hidden)
{
    _hidden = hidden;
}

//! @brief Executes XSetInputFocus on the appropriate window.
void
PWinObj::giveInputFocus(void)
{
    if (! _mapped  || ! _focusable) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PWinObj(" << this << ")::giveInputFocus()" << endl
             << " *** non focusable window."
             << " mapped: " << _mapped  << " focusable: " << _focusable << endl;
#endif // DEBUG
        return;
    }

    XSetInputFocus(_dpy, _window, RevertToPointerRoot, CurrentTime);
}

//! @brief Reparents and sets _parent member
void
PWinObj::reparent(PWinObj *wo, int x, int y)
{
    _parent = wo;
    XReparentWindow(_dpy, _window, wo->getWindow(), x, y);
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
    vector<PWinObj*>::iterator it(find(_wo_list.begin(), _wo_list.end(), wo));
    if (it != _wo_list.end()) {
        _wo_list.erase(it);
    }
}
