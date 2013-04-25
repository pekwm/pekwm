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

using std::vector;
using std::string;
using std::find;
using std::wostringstream;
using std::wstring;
using std::cerr;
using std::endl;

// Workspaces::Workspace

Workspaces::Workspace::Workspace(const std::wstring &name, uint number)
  : _name(name), _number(number), _last_focused(0)
{
}

Workspaces::Workspace::~Workspace(void)
{
}

// Workspaces

uint Workspaces::_active;
uint Workspaces::_previous;
uint Workspaces::_per_row;
vector<PWinObj*> Workspaces::_wobjs;
vector<Workspaces::Workspace *> Workspaces::_workspace_list;

//! @brief Workspaces destructor
void
Workspaces::free(void)
{
    vector<Workspace*>::iterator it(_workspace_list.begin());
    for (; it != _workspace_list.end(); ++it) {
        delete *it;
    }
}

//! @brief Sets total amount of workspaces to number
void
Workspaces::setSize(uint number)
{
    if (number < 1) {
        number = 1;
    }

    if (number == _workspace_list.size()) {
        return; // no need to change number of workspaces to the current number
    }

    uint before = _workspace_list.size();
    if (_active >= number) {
        _active = number - 1;
    }
    if (_previous >= number) {
        _previous = number - 1;
    }

    // We have more workspaces than we want, lets remove the last ones
    if (before > number) {
        vector<PWinObj*>::const_iterator it(_wobjs.begin());
        for (; it != _wobjs.end(); ++it) {
            if ((*it)->getWorkspace() > (number - 1))
                (*it)->setWorkspace(number - 1);
        }

        for (uint i = before - 1; i >= number; --i) {
            delete _workspace_list[i];
        }

        _workspace_list.resize(number, 0);

    } else { // We need more workspaces, lets create some
        for (uint i = before; i < number; ++i) {
            _workspace_list.push_back(new Workspace(getWorkspaceName(i), i));
        }
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
    vector<Workspace*>::iterator it(_workspace_list.begin());
    for (; it != _workspace_list.end(); ++it) {
        (*it)->setName(Config::instance()->getWorkspaceName((*it)->getNumber()));
    }
}

//! @brief Activates Workspace workspace and sets the right hints
//! @param num Workspace to activate
//! @param focus whether or not to focus a window after switch
void
Workspaces::setWorkspace(uint num, bool focus)
{
    if ((num == _active) || ( num >= _workspace_list.size())) {
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
        workspace = _workspace_list.size() - per_row;
        // Add column
        workspace += _active - cur_row * per_row;
      } else {
        switched = false;
      }
      break;
    case WORKSPACE_NEXT_V:
    case WORKSPACE_DOWN:
      dir = -2;

      if ((_active + per_row) < _workspace_list.size()) {
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
    if ((num == _active) || ( num >= _workspace_list.size())) {
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

/**
 * Adds a PWinObj to the stacking list.
 * @param wo PWinObj to insert
 * @param raise Whether to insert at the bottom or top of the layer (defaults to true).
 */
void
Workspaces::insert(PWinObj *wo, bool raise)
{
    vector<PWinObj*>::iterator it(_wobjs.begin()), position(_wobjs.end());
    for (; it != _wobjs.end() && position == _wobjs.end(); ++it) {
        if (raise) {
            // If raising, make sure the inserted wo gets below the first
            // window in the next layer.
            if ((*it)->getLayer() > wo->getLayer()) {
                position = it;
            }
        } else {
            // If lowering, put the window below the first window with the same level.
            if (wo->getLayer() <= (*it)->getLayer()) {
                position = it;
            }
        }
    }

    position = _wobjs.insert(position, wo);

    if (++position == _wobjs.end()) {
        XRaiseWindow(X11::getDpy(), wo->getWindow());
    } else {
        stackWinUnderWin((*position)->getWindow(), wo->getWindow());
    }
}

//! @brief Removes a PWinObj from the stacking list.
void
Workspaces::remove(PWinObj* wo)
{
    vector<PWinObj*>::iterator it_wo(_wobjs.begin());
    for (;it_wo != _wobjs.end();) {
        if (wo == *it_wo) {
            it_wo = _wobjs.erase(it_wo);
        } else {
            ++it_wo;
        }
    }

    // remove from last focused
    vector<Workspace*>::iterator it(_workspace_list.begin());
    for (; it != _workspace_list.end(); ++it) {
        if (wo == (*it)->getLastFocused()) {
            (*it)->setLastFocused(0);
        }
    }
}

//! @brief Hides all non-sticky Frames on the workspace.
void
Workspaces::hideAll(uint workspace)
{
    vector<PWinObj*>::const_iterator it(_wobjs.begin());
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
    vector<PWinObj*>::const_iterator it(_wobjs.begin());
    for (; it != _wobjs.end(); ++it) {
        if (! (*it)->isMapped() && ! (*it)->isIconified() && ! (*it)->isHidden()
                && ((*it)->getWorkspace() == workspace)) {
            (*it)->mapWindow(); // don't restack ontop windows
        }
    }

    // Try to focus last focused window and if that fails we get the top-most
    // Frame if any and give it focus.
    if (focus) {
        PWinObj *wo = _workspace_list[workspace]->getLastFocused();
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
    vector<PWinObj*>::iterator it(find(_wobjs.begin(), _wobjs.end(), wo));

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
    vector<PWinObj*>::iterator it(find(_wobjs.begin(), _wobjs.end(), wo));

    if (it == _wobjs.end()) // no Frame to raise.
        return;
    _wobjs.erase(it);

    insert(wo, false); // reposition and restack
}

//! @brief Places the PWinObj below the window win
//! @param wo PWinObj to place.
//! @param win Window to place Frame under
//! @param above Stack above or below
//! @param restack Restack the X windows, defaults to true
void
Workspaces::stack(PWinObj* wo, Window win, bool above, bool restack)
{
    vector<PWinObj*>::iterator old_pos(find(_wobjs.begin(), _wobjs.end(), wo));

    if (old_pos != _wobjs.end()) {
        old_pos = _wobjs.erase(old_pos);
        vector<PWinObj*>::iterator it(_wobjs.begin());
        for (; it != _wobjs.end(); ++it) {
            if (win == (*it)->getWindow()) {
                if (above) {
                    ++it;
                }

                _wobjs.insert(it, wo);

                // Before restacking make sure we are the active frame
                // also that there are two different frames
                if (restack) {
                    if (above)
                        stackWinUnderWin(win, wo->getWindow());
                    else
                        stackWinUnderWin(wo->getWindow(), win);
                }
                return;
            }
        }
        _wobjs.insert(old_pos, wo);
    }
}


//! @brief
PWinObj*
Workspaces::getLastFocused(uint workspace)
{
    if (workspace >= _workspace_list.size()) {
        return 0;
    }
    return _workspace_list[workspace]->getLastFocused();
}

//! @brief
void
Workspaces::setLastFocused(uint workspace, PWinObj* wo)
{
    if (workspace >= _workspace_list.size()) {
        return;
    }
    
    _workspace_list[workspace]->setLastFocused(wo);
}

//! @brief Helper function to stack a window below another
//! @param win_over Window to place win_under under
//! @param win_under Window to place under win_over
void
Workspaces::stackWinUnderWin(Window over, Window under)
{
    if (over == under) {
        return;
    }

    Window windows[2] = { over, under };
    XRestackWindows(X11::getDpy(), windows, 2);
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
    vector<PWinObj*>::const_reverse_iterator r_it = _wobjs.rbegin();
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
    vector<PWinObj*>::const_iterator it_f, it_c;
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

// PLACEMENT ROUTINES

//! @brief Tries to place the Wo.
void
Workspaces::placeWo(PWinObj *wo, Window parent)
{
    Geometry head_curr;
    uint head_num_curr = X11::getCurrHead();

    bool placed = placeWo(wo, head_num_curr, parent);
    if (! placed) {
        // Placement failed on current head, try to fit it on another head. 
        for (uint i = 0; !placed && i < X11::getNumHeads(); ++i) {
            if (i != head_num_curr) {
                placed = placeWo(wo, head_num_curr, parent);
            }
        }

        // Still not placed, look out for fullscreen heads before final decision.
        if (! placed) {
            Geometry head;
            if (! isFullScreen(head_num_curr)) {
                X11::getHeadInfoWithEdge(head_num_curr, head);
                placed = placeHeadTR(wo, head, parent);
            } else {
                for (uint i = 0; !placed && i < X11::getNumHeads(); ++i) {
                    if (i != head_num_curr && !isFullScreen(i)) {
                        // No fullscreen window on this head
                        X11::getHeadInfoWithEdge(i, head);
                        placed = placeHeadTR(wo, head, parent);
                    }
                }
            }
        }
    }
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

bool
Workspaces::placeWo(PWinObj *wo, uint head_num, Window parent)
{
    // No point in trying to place on fullscreen
    if (isFullScreen(head_num)) {
        return false;
    }

    bool placed = false;
    Geometry head;
    X11::getHeadInfoWithEdge(head_num, head);

    vector<uint>::const_iterator it(Config::instance()->getPlacementModelBegin());
    for (; !placed && it != Config::instance()->getPlacementModelEnd(); ++it) {
        placed = placeWoOnHead(static_cast<enum PlacementModel>(*it), wo, head, parent);
    }

    return placed;
}

bool
Workspaces::placeWoOnHead(enum PlacementModel model, PWinObj *wo, const Geometry &head, Window parent)
{
    bool placed;

    switch (model) {
    case PLACE_SMART:
        placed = placeSmart(wo, head);
        break;
    case PLACE_MOUSE_NOT_UNDER:
        placed = placeMouseNotUnder(wo, head);
        break;
    case PLACE_MOUSE_CENTERED:
        placed = placeMouseCentered(wo, head);
        break;
    case PLACE_MOUSE_TOP_LEFT:
        placed = placeMouseTopLeft(wo, head);
        break;
    case PLACE_CENTERED_ON_PARENT:
        placed = placeCenteredOnParent(wo, head, parent);
        break;
    default:
        placed = false;
        break;
    }

    return placed;
}

//! @brief Tries to find empty space to place the client in
//! @return true if client got placed, else false
//! @todo What should we do about Xinerama as when we don't have it enabled we care about the struts.
bool
Workspaces::placeSmart(PWinObj* wo, const Geometry &head)
{
    PWinObj *wo_e;
    bool placed = false;

    int step_x = (Config::instance()->getPlacementLtR()) ? 1 : -1;
    int step_y = (Config::instance()->getPlacementTtB()) ? 1 : -1;
    int offset_x = (Config::instance()->getPlacementLtR())
                   ? Config::instance()->getPlacementOffsetX()
                   : -Config::instance()->getPlacementOffsetX();
    int offset_y = (Config::instance()->getPlacementTtB())
                   ? Config::instance()->getPlacementOffsetY()
                   : -Config::instance()->getPlacementOffsetY();
    int start_x, start_y, test_x = 0, test_y = 0;

    // Wrap these up, to get proper checking of space.
    uint wo_width = wo->getWidth() + Config::instance()->getPlacementOffsetX();
    uint wo_height = wo->getHeight() + Config::instance()->getPlacementOffsetY();

    start_x = (Config::instance()->getPlacementLtR())
              ? (head.x)
              : (head.x + head.width - wo_width);
    start_y = (Config::instance()->getPlacementTtB())
              ? (head.y)
              : (head.y + head.height - wo_height);

    if (Config::instance()->getPlacementRow()) { // row placement
        test_y = start_y;
        while (! placed && (Config::instance()->getPlacementTtB()
                           ? ((test_y + wo_height) <= (head.y + head.height))
                           : (test_y >= head.y))) {
            test_x = start_x;
            while (! placed && (Config::instance()->getPlacementLtR()
                               ? ((test_x + wo_width) <= (head.x + head.width))
                               : (test_x >= head.x))) {
                // see if we can place the window here
                if ((wo_e = isEmptySpace(test_x, test_y, wo))) {
                    placed = false;
                    test_x = Config::instance()->getPlacementLtR()
                             ? (wo_e->getX() + wo_e->getWidth()) : (wo_e->getX() - wo_width);
                } else {
                    placed = true;
                    wo->move(test_x + offset_x, test_y + offset_y);
                }
            }
            test_y += step_y;
        }
    } else { // column placement
        test_x = start_x;
        while (! placed && (Config::instance()->getPlacementLtR()
                           ? ((test_x + wo_width) <= (head.x + head.width))
                           : (test_x >= head.x))) {
            test_y = start_y;
            while (! placed && (Config::instance()->getPlacementTtB()
                               ? ((test_y + wo_height) <= (head.y + head.height))
                               : (test_y >= head.y))) {
                // see if we can place the window here
                if ((wo_e = isEmptySpace(test_x, test_y, wo))) {
                    placed = false;
                    test_y = Config::instance()->getPlacementTtB()
                             ? (wo_e->getY() + wo_e->getHeight()) : (wo_e->getY() - wo_height);
                } else {
                    placed = true;
                    wo->move(test_x + offset_x, test_y + offset_y);
                }
            }
            test_x += step_x;
        }
    }

    return placed;

}

//! @brief Places the wo in a corner of the screen not under the pointer
bool
Workspaces::placeMouseNotUnder(PWinObj *wo, const Geometry &head)
{
    int mouse_x, mouse_y;
    X11::getMousePosition(mouse_x, mouse_y);

    // compensate for head offset
    mouse_x -= head.x;
    mouse_y -= head.y;

    // divide the screen into four rectangles using the pointer as divider
    if ((wo->getWidth() < unsigned(mouse_x)) && (wo->getHeight() < head.height)) {
        wo->move(head.x, head.y);
        return true;
    }

    if ((wo->getWidth() < head.width) && (wo->getHeight() < unsigned(mouse_y))) {
        wo->move(head.x + head.width - wo->getWidth(), head.y);
        return true;
    }

    if ((wo->getWidth() < (head.width - mouse_x)) && (wo->getHeight() < head.height)) {
        wo->move(head.x + head.width - wo->getWidth(), head.y + head.height - wo->getHeight());
        return true;
    }

    if ((wo->getWidth() < head.width) && (wo->getHeight() < (head.height - mouse_y))) {
        wo->move(head.x, head.y + head.height - wo->getHeight());
        return true;
    }

    return false;
}

//! @brief Places the client centered under the mouse
bool
Workspaces::placeMouseCentered(PWinObj *wo, const Geometry &head)
{
    int mouse_x, mouse_y;
    X11::getMousePosition(mouse_x, mouse_y);

    Geometry gm(mouse_x - (wo->getWidth() / 2), mouse_y - (wo->getHeight() / 2),
                wo->getWidth(), wo->getHeight());

    // make sure it's within the screen's border
    X11::placeInsideScreen(gm);

    wo->move(gm.x, gm.y);

    return true;
}

//! @brief Places the client like the menu gets placed
bool
Workspaces::placeMouseTopLeft(PWinObj *wo, const Geometry &head)
{
    int mouse_x, mouse_y;
    X11::getMousePosition(mouse_x, mouse_y);

    Geometry gm(mouse_x, mouse_y, wo->getWidth(), wo->getHeight());
    X11::placeInsideScreen(gm); // make sure it's within the screen's border

    wo->move(gm.x, gm.y);

    return true;
}

//! @brief Places centerd on the window parent
bool
Workspaces::placeCenteredOnParent(PWinObj *wo, const Geometry &head, Window parent)
{
    if (parent == None) {
        return false;
    }

    PWinObj *wo_s = PWinObj::findPWinObj(parent);
    if (wo_s) {
        wo->move(wo_s->getX() + wo_s->getWidth() / 2 - wo->getWidth() / 2,
                 wo_s->getY() + wo_s->getHeight() / 2 - wo->getHeight() / 2);
        return true;
    }

    return false;
}

bool
Workspaces::placeHeadTR(PWinObj *wo, const Geometry &head, Window parent)
{
    wo->move(head.x, head.y);
    return true;
}

bool
Workspaces::isFullScreen(uint head_num)
{
    vector<PWinObj*>::const_iterator it(_wobjs.begin());
    for (; it != _wobjs.end(); ++it) {
        if ((*it)->isMapped() && (*it)->getType() == PWinObj::WO_FRAME) {
            Client *client = static_cast<Frame*>((*it))->getActiveClient();
            if (client && client->isFullscreen() && client->getHead() == head_num) {
                return true;
            }
        }
    }
    return false;
}

PWinObj*
Workspaces::isEmptySpace(int x, int y, const PWinObj* wo)
{
    if (! wo) {
        return 0;
    }

    // say that it's placed, now check if we are wrong!
    vector<PWinObj*>::const_iterator it(_wobjs.begin());
    for (; it != _wobjs.end(); ++it) {
        // Skip ourselves, non-mapped and desktop objects. Iconified means
        // skip placement.
        if (wo == (*it) || ! (*it)->isMapped() || (*it)->isIconified()
            || ((*it)->getLayer() == LAYER_DESKTOP)) {
            continue;
        }

        // Also skip windows tagged as Maximized as they cause us to
        // automatically fail.
        if ((*it)->getType() == PWinObj::WO_FRAME) {
            Client *client = static_cast<Frame*>((*it))->getActiveClient();
            if (client && (client->isMaximizedVert() && client->isMaximizedHorz())) {
                continue;
            }
        }

        // Check if we are "intruding" on some other window's place
        if (((*it)->getX() < signed(x + wo->getWidth())) &&
            (signed((*it)->getX() + (*it)->getWidth()) > x) &&
            ((*it)->getY() < signed(y + wo->getHeight())) &&
            (signed((*it)->getY() + (*it)->getHeight()) > y)) {
            return (*it);
        }
    }

    return 0; // we passed the test, no frames in the way
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

    vector<PWinObj*>::const_iterator it(_wobjs.begin());
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

