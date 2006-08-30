//
// Viewport.cc for pekwm
// Copyright (C)  2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "Viewport.hh"
#include "Atoms.hh"
#include "Config.hh"
#include "PScreen.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "ScreenResources.hh"
#include "Workspaces.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::list;

//! @brief Viewport constructor
Viewport::Viewport(uint ws, const std::list<PWinObj*> &wo_list) :
        _ws(ws), _wo_list(wo_list),
        _x(0), _y(0), _cols(1), _rows(1)
{
    _scr = PScreen::instance();

    // init viewport
    reload();
}

//! @brief Viewport destructor
Viewport::~Viewport(void)
{
}

//! @brief Moves the viewport to x y, optionally warping the pointer
//! @param x X position to move to
//! @param y Y position to move to
//! @param warp Wheter or not to warp the pointer, defaults to false
//! @param wx Mouse warp x position, defaults to 0
//! @param wy Mouse warp y position, defaults to 0
bool
Viewport::move(int x, int y, bool warp, int wx, int wy)
{
    // validate position
    if (x < 0)
        x = 0;
    else if (x > _x_max)
        x = _x_max;
    if (y < 0)
        y = 0;
    else if (y > _y_max)
        y = _y_max;

    // if the position has changed, let's move all frames
    if ((_x != x) || (_y != y)) {

        // calculate relative offset
        int d_x = _x - x;
        int d_y = _y - y;

        _x = x;
        _y = y;

        // Update _NET_DESKTOP_VIEWPORT
        hintSetViewport();

        if (warp == true)
            XWarpPointer(_scr->getDpy(), None, _scr->getRoot(), 0, 0, 0, 0, wx, wy);

        list<PWinObj*>::const_iterator it = _wo_list.begin();
        for (; it != _wo_list.end(); ++it) {
            if (((*it)->getWorkspace() == _ws) && ((*it)->isSticky() != true)) {
                (*it)->move((*it)->getX() + d_x, (*it)->getY() + d_y);
                // FIXME: This won't send any configure requests to the client,
                // should we overload the Frame::move or add a special case?
            }
        }

        return true;
    }

    return false;
}

//! @brief Moves the viewport in direction dir, optionally warping the pointer
//! @param dir Direction to move
//! @param warp Wheter or not to warp the pointer, defaults to true
bool
Viewport::moveDirection(DirectionType dir, bool warp)
{
    int n_x = _x, n_y = _y;
    int warp_x, warp_y;
    uint warp_d = Config::instance()->getScreenEdgeSize() * 2;
    if (warp_d == 0)
        warp_d = 2;

    _scr->getMousePosition(warp_x, warp_y);

    switch (dir) {
    case DIRECTION_UP:
        n_y -= _v_height;

        if (n_y < 0) {
            warp_y = _v_height + n_y;
        } else {
            warp_y = _v_height - warp_d;
        }
        break;
    case DIRECTION_DOWN:
        n_y += _v_height;

        if (n_y > _y_max) {
            warp_y = _v_height + _y - _y_max;
        } else {
            warp_y = warp_d;
        }
        break;
    case DIRECTION_LEFT:
        n_x -= _v_width;

        if (n_x < 0) {
            warp_x = _v_width + n_x;
        } else {
            warp_x = _v_width - warp_d;
        }
        break;
    case DIRECTION_RIGHT:
        n_x += _v_width;

        if (n_x > _x_max) {
            warp_x = _v_width + _x - _x_max;
        } else {
            warp_x = warp_d;
        }
        break;
    default:
        // Do nothing
        break;
    };

    return (move(n_x, n_y, warp, warp_x, warp_y));
}

//! @brief Initiates moving of the viewport starting at x y
void
Viewport::moveDrag(int x, int y)
{
    int status =
        _scr->grabPointer(_scr->getRoot(), ButtonMotionMask|ButtonReleaseMask,
                          ScreenResources::instance()->getCursor(ScreenResources::CURSOR_MOVE));

    if (status != true)
        return;

    // apply virtual desktop offset
    x -= _x;
    y -= _y;

    bool exit = false;

    XEvent ev;
    while (exit != true) {
        XMaskEvent(_scr->getDpy(), ButtonReleaseMask|ButtonMotionMask, &ev);

        switch (ev.type) {
        case MotionNotify:
            move(ev.xmotion.x_root - x, ev.xmotion.y_root - y);
            break;
        case ButtonRelease:
            exit = true;
            break;
        default:
            // Do nothing
            break;
        }
    }

    _scr->ungrabPointer();
}

//! @brief Moves the viewport to the column and row making the wo visible
//! @param wo PWinObj to make visible
bool
Viewport::moveToWO(PWinObj *wo)
{
    // make sure it is possible to move to the WO
    makeWOInsideVirtual(wo);

    return (move(getCol(wo) * _v_width, getRow(wo) * _v_height));
}

//! @brief Moves the viewport x_off and y_off pixels from current position
void
Viewport::scroll(int x_off, int y_off)
{
    move(_x + x_off, _y + y_off);
}

//! @brief Moves the viewport to column col and row row
void
Viewport::gotoColRow(uint col, uint row)
{
    move(col * _v_width, row * _v_height);
}

//! @brief Updates the viewport related hints
void
Viewport::hintsUpdate(void)
{
    hintSetViewport();
    hintSetGeometry();
    hintSetWorkarea();
}

//! @brief Updates the _NET_DESKTOP_VIEWPORT hint
void
Viewport::hintSetViewport(void)
{
    AtomUtil::setPosition(_scr->getRoot(),
                          EwmhAtoms::instance()->getAtom(NET_DESKTOP_VIEWPORT),
                          _x, _y);
}

//! @brief Updates the _NET_DESKTOP_GEOMETRY hint
void
Viewport::hintSetGeometry(void)
{
    AtomUtil::setPosition(_scr->getRoot(),
                          EwmhAtoms::instance()->getAtom(NET_DESKTOP_GEOMETRY),
                          _width, _height);
}

//! @brief Updates the _NET_WORKAREA hint
void
Viewport::hintSetWorkarea(void)
{
    int x, y;
    uint width, height;

    // FIXME: Expected behaviour?
    x = _x + _scr->getStrut()->left;
    y = _y + _scr->getStrut()->top;
    width = _v_width - x - _scr->getStrut()->right;
    height = _v_height - y - _scr->getStrut()->bottom;

    CARD32 workarea[] = { x, y, width, height };

    XChangeProperty(_scr->getDpy(), _scr->getRoot(),
                    EwmhAtoms::instance()->getAtom(NET_WORKAREA),
                    XA_CARDINAL, 32, PropModeReplace, (uchar *) workarea, 4);
}

#ifdef HAVE_XRANDR
//! @brief Refetches the root-window size and takes appropriate actions.
void
Viewport::updateGeometry(void)
{
    reload();
}
#endif // HAVE_XRANDR

//! @brief Reloads confiuration
void
Viewport::reload(void)
{
    _cols = Config::instance()->getViewportCols();
    _rows = Config::instance()->getViewportRows();

    // setup boundaries
    _v_width = _scr->getWidth();
    _v_height = _scr->getHeight();

    _width = _cols * _v_width;
    _height = _rows * _v_height;

    _x_max = _width - _v_width;
    _y_max = _height - _v_height;

    makeAllInsideVirtual();

    hintsUpdate(); // make sure workarea is up to date
}

//! @brief Makes sure all PWinObjs are inside the real screen
void
Viewport::makeAllInsideReal(void)
{
    list<PWinObj*>::const_iterator it = _wo_list.begin();
    for (; it != _wo_list.end(); ++it) {
        if (((*it)->getType() == PWinObj::WO_FRAME) &&
                ((*it)->getWorkspace() == _ws)) {

            makeWOInsideReal(*it);
        }
    }
}

//! @brief Make sure all PWinObjs are inside the virtual screen
void
Viewport::makeAllInsideVirtual(void)
{
    list<PWinObj*>::const_iterator it = _wo_list.begin();
    for (; it != _wo_list.end(); ++it) {
        if (((*it)->getType() == PWinObj::WO_FRAME) &&
                ((*it)->getWorkspace() == _ws)) {

            makeWOInsideVirtual(*it);
        }
    }
}

//! @brief Make sure wo is inside the real screen
//! @param wo PWinObj to make inside real screen
void
Viewport::makeWOInsideReal(PWinObj *wo)
{
    // NOTE: passing false to move, we don't want to corrupt
    //  _PEKWM_FRAME_VPOS hint with invalid position information

    // fix horizontal
    if (wo->getRX() < 0) {
        wo->move(0, wo->getY(), false);
    } else if (wo->getX() > signed(_v_width)) {
        wo->move(_v_width - wo->getWidth(), wo->getY(), false);
    }

    // fix vertical
    if (wo->getBY() < 0) {
        wo->move(wo->getX(), 0, false);
    } else if (wo->getY() > signed(_v_height)) {
        wo->move(wo->getX(), _v_height - wo->getHeight(), false);
    }
}

//! @brief Make sure wo is inside the virtual screen
//! @param wo PWinObj to make inside virtual screen
void
Viewport::makeWOInsideVirtual(PWinObj *wo)
{
    // fix horizontal
    if ((wo->getRX() + _x) < 0) {
        wo->move(0, wo->getY());
        wo->moveVirtual(wo->getX() + _x, wo->getY() + _y);
    } else if ((wo->getX() + _x) > signed(_width)) {
        wo->move(_width - _x - wo->getWidth(), wo->getY());
        wo->moveVirtual(wo->getX() + _x, wo->getY() + _y);
    }

    // fix vertical
    if ((wo->getBY() + _y) < 0) {
        wo->move(wo->getX(), 0);
        wo->moveVirtual(wo->getX() + _x, wo->getY() + _y);
    } else if ((wo->getY() + _y) > signed(_height)) {
        wo->move(wo->getX(), _height - _y - wo->getHeight());
        wo->moveVirtual(wo->getX() + _x, wo->getY() + _y);
    }
}
