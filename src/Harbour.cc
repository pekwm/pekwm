//
// Harbour.cc for pekwm
// Copyright (C) 2003-2020 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Harbour.hh"

#include "x11.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "DockApp.hh"
#include "Workspaces.hh"
#include "AutoProperties.hh"
#include "PDecor.hh"
#include "PMenu.hh"

#include <algorithm>
#include <functional>

using std::find;
#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

//! @brief Harbour constructor
Harbour::Harbour(void) :
        _hidden(false), _size(0), _strut(0),
        _last_button_x(0), _last_button_y(0)
{
    _strut = new Strut();
    X11::addStrut(_strut);
    _strut->head = Config::instance()->getHarbourHead();
}

//! @brief Harbour destructor
Harbour::~Harbour(void)
{
    removeAllDockApps();

    X11::removeStrut(_strut);
    delete _strut;
}

//! @brief Adds a DockApp to the Harbour
//! @todo Add sort option
void
Harbour::addDockApp(DockApp *da)
{
    if (! da) {
        return;
    }

    // add to the list
    if (AutoProperties::instance()->isHarbourSort()) {
        insertDockAppSorted(da);
        placeDockAppsSorted(); // place in sorted way
    } else {
        _dapps.push_back(da);
        placeDockApp(da); // place it in a empty space
    }

    da->setLayer(Config::instance()->isHarbourOntop() ? LAYER_DOCK : LAYER_DESKTOP);
    Workspaces::insert(da); // add the dockapp to the stacking list

    if (! da->isMapped()) { // make sure it's visible
        da->mapWindow();
    }

    da->setOpacity(Config::instance()->getHarbourOpacity());
    updateHarbourSize();
}

//! @brief Removes a DockApp from the Harbour
void
Harbour::removeDockApp(DockApp *da)
{
    if (! da)
        return;

    vector<DockApp*>::iterator it(find(_dapps.begin(), _dapps.end(), da));

    if (it != _dapps.end()) {
        _dapps.erase(it);
        Workspaces::remove(da); // remove the dockapp to the stacking list
        delete da;

        if (AutoProperties::instance()->isHarbourSort()) {
            placeDockAppsSorted();
        }
    }

    updateHarbourSize();
}

//! @brief Removes all DockApps from the Harbour
void
Harbour::removeAllDockApps(void)
{
    vector<DockApp*>::const_iterator it(_dapps.begin());
    for (; it != _dapps.end(); ++it) {
        Workspaces::remove(*it); // remove the dockapp to the stacking list
        delete (*it);
    }
    _dapps.clear();
}

//! @brief Tries to find a dockapp which uses the window win
DockApp*
Harbour::findDockApp(Window win)
{
    DockApp *dockapp = 0;

    vector<DockApp*>::const_iterator it(_dapps.begin());
    for (; it != _dapps.end(); ++it) {
        if ((*it)->findDockApp(win)) {
            dockapp = (*it);
            break;
        }
    }

    return dockapp;
}

//! @brief Tries to find a dockapp which has the window win as frame.
DockApp*
Harbour::findDockAppFromFrame(Window win)
{
    DockApp *dockapp = 0;

    vector<DockApp*>::const_iterator it(_dapps.begin());
    for (; it != _dapps.end(); ++it) {
        if ((*it)->findDockAppFromFrame(win)) {
            dockapp = (*it);
            break;
        }
    }

    return dockapp;
}

#ifdef HAVE_XRANDR
//! @brief Refetches the root-window size and takes appropriate actions.
void
Harbour::updateGeometry(void)
{
    vector<DockApp*>::const_iterator it(_dapps.begin());
    for (; it != _dapps.end(); ++it) {
        placeDockAppInsideScreen(*it);
    }
}
#endif // HAVE_XRANDR

//! @brief Lowers or Raises all the DockApps in the harbour.
void
Harbour::restack(void)
{
    X11::removeStrut(_strut);
    if (Config::instance()->isHarbourOntop() ||
            ! Config::instance()->isHarbourMaximizeOver()) {

        X11::addStrut(_strut);
    }

    Layer l = Config::instance()->isHarbourOntop() ? LAYER_DOCK : LAYER_DESKTOP;

    vector<DockApp*>::const_iterator it(_dapps.begin());
    for (; it != _dapps.end(); ++it) {
        (*it)->setLayer(l);

        if (Config::instance()->isHarbourOntop()) {
            Workspaces::raise(*it);
        } else {
            Workspaces::lower(*it);
        }
    }
}

//! @brief Goes through the DockApp list and places the dockapp.
void
Harbour::rearrange(void)
{
    _strut->head = Config::instance()->getHarbourHead();

    if (AutoProperties::instance()->isHarbourSort()) {
        placeDockAppsSorted();
    } else {
        vector<DockApp*>::const_iterator it(_dapps.begin());
        for (; it != _dapps.end(); ++it)
            placeDockApp(*it);
    }
}

//! @brief Repaints all dockapps with the new theme
void
Harbour::loadTheme(void)
{
    for_each(_dapps.begin(), _dapps.end(), std::mem_fn(&DockApp::loadTheme));
}

//! @brief Updates the harbour max size variable.
void
Harbour::updateHarbourSize(void)
{
    _size = 0;

    vector<DockApp*>::const_iterator it(_dapps.begin());
    for (; it != _dapps.end(); ++it)
    {
        switch (Config::instance()->getHarbourPlacement())
        {
        case TOP:
        case BOTTOM:
            if ((*it)->getHeight() > _size)
                _size = (*it)->getHeight();
            break;
        case LEFT:
        case RIGHT:
            if ((*it)->getWidth() > _size)
                _size = (*it)->getWidth();
            break;
        default:
            // Do nothing.
            break;
        }
    }

    updateStrutSize();
}

//! @brief Sets the Hidden state of the harbour
//! @param sa StateAction specifying state to set.
void
Harbour::setStateHidden(StateAction sa)
{
    // Check if there is anything to do
    if (! ActionUtil::needToggle(sa, _hidden)) {
        return;
    }

    if (_hidden) {
        // Show if currently hidden.
        for_each(_dapps.begin(), _dapps.end(),
                 std::mem_fn(&DockApp::mapWindow));
    } else {
        // Hide if currently visible.
        for_each(_dapps.begin(), _dapps.end(),
                 std::mem_fn(&DockApp::unmapWindow));
    }

    _hidden = !_hidden;
    updateStrutSize();
}

//! @brief Updates Harbour strut size.
void
Harbour::updateStrutSize(void)
{
    _strut->left = _strut->right = _strut->top = _strut->bottom = 0;

    if (! Config::instance()->isHarbourMaximizeOver() && ! _hidden) {
        switch (Config::instance()->getHarbourPlacement()) {
        case TOP:
            _strut->top = _size;
            break;
        case BOTTOM:
            _strut->bottom = _size;
            break;
        case LEFT:
            _strut->left = _size;
            break;
        case RIGHT:
            _strut->right = _size;
            break;
        }
    }

    X11::updateStrut();
}

//! @brief Handles XButtonEvents made on the DockApp's frames.
void
Harbour::handleButtonEvent(XButtonEvent* ev, DockApp* da)
{
    if (! da) {
        return;
    }
    
    _last_button_x = ev->x;
    _last_button_y = ev->y;
}

//! @brief Initiates moving of a DockApp based on info from a XMotionEvent.
void
Harbour::handleMotionNotifyEvent(XMotionEvent* ev, DockApp* da)
{
    if (! da) {
        return;
    }
    
    Geometry head;
    int x = 0, y = 0;

    X11::getHeadInfo(Config::instance()->getHarbourHead(), head);

    switch(Config::instance()->getHarbourPlacement()) {
    case TOP:
    case BOTTOM:
        x = ev->x_root - _last_button_x;
        y = da->getY();
        if (x < head.x) {
            x = head.x;
        } else if ((x + da->getWidth()) > (head.x + head.width)) {
            x = head.x + head.width - da->getWidth();
        }
        break;
    case LEFT:
    case RIGHT:
        x = da->getX();
        y = ev->y_root - _last_button_y;
        if (y < head.y) {
            y = head.y;
        } else if ((y + da->getHeight()) > (head.y + head.height)) {
            y = head.y + head.height - da->getHeight();
        }
        break;
    default:
        // do nothing
        break;
    }

    da->move(x, y);
}

//! @brief Handles XConfigureRequestEvents.
void
Harbour::handleConfigureRequestEvent(XConfigureRequestEvent* ev, DockApp* da)
{
    if (! da) {
        return;
    }
    
    vector<DockApp*>::const_iterator it(find(_dapps.begin(), _dapps.end(), da));

    if (it != _dapps.end()) {
        // Thing is that we doesn't listen to border width, position or
        // stackign so the only thing that we'll alter is size if that's
        // what we want to configure

        uint width = (ev->value_mask&CWWidth) ? ev->width : da->getClientWidth();
        uint height = (ev->value_mask&CWHeight) ? ev->height : da->getClientHeight();

        da->resize(width, height);

        placeDockAppInsideScreen(da);
    }
}

//! @brief Tries to find a empty spot for the DockApp
void
Harbour::placeDockApp(DockApp *da)
{
    if (! da || ! _dapps.size())
        return;

    bool right = (Config::instance()->getHarbourOrientation() == BOTTOM_TO_TOP);

    int test, x = 0, y = 0;
    bool placed = false, increase = false, x_place = false;

    Geometry head;
    X11::getHeadInfo(Config::instance()->getHarbourHead(), head);

    getPlaceStartPosition (da, x, y, x_place);
    if (right) {
        if (x_place) {
            x -= da->getWidth ();
        } else {
            y -= da->getHeight ();
        }
    }

    vector<DockApp*>::const_iterator it;
    if (x_place) {
        x = test = right ? head.x + head.width - da->getWidth() : head.x;

        while (! placed
                && (right
                    ? (test >= 0)
                    : ((test + da->getWidth()) < (head.x + head.width))))
        {
            placed = increase = true;

            it = _dapps.begin();
            for (; placed && it != _dapps.end(); ++it) {
                if ((*it) == da) {
                    continue; // exclude ourselves
                }
                
                if (((*it)->getX() < static_cast<signed>(test + da->getWidth())) &&
                        ((*it)->getRX() > test)) {
                    placed = increase = false;
                    test = right ? (*it)->getX() - da->getWidth() : (*it)->getRX();
                }
            }

            if (placed) {
                x = test;
            } else if (increase) {
                test += right ? -1 : 1;
            }
        }
    } else { // !x_place
        y = test = right ? head.y + head.height - da->getHeight() : head.y;

        while (! placed
                && (right
                    ? (test >= 0)
                    : ((test + da->getHeight()) < (head.y + head.height))))
        {
            placed = increase = true;

            it = _dapps.begin();
            for (; placed && it != _dapps.end(); ++it) {
                if ((*it) == da) {
                    continue; // exclude ourselves
                }
                
                if (((*it)->getY() < static_cast<signed>(test + da->getHeight())) &&
                        ((*it)->getBY() > test)) {
                    placed = increase = false;
                    test = right ? (*it)->getY() - da->getHeight() : (*it)->getBY();
                }
            }

            if (placed) {
                y = test;
            } else if (increase) {
                test += right ? -1 : 1;
            }
        }
    }

    da->move(x, y);
}


//! @brief Inserts DockApp and places all dockapps in sorted order
//! @todo Screen boundary checking
void
Harbour::placeDockAppsSorted(void)
{
    if (! _dapps.size()) {
        return;
    }
    
    // place the dockapps
    int x, y, x_real, y_real;
    bool inc_x = false;
    bool right = (Config::instance()->getHarbourOrientation() == BOTTOM_TO_TOP);

    getPlaceStartPosition(_dapps.front (), x_real, y_real, inc_x);

    vector<DockApp*>::const_iterator it(_dapps.begin());
    for (; it != _dapps.end(); ++it) {
        getPlaceStartPosition(*it, x, y, inc_x);

        if (inc_x) {
            if (right) {
                (*it)->move(x_real - (*it)->getWidth(), y);
                x_real -= -(*it)->getWidth();
            } else {
                (*it)->move(x_real, y);
                x_real += (*it)->getWidth();
            }
        } else {
            if (right) {
                (*it)->move(x, y_real - (*it)->getHeight());
                y_real -= (*it)->getHeight();
            } else {
                (*it)->move(x, y_real);
                y_real += (*it)->getHeight();
            }
        }
    }
}

/**
 * Make sure dock app is inside screen boundaries and placed on the
 * right edge, usually called after resizing the dockapp.
 */
void
Harbour::placeDockAppInsideScreen(DockApp *da)
{
    Geometry head;
    X11::getHeadInfo(Config::instance()->getHarbourHead(), head);
    uint pos = Config::instance()->getHarbourPlacement();

    // top or bottom placement
    if ((pos == TOP) || (pos == BOTTOM)) {
        // check horizontal position
        if (da->getX() < head.x) {
            da->move(head.x, da->getY());
        } else if (da->getRX() > static_cast<signed>(head.x + head.width)) {
            da->move(head.x + head.width - da->getWidth(), da->getY());
        }

        // check vertical position
        if ((pos == TOP) && (da->getY() != head.y)) {
            da->move(da->getX(), head.y);
        } else if ((pos == BOTTOM) && (da->getBY() != static_cast<signed>(head.y + head.height))) {
            da->move(da->getX(), head.y + head.height - da->getHeight());
        }

        // left or right placement
    } else {
        // check vertical position
        if (da->getY() < head.y) {
            da->move(da->getX(), head.y);
        } else if (da->getBY() > static_cast<signed>(head.y + head.height)) {
            da->move(da->getX(), head.y + head.height - da->getHeight());
        }

        // check horizontal position
        if ((pos == LEFT) && (da->getX() != head.x)) {
            da->move(head.x, da->getY());
        } else if ((pos == RIGHT) && (da->getRX() != static_cast<signed>(head.x + head.width))) {
            da->move(head.x + head.width - da->getWidth(), da->getY());
        }
    }
}

void
Harbour::getPlaceStartPosition(DockApp *da, int &x, int &y, bool &inc_x)
{
    if (! da) {
        return;
    }
    
    Geometry head;
    X11::getHeadInfo(Config::instance()->getHarbourHead(), head);
    bool right = (Config::instance()->getHarbourOrientation() == BOTTOM_TO_TOP);

    switch (Config::instance()->getHarbourPlacement()) {
    case TOP:
        inc_x = true;
        x = right ? head.x + head.width : head.x;
        y = head.y;
        break;
    case BOTTOM:
        inc_x = true;
        x = right ? head.x + head.width : head.x;
        y = head.y + head.height - da->getHeight();
        break;
    case LEFT:
        x = head.x;
        y = right ? head.y + head.height : head.y;
        break;
    case RIGHT:
        x = head.x + head.width - da->getWidth();
        y = right ? head.y + head.height : head.y;
        break;
    }
}


//! @brief Inserts DockApp in sorted order in the list
void
Harbour::insertDockAppSorted(DockApp *da)
{
    vector<DockApp*>::iterator it(_dapps.begin());

    // The order of this list doesn't make much sense to me when
    // it comes to representing it in code, however it's perfectly sane
    // for representing the order in the config files.
    // anyway, order goes as follows: 1 2 3 0 0 0 -3 -2 -1

    // Middle of the list.
    if (da->getPosition() == 0) {
        for (; it != _dapps.end() && (*it)->getPosition() >= 0; ++it)
            ;
        // Beginning of the list.
    } else if (da->getPosition() > 0) {
        for (; it != _dapps.end(); ++it) {
            if (((*it)->getPosition() < 1) || // got to 0 or less
                    (da->getPosition() <= (*it)->getPosition())) {
                break;
            }
        }
        // end of the list
    } else {
        for (; it != _dapps.end() && (*it)->getPosition() >= 0; ++it)
            ;
        for (; it != _dapps.end() && da->getPosition() >= (*it)->getPosition(); ++it)
            ;
    }

    _dapps.insert(it, da);
}

