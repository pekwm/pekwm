//
// Workspaces.cc for pekwm
// Copyright © 2002-2009 Claes Nasten <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Debug.hh"
#include "Workspaces.hh"
#include "ParseUtil.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "Frame.hh"
#include "Client.hh" // For isSkip()
#include "WindowManager.hh"
#include "WinLayouter.hh"
#include "WorkspaceIndicator.hh"
#include "x11.hh"

#include <iostream>
#include <sstream>
#ifdef HAVE_LIMITS
#include <limits>
using std::numeric_limits;
#endif // HAVE_LIMITS

extern "C" {
#include <X11/Xatom.h> // for XA_WINDOW
}

using std::string;
using std::find;
using std::wostringstream;
using std::wstring;
using std::cerr;
using std::endl;

// Workspace

Workspace::~Workspace(void)
{
    delete _layouter;
}

Workspace &Workspace::operator=(const Workspace &w)
{
    _name = w._name;
    delete _layouter;
    _layouter = w._layouter;
    w._layouter = 0;
    _last_focused = w._last_focused;
    return *this;
}

void
Workspace::setLayouter(WinLayouter *wl)
{
    delete _layouter;
    _layouter = wl;
}

void
Workspace::setDefaultLayouter(const std::string &layo)
{
    WinLayouter *wl = WinLayouterFactory(layo);
    if (wl) {
        delete _default_layouter;
        _default_layouter = wl;
    }
}

// Workspaces

uint Workspaces::_active;
uint Workspaces::_previous;
uint Workspaces::_per_row;
vector<PWinObj*> Workspaces::_wobjs;
vector<Workspace> Workspaces::_workspaces;
WinLayouter *Workspace::_default_layouter = WinLayouterFactory("SMART");

//! @brief Sets total amount of workspaces to number
void
Workspaces::setSize(uint number)
{
    if (number < 1) {
        number = 1;
    }

    if (number == _workspaces.size()) {
        return; // no need to change number of workspaces to the current number
    }

    uint before = _workspaces.size();
    if (_active >= number) {
        _active = number - 1;
    }
    if (_previous >= number) {
        _previous = number - 1;
    }

    _workspaces.resize(number);

    for (uint i = before; i < number; ++i) {
        _workspaces[i].setName(getWorkspaceName(i));
    }

    // Tell the rest of the world how many workspaces we have.
    XChangeProperty(X11::getDpy(),
                    X11::getRoot(),
                    X11::getAtom(NET_NUMBER_OF_DESKTOPS),
                    XA_CARDINAL, 32, PropModeReplace,
                    (uchar *) &number, 1);

    // make sure we aren't on an non-existent workspace
    if (number <= _active) {
        setWorkspace(number - 1, true);
    }
}

/**
 * Set workspace names.
 */
void
Workspaces::setNames(void)
{
    vector<Workspace>::size_type i=0, size=_workspaces.size();
    for (; i<size; ++i) {
        _workspaces[i].setName(Config::instance()->getWorkspaceName(i));
    }
}

//! @brief Activates Workspace workspace and sets the right hints
//! @param num Workspace to activate
//! @param focus whether or not to focus a window after switch
void
Workspaces::setWorkspace(uint num, bool focus)
{
    if (num == _active || num >= _workspaces.size()) {
        return;
    }

    X11::grabServer();

    PWinObj *wo = PWinObj::getFocusedPWinObj();
    // Make sure that sticky windows gets unfocused on workspace change,
    // it will be set back after switch is done.
    if (wo) {
        if (wo->getType() == PWinObj::WO_CLIENT) {
            wo->getParent()->setFocused(false);
        } else {
            wo->setFocused(false);
        }
    }

    // Save the focused window object
    setLastFocused(_active, wo);
    PWinObj::setFocusedPWinObj(0);

    // switch workspace
    hideAll(_active);
    X11::setLong(X11::getRoot(), NET_CURRENT_DESKTOP, num);

    _previous = _active;
    _active = num;

    unhideAll(num, focus);

    X11::ungrabServer(true);

    // Show workspace indicator if requested
    if (Config::instance()->getShowWorkspaceIndicator() > 0) {
        WindowManager::instance()->getWorkspaceIndicator()->render();
        WindowManager::instance()->getWorkspaceIndicator()->mapWindowRaised();
        WindowManager::instance()->getWorkspaceIndicator()->updateHideTimer(Config::instance()->getShowWorkspaceIndicator());
    }
}

//! @brief
bool
Workspaces::gotoWorkspace(uint direction, bool warp)
{
    uint workspace;
    int dir = 0;
    // Using a bool flag to detect changes due to special workspaces such
    // as PREV
    bool switched = true;
    uint per_row = Config::instance()->getWorkspacesPerRow();

    uint cur_row = getRow(), row_min = getRowMin(), row_max = getRowMax();
    switch (direction) {
    case WORKSPACE_LEFT:
    case WORKSPACE_PREV:
        dir = 1;

        if (_active > row_min) {
          workspace = _active - 1;
        } else if (direction == WORKSPACE_PREV) {
          workspace = row_max;
        } else {
          switched = false;
        }
        break;
    case WORKSPACE_NEXT:
    case WORKSPACE_RIGHT:
        dir = 2;

        if (_active < row_max) {
            workspace = _active + 1;
        } else if (direction == WORKSPACE_NEXT) {
            workspace = row_min;
        } else {
            switched = false;
        }
        break;
    case WORKSPACE_PREV_V:
    case WORKSPACE_UP:
      dir = -1;

      if (_active >= per_row) {
        workspace = _active - per_row;
      } else if (direction == WORKSPACE_PREV_V) {
        // Bottom left
        workspace = _workspaces.size() - per_row;
        // Add column
        workspace += _active - cur_row * per_row;
      } else {
        switched = false;
      }
      break;
    case WORKSPACE_NEXT_V:
    case WORKSPACE_DOWN:
        dir = -2;

        if (_active + per_row < _workspaces.size()) {
            workspace = _active + per_row;
        } else if (direction == WORKSPACE_NEXT_V) {
            workspace = _active - cur_row * per_row;
        } else {
            switched = false;
        }
        break;
    case WORKSPACE_LAST:
        workspace = _previous;
        if (_active == workspace) {
            switched = false;
        }
        break;
    default:
        if (direction == _active) {
            switched = false;
        } else {
            workspace = direction;
        }
    }

    if (switched) {
        if (warp) {
            warpToWorkspace(workspace, dir);
        } else {
            setWorkspace(workspace, true);
        }
    }

    return switched;
}

//! @brief
bool
Workspaces::warpToWorkspace(uint num, int dir)
{
    if (num == _active || num >= _workspaces.size()) {
        return false;
    }

    int x, y;
    X11::getMousePosition(x, y);

    if (dir != 0) {
      switch(dir) {
      case 1:
        x = X11::getWidth() - std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_LEFT) + 2, 2);
        break;
      case 2:
        x = std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_RIGHT) * 2, 2);
        break;
      case -1:
        y = X11::getHeight() - std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_BOTTOM) + 2, 2);
        break;
      case -2:
        y = std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_TOP) + 2, 2);
        break;
      }

        // warp pointer
        XWarpPointer(X11::getDpy(), None,
                     X11::getRoot(), 0, 0, 0, 0, x, y);
    }

    // set workpsace
    setWorkspace(num, true);

    return true;
}

void
Workspaces::layoutOnce(const std::string &layouter)
{
    if (WinLayouter *wl = WinLayouterFactory(layouter)) {
        wl->layout(0, false);
        delete wl;
    }
}

void
Workspaces::setLayouter(uint ws, const std::string &layouter)
{
    if (ws >= _workspaces.size()) {
        _workspaces.resize(ws+1);
    }
    bool tiling = _workspaces[ws].getLayouter()->isTiling();
    _workspaces[ws].setLayouter(WinLayouterFactory(layouter));
    if (ws == _active) {
        bool newTiling = _workspaces[ws].getLayouter()->isTiling();
        if (newTiling != tiling) {
            Frame *frame;
            const_iterator it = _wobjs.begin();
            const_iterator end = _wobjs.end();
            for (; it!=end; ++it) {
                if ((frame = dynamic_cast<Frame*>(*it)) && frame->isMapped()) {
                    frame->updateDecor();
                }
            }
        }
        layoutIfTiling();
    }
}

/**
 * Adds a PWinObj to the stacking list. Assumes that wo is not in _wobjs already.
 * @param wo PWinObj to insert
 * @param raise Whether to insert at the bottom or top of the layer (defaults to true).
 */
void
Workspaces::insert(PWinObj *wo, bool raise)
{
    PWinObj *top_obj = 0;
    Frame *frame, *wo_frame = dynamic_cast<Frame*>(wo);
    iterator it;

    if (! raise && wo_frame && wo_frame->getTransFor()
                && wo_frame->getTransFor()->getLayer() == wo_frame->getLayer()) {
        // Lower only to the top of the transient_for window.
        it = ++find(_wobjs.begin(), _wobjs.end(), wo_frame->getTransFor()->getParent());
        top_obj = it!=_wobjs.end()?*it:0; // I think it==_wobjs.end() can't happen
    } else {
        it = _wobjs.begin();
        for (; it != _wobjs.end(); ++it) {
            if (raise) {
                // If raising, make sure the inserted wo gets below the first
                // window in the next layer.
                if ((*it)->getLayer() > wo->getLayer()) {
                    top_obj = *it;
                    break;
                }
            } else if (wo->getLayer() <= (*it)->getLayer()) {
                // If lowering, put the window below the first window with the same level.
                top_obj = *it;
                break;
            }
        }
    }

    _wobjs.insert(it, wo);

    vector<PWinObj*> winstack;
    winstack.reserve(3);
    winstack.push_back(wo);

    if (wo_frame && wo_frame->hasTrans()) {
        vector<Client*>::const_iterator t_it;
        for (it = _wobjs.begin(); *it != wo;) {
            if ((frame = dynamic_cast<Frame*>(*it))) {
                t_it = find(wo_frame->getTransBegin(), wo_frame->getTransEnd(),
                            frame->getActiveClient());
                if (t_it != wo_frame->getTransEnd()) {
                    winstack.push_back(frame);
                    it = _wobjs.erase(it);
                    continue;
                }
            }
            ++it;
        }

        it = ++find(_wobjs.begin(), _wobjs.end(), wo);
        _wobjs.insert(it, winstack.begin()+1, winstack.end());
    }

    if (top_obj) {
        winstack.push_back(top_obj);
    } else {
        XRaiseWindow(X11::getDpy(), winstack.back()->getWindow());
    }

    const unsigned size = winstack.size();
    Window *wins = new Window[size];
    vector<PWinObj*>::const_iterator wit(winstack.end());
    for (unsigned i=0; i<size; ++i) {
        wins[i] = (*--wit)->getWindow();
    }
    X11::stackWindows(wins, size);
    delete [] wins;
}

//! @brief Removes a PWinObj from the stacking list.
void
Workspaces::remove(PWinObj* wo)
{
    iterator it_wo(_wobjs.begin());
    for (;it_wo != _wobjs.end();) {
        if (wo == *it_wo) {
            it_wo = _wobjs.erase(it_wo);
        } else {
            ++it_wo;
        }
    }

    // remove from last focused
    vector<Workspace>::iterator it(_workspaces.begin());
    for (; it != _workspaces.end(); ++it) {
        if (wo == (*it).getLastFocused()) {
            (*it).setLastFocused(0);
        }
    }
}

//! @brief Hides all non-sticky Frames on the workspace.
void
Workspaces::hideAll(uint workspace)
{
    const_iterator it(_wobjs.begin());
    for (; it != _wobjs.end(); ++it) {
        if (! ((*it)->isSticky()) && ! ((*it)->isHidden()) &&
                ((*it)->getWorkspace() == workspace)) {
            (*it)->unmapWindow();
        }
    }
}

//! @brief Unhides all hidden PWinObjs on the workspace.
void
Workspaces::unhideAll(uint workspace, bool focus)
{
    const_iterator it(_wobjs.begin());
    for (; it != _wobjs.end(); ++it) {
        if (! (*it)->isMapped() && ! (*it)->isIconified() && ! (*it)->isHidden()
                && ((*it)->getWorkspace() == workspace)) {
            (*it)->mapWindow(); // don't restack ontop windows
            if ((*it)->getType() == PWinObj::WO_FRAME) {
                static_cast<Frame*>(*it)->updateDecor();
            }
        }
    }

    // Try to focus last focused window and if that fails we get the top-most
    // Frame if any and give it focus.
    if (focus) {
        PWinObj *wo = _workspaces[workspace].getLastFocused();
        if (! wo || ! PWinObj::windowObjectExists(wo)) {
            wo = getTopWO(PWinObj::WO_FRAME);
        }

        if (wo && wo->isMapped() && wo->isFocusable()) {
            // Render as focused
            if (wo->getType() == PWinObj::WO_CLIENT) {
                wo->getParent()->setFocused(true);
            } else {
                wo->setFocused(true);
            }

            // Get the active child if a frame, to get correct focus behavior
            if (wo->getType() == PWinObj::WO_FRAME) {
                wo = static_cast<Frame*>(wo)->getActiveChild();
            }

            // Focus
            wo->giveInputFocus();
            PWinObj::setFocusedPWinObj(wo);

            // update the MRU list
            if (wo->getType() == PWinObj::WO_CLIENT) {
                Frame *frame = static_cast<Frame*>(wo->getParent());
                WindowManager::instance()->addToMRUFront(frame);
            }
        }

        // If focusing fails, focus the root window.
        if (! PWinObj::getFocusedPWinObj()
                || ! PWinObj::getFocusedPWinObj()->isMapped()) {
            PWinObj::getRootPWinObj()->giveInputFocus();
        }
    }
}

//! @brief Raises a PWinObj and restacks windows.
void
Workspaces::raise(PWinObj* wo)
{
    iterator it(find(_wobjs.begin(), _wobjs.end(), wo));

    if (it == _wobjs.end()) { // no Frame to raise.
        return;
    }
    _wobjs.erase(it);

    insert(wo, true); // reposition and restack
}

//! @brief Lower a PWinObj and restacks windows.
void
Workspaces::lower(PWinObj* wo)
{
    iterator it(find(_wobjs.begin(), _wobjs.end(), wo));

    if (it == _wobjs.end()) // no Frame to raise.
        return;
    _wobjs.erase(it);

    insert(wo, false); // reposition and restack
}

//! @brief
PWinObj*
Workspaces::getLastFocused(uint workspace)
{
    if (workspace >= _workspaces.size()) {
        return 0;
    }
    return _workspaces[workspace].getLastFocused();
}

//! @brief
void
Workspaces::setLastFocused(uint workspace, PWinObj* wo)
{
    if (workspace >= _workspaces.size()) {
        return;
    }
    
    _workspaces[workspace].setLastFocused(wo);
}

//! @brief Create name for workspace num
wstring
Workspaces::getWorkspaceName(uint num)
{
    wostringstream buf;
    buf << num + 1;
    buf << L": ";
    buf << Config::instance()->getWorkspaceName(num);
    
    return buf.str();
}

// MISC METHODS

//! @brief Returns the first focusable PWinObj with the highest stacking
PWinObj*
Workspaces::getTopWO(uint type_mask)
{
    const_reverse_iterator r_it = _wobjs.rbegin();
    for (; r_it != _wobjs.rend(); ++r_it) {
        if ((*r_it)->isMapped()
                && (*r_it)->isFocusable()
                && ((*r_it)->getType()&type_mask)) {
            return (*r_it);
        }
    }
    return 0;
}

/**
 * Builds a list of all clients in stacking order, clients in the same
 * frame come after each other.
 */
Window*
Workspaces::buildClientList(unsigned int &num_windows)
{
    Frame *frame;
    Client *client, *client_active;

    vector<Window> windows;
    iterator it_f;
    const_iterator it_c;
    for (it_f = _wobjs.begin(); it_f != _wobjs.end(); ++it_f) {
        if ((*it_f)->getType() != PWinObj::WO_FRAME) {
            continue;
        }

        frame = static_cast<Frame*>(*it_f);
        client_active = frame->getActiveClient();

        if (Config::instance()->isReportAllClients()) {
            for (it_c = frame->begin(); it_c != frame->end(); ++it_c) {
                client = dynamic_cast<Client*>(*it_c);
                if (client && ! client->isSkip(SKIP_TASKBAR)
                    && client != client_active) {
                    windows.push_back(client->getWindow());
                }
            }
        }

        if (client_active && ! client_active->isSkip(SKIP_TASKBAR)) {
            windows.push_back(client_active->getWindow());
        }
    }

    num_windows = windows.size();
    Window *wins = new Window[num_windows ? num_windows : 1];
    if (num_windows > 0) {
        copy(windows.begin(), windows.end(), wins);
    }

    return wins;
}

/**
 * Updates the Ewmh Client list hint.
 */
void
Workspaces::updateClientList(void)
{
    unsigned int num_windows;
    Window *windows = buildClientList(num_windows);

    X11::setWindows(X11::getRoot(), NET_CLIENT_LIST, windows, num_windows);

    delete [] windows;
}

/**
 * Updates the Ewmh Stacking list hint.
 */
void
Workspaces::updateClientStackingList(void)
{
    unsigned int num_windows;
    Window *windows = buildClientList(num_windows);

    X11::setWindows(X11::getRoot(), NET_CLIENT_LIST_STACKING, windows, num_windows);

    delete [] windows;
}

/**
 * Make sure window is inside screen boundaries.
 */
void
Workspaces::placeWoInsideScreen(PWinObj *wo)
{
    Geometry gm_before(wo->getX(), wo->getY(), wo->getWidth(), wo->getHeight());
    Geometry gm_after(gm_before);

    Strut *strut = 0;
    if (wo->getType() == PWinObj::WO_FRAME) {
        Client *client = static_cast<Frame*>(wo)->getActiveClient();
        if (client) {
            strut = client->getStrut();
        }
    }

    X11::placeInsideScreen(gm_after, strut);
    if (gm_before != gm_after) {
        wo->move(gm_after.x, gm_after.y);
    }
}

//! @brief
//! @param wo PWinObj to originate from when searching
//! @param dir Direction to search
//! @param skip Bitmask for skipping window objects, defaults to 0
PWinObj*
Workspaces::findDirectional(PWinObj *wo, DirectionType dir, uint skip)
{
    struct Triplet {
        Triplet() : x(0), y(0), wo(0) { }
        int x, y;
        PWinObj *wo;
        bool lessX(const Triplet &p) {
            return x<p.x || (x==p.x && y<p.y) || (x==p.x && y==p.y && wo < p.wo);
        }
        bool lessY(const Triplet &p) {
            return y<p.y || (y==p.y && x<p.x) || (x==p.x && y==p.y && wo < p.wo);
        }
    } S, T, N; // Start, Temp, Next

    // search from the frame not client, if client
    if (wo->getType() == PWinObj::WO_CLIENT) {
        wo = static_cast<Client*>(wo)->getParent();
    }

    S.x = wo->getX()+wo->getWidth()/2;
    S.y = wo->getY()+wo->getHeight()/2;
    S.wo = N.wo = wo;

    const_iterator it(_wobjs.begin());
    for (; it != _wobjs.end(); ++it) {
        T.wo = *it;
        if (wo == T.wo) {
            continue;
        }
        if (T.wo->getType() != PWinObj::WO_FRAME ||
                static_cast<Frame*>(T.wo)->isSkip(skip) ||
                ! T.wo->isMapped()) {
            continue; // only include frames and not having skip set
        }
        T.x = T.wo->getX()+T.wo->getWidth()/2;
        T.y = T.wo->getY()+T.wo->getHeight()/2;

        if (N.wo == S.wo) {
            N = T;
            continue;
        }

        switch (dir) {
        case DIRECTION_UP:
            if ( (N.lessY(S) && T.lessY(S) && N.lessY(T)) ||
                 (!N.lessY(S) && (T.lessY(S) || N.lessY(T))) ) {
                N = T;
            }
            break;
        case DIRECTION_DOWN:
            if ( (S.lessY(N) && S.lessY(T) && T.lessY(N)) ||
                 (!S.lessY(N) && (S.lessY(T) || T.lessY(N))) ) {
                N = T;
            }
            break;
        case DIRECTION_LEFT:
            if ( (N.lessX(S) && T.lessX(S) && N.lessX(T)) ||
                 (!N.lessX(S) && (T.lessX(S) || N.lessX(T))) ) {
                N = T;
            }
            break;
        case DIRECTION_RIGHT:
            if ( (S.lessX(N) && S.lessX(T) && T.lessX(N)) ||
                 (!S.lessX(N) && (S.lessX(T) || T.lessX(N))) ) {
                N = T;
            }
            break;
        default:
            break;
        }
    }
    return N.wo;
}

