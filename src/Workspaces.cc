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

#include "Workspaces.hh"
#include "PScreen.hh"
#include "Atoms.hh"
#include "ParseUtil.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "Frame.hh"
#include "Client.hh" // For isSkip()
#include "WindowManager.hh"
#include "WorkspaceIndicator.hh"

#include <iostream>
#include <sstream>
#ifdef HAVE_LIMITS
#include <limits>
using std::numeric_limits;
#endif // HAVE_LIMITS

extern "C" {
#include <X11/Xatom.h> // for XA_WINDOW
}

using std::list;
using std::vector;
using std::string;
using std::find;
using std::wostringstream;
using std::wstring;
using std::cerr;
using std::endl;

// Workspaces::Workspace

Workspaces::Workspace::Workspace(const std::wstring &name, uint number, const std::list<PWinObj*> &wo_list)
  : _name(name), _number(number), _wo_list(wo_list), _last_focused(0)
{
}

Workspaces::Workspace::~Workspace(void)
{
}

// Workspaces

Workspaces *Workspaces::_instance = 0;


//! @brief Workpsaces constructor
Workspaces::Workspaces(uint number, uint per_row)
  : _active(0), _previous(0), _per_row(per_row)
{
#ifdef DEBUG
    if (_instance) {
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Workspaces(" << this << ")::Workspaces(" << number << ")"
             << endl << " *** _instance already set: " << _instance << endl;
    }
#endif // DEBUG
    _instance = this;

    if (number < 1) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Workspaces(" << this << ")::Workspaces(" << number << ")"
             << " *** number < 1" << endl;
#endif // DEBUG
        number = 1;
    }

    // create new workspaces
    for (uint i = 0; i < number; ++i) {
      _workspace_list.push_back(new Workspace(getWorkspaceName(i), i, _wo_list));
    }
}

//! @brief Workspaces destructor
Workspaces::~Workspaces(void)
{
    vector<Workspace*>::iterator it(_workspace_list.begin());
    for (; it != _workspace_list.end(); ++it) {
        delete *it;
    }

    _instance = 0;
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
        list<PWinObj*>::iterator it(_wo_list.begin());
        for (; it != _wo_list.end(); ++it) {
            if ((*it)->getWorkspace() > (number - 1))
                (*it)->setWorkspace(number - 1);
        }

        for (uint i = before - 1; i >= number; --i) {
            delete _workspace_list[i];
        }

        _workspace_list.resize(number, 0);

    } else { // We need more workspaces, lets create some
        for (uint i = before; i < number; ++i) {
          _workspace_list.push_back(new Workspace(getWorkspaceName(i), i, _wo_list));
        }
    }

    // Tell the rest of the world how many workspaces we have.
    XChangeProperty(PScreen::instance()->getDpy(),
                    PScreen::instance()->getRoot(),
                    Atoms::getAtom(NET_NUMBER_OF_DESKTOPS),
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
//! @param focus wheter or not to focus a window after switch
void
Workspaces::setWorkspace(uint num, bool focus)
{
    if ((num == _active) || ( num >= _workspace_list.size())) {
        return;
    }

    PScreen::instance()->grabServer();

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
    AtomUtil::setLong(PScreen::instance()->getRoot(),
                      Atoms::getAtom(NET_CURRENT_DESKTOP),
                      num);
    unhideAll(num, focus);

    PScreen::instance()->ungrabServer(true);

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
    PScreen::instance()->getMousePosition(x, y);

    if (dir != 0) {
      switch(dir) {
      case 1:
        x = PScreen::instance()->getWidth() - std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_LEFT) + 2, 2);
        break;
      case 2:
        x = std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_RIGHT) * 2, 2);
        break;
      case -1:
        y = PScreen::instance()->getHeight() - std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_BOTTOM) + 2, 2);
        break;
      case -2:
        y = std::max(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_TOP) + 2, 2);
        break;
      }

        // warp pointer
        XWarpPointer(PScreen::instance()->getDpy(), None,
                     PScreen::instance()->getRoot(), 0, 0, 0, 0, x, y);
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
    list<PWinObj*>::iterator it(_wo_list.begin()), position(_wo_list.end());
    for (; it != _wo_list.end() && position == _wo_list.end(); ++it) {
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

    _wo_list.insert(position, wo);

    if (position == _wo_list.end()) {
         XRaiseWindow(PScreen::instance()->getDpy(), wo->getWindow());
    } else {
         stackWinUnderWin((*position)->getWindow(), wo->getWindow());
    }
}

//! @brief Removes a PWinObj from the stacking list.
void
Workspaces::remove(PWinObj* wo)
{
    _wo_list.remove(wo);

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
    list<PWinObj*>::iterator it(_wo_list.begin());
    for (; it != _wo_list.end(); ++it) {
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
    if (workspace >= _workspace_list.size())
        return;
    _previous = _active;
    _active = workspace;


    list<PWinObj*>::iterator it(_wo_list.begin());
    for (; it != _wo_list.end(); ++it) {
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
    list<PWinObj*>::iterator it(find(_wo_list.begin(), _wo_list.end(), wo));

    if (it == _wo_list.end()) { // no Frame to raise.
        return;
    }
    _wo_list.erase(it);

    insert(wo, true); // reposition and restack
}

//! @brief Lower a PWinObj and restacks windows.
void
Workspaces::lower(PWinObj* wo)
{
    list<PWinObj*>::iterator it(find(_wo_list.begin(), _wo_list.end(), wo));

    if (it == _wo_list.end()) // no Frame to raise.
        return;
    _wo_list.erase(it);

    insert(wo, false); // reposition and restack
}

//! @brief Places the PWinObj above the window win
//! @param wo PWinObj to place.
//! @param win Window to place Frame above.
//! @param restack Restack the X windows, defaults to true.
void
Workspaces::stackAbove(PWinObj *wo, Window win, bool restack)
{
    list<PWinObj*>::iterator old_pos(find(_wo_list.begin(), _wo_list.end(), wo));

    if (old_pos != _wo_list.end()) {
        list<PWinObj*>::iterator it(_wo_list.begin());
        for (; it != _wo_list.end(); ++it) {
            if (win == (*it)->getWindow()) {
                _wo_list.erase(old_pos);
                _wo_list.insert(++it, wo);

                // Before restacking make sure we are the active frame
                // also that there are two different frames
                if (restack) {
                    stackWinUnderWin(win, wo->getWindow());
                }
                break;
            }
        }
    }
}

//! @brief Places the PWinObj below the window win
//! @param wo PWinObj to place.
//! @param win Window to place Frame under
//! @param restack Restack the X windows, defaults to true
void
Workspaces::stackBelow(PWinObj* wo, Window win, bool restack)
{
    list<PWinObj*>::iterator old_pos(find(_wo_list.begin(), _wo_list.end(), wo));

    if (old_pos != _wo_list.end()) {
        list<PWinObj*>::iterator it(_wo_list.begin());
        for (; it != _wo_list.end(); ++it) {
            if (win == (*it)->getWindow()) {
                _wo_list.erase(old_pos);
                _wo_list.insert(it, wo);

                if (restack) {
                    stackWinUnderWin(wo->getWindow(), win);
                }
                break;
            }
        }
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
    XRestackWindows(PScreen::instance()->getDpy(), windows, 2);
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
    list<PWinObj*>::reverse_iterator r_it = _wo_list.rbegin();
    for (; r_it != _wo_list.rend(); ++r_it) {
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

    list<Window> windows_list;
    list<PWinObj*>::iterator it_f, it_c;
    for (it_f = _wo_list.begin(); it_f != _wo_list.end(); ++it_f) {
        if ((*it_f)->getType() != PWinObj::WO_FRAME) {
	  continue;
	}

	frame = static_cast<Frame*>(*it_f);
	client_active = frame->getActiveClient();

	if (Config::instance()->isReportAllClients()) {
            for (it_c = frame->begin(); it_c != frame->end(); ++it_c) {
                client = dynamic_cast<Client*>(*it_c);
                if (client
		    && ! client->isSkip(SKIP_TASKBAR)
		    && client != client_active) {
                    windows_list.push_back(client->getWindow());
	        }
	    }
	}

	if (client_active && ! client_active->isSkip(SKIP_TASKBAR)) {
	    windows_list.push_back(client_active->getWindow());
	}
    }

    num_windows = windows_list.size();
    Window *windows = new Window[num_windows ? num_windows : 1];
    if (num_windows > 0) {
      copy(windows_list.begin(), windows_list.end(), windows);
    }

    return windows;
}

/**
 * Updates the Ewmh Client list hint.
 */
void
Workspaces::updateClientList(void)
{
    unsigned int num_windows;
    Window *windows = buildClientList(num_windows);

    AtomUtil::setWindows(PScreen::instance()->getRoot(),
			 Atoms::getAtom(NET_CLIENT_LIST),
			 windows, num_windows);
    
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

    AtomUtil::setWindows(PScreen::instance()->getRoot(),
			 Atoms::getAtom(NET_CLIENT_LIST_STACKING),
			 windows, num_windows);

    delete [] windows;
}

// PLACEMENT ROUTINES

//! @brief Tries to place the Wo.
void
Workspaces::placeWo(PWinObj *wo, Window parent)
{
    bool placed = false;

    list<uint>::iterator it(Config::instance()->getPlacementModelBegin());
    for (; (placed != true) &&
            (it != Config::instance()->getPlacementModelEnd()); ++it) {
        switch (*it) {
        case PLACE_SMART:
            placed = placeSmart(wo);
            break;
        case PLACE_MOUSE_NOT_UNDER:
            placed = placeMouseNotUnder(wo);
            break;
        case PLACE_MOUSE_CENTERED:
            placed = placeMouseCentered(wo);
            break;
        case PLACE_MOUSE_TOP_LEFT:
            placed = placeMouseTopLeft(wo);
            break;
        case PLACE_CENTERED_ON_PARENT:
            placed = placeCenteredOnParent(wo, parent);
            break;
        default:
            // do nothing
            break;
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

    placeInsideScreen(gm_after, strut);
    if (gm_before != gm_after) {
        wo->move(gm_after.x, gm_after.y);
    }
}

//! @brief Tries to find empty space to place the client in
//! @return true if client got placed, else false
//! @todo What should we do about Xinerama as when we don't have it enabled we care about the struts.
bool
Workspaces::placeSmart(PWinObj* wo)
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

    Geometry head;
    PScreen::instance()->getHeadInfoWithEdge(PScreen::instance()->getCurrHead(),
            head);

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
Workspaces::placeMouseNotUnder(PWinObj *wo)
{
    Geometry head;
    PScreen::instance()->getHeadInfoWithEdge(PScreen::instance()->getCurrHead(),
            head);

    int mouse_x, mouse_y;
    PScreen::instance()->getMousePosition(mouse_x, mouse_y);

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
Workspaces::placeMouseCentered(PWinObj *wo)
{
    int mouse_x, mouse_y;
    PScreen::instance()->getMousePosition(mouse_x, mouse_y);

    Geometry gm(mouse_x - (wo->getWidth() / 2), mouse_y - (wo->getHeight() / 2),
                wo->getWidth(), wo->getHeight());

    // make sure it's within the screens border
    placeInsideScreen(gm);

    wo->move(gm.x, gm.y);

    return true;
}

//! @brief Places the client like the menu gets placed
bool
Workspaces::placeMouseTopLeft(PWinObj *wo)
{
    int mouse_x, mouse_y;
    PScreen::instance()->getMousePosition(mouse_x, mouse_y);

    Geometry gm(mouse_x, mouse_y, wo->getWidth(), wo->getHeight());
    placeInsideScreen(gm); // make sure it's within the screens border

    wo->move(gm.x, gm.y);

    return true;
}

//! @brief Places centerd on the window parent
bool
Workspaces::placeCenteredOnParent(PWinObj *wo, Window parent)
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

//! @brief Makes sure the window is inside the screen.
void
Workspaces::placeInsideScreen(Geometry &gm, Strut *strut)
{
    // Do not include screen edges when calculating the position if the window
    // has a strut as it then is likely to be a panel or the like placed
    // along the edge of the screen.
    Geometry head;
    if (strut) {
        PScreen::instance()->getHeadInfo(PScreen::instance()->getCurrHead(), head);
    } else {
        PScreen::instance()->getHeadInfoWithEdge(PScreen::instance()->getCurrHead(), head);
    }

    if (gm.x < head.x) {
        gm.x = head.x;
    } else if ((gm.x + gm.width) > (head.x + head.width)) {
        gm.x = head.x + head.width - gm.width;
    }

    if (gm.y < head.y) {
        gm.y = head.y;
    } else if ((gm.y + gm.height) > (head.y + head.height)) {
        gm.y = head.y + head.height - gm.height;
    }
}

//! @brief
PWinObj*
Workspaces::isEmptySpace(int x, int y, const PWinObj* wo)
{
    if (! wo) {
        return 0;
    }

    // say that it's placed, now check if we are wrong!
    list<PWinObj*>::iterator it(_wo_list.begin());
    for (; it != _wo_list.end(); ++it) {
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
            if (client &&
                (client->isFullscreen()
                 || (client->isMaximizedVert() && client->isMaximizedHorz()))) {
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
    // search from the frame not client, if client
    if (wo->getType() == PWinObj::WO_CLIENT) {
        wo = static_cast<Client*>(wo)->getParent();
    }

    PWinObj *found_wo = 0;

    uint score = 0, score_min;
    int wo_main, wo_sec;
    int diff_main = 0;

#ifdef HAVE_LIMITS
    score_min = numeric_limits<uint>::max();
#else // !HAVE_LIMITS
    score_min = ~0;
#endif // HAVE_LIMITS

    // init wo variables
    if ((dir == DIRECTION_UP) || (dir == DIRECTION_DOWN)) {
        wo_main = wo->getY() + wo->getHeight() / 2;
        wo_sec = wo->getX() + wo->getWidth() / 2;
    } else {
        wo_main = wo->getX() + wo->getWidth() / 2;
        wo_sec = wo->getY() + wo->getHeight() / 2;
    }

    list<PWinObj*>::iterator it(_wo_list.begin());
    for (; it != _wo_list.end(); ++it) {
        if ((wo == (*it)) || ! ((*it)->isMapped())) {
            continue; // skip ourselves and unmapped wo's
        }
        if (((*it)->getType() != PWinObj::WO_FRAME) ||
                static_cast<Frame*>(*it)->isSkip(skip)) {
            continue; // only include frames and not having skip set
        }

        // check main direction, making sure it's at the right side
        // we check against the middle of the window as it gives a saner feeling
        // than the edges IMHO
        switch (dir) {
        case DIRECTION_UP:
            diff_main = wo_main - ((*it)->getY() + (*it)->getHeight() / 2);
            break;
        case DIRECTION_DOWN:
            diff_main = ((*it)->getY() + (*it)->getHeight() / 2) - wo_main;
            break;
        case DIRECTION_LEFT:
            diff_main = wo_main - ((*it)->getX() + (*it)->getWidth() / 2);
            break;
        case DIRECTION_RIGHT:
            diff_main = ((*it)->getX() + (*it)->getWidth() / 2) - wo_main;
            break;
        default:
            return 0; // no direction to search
        }

        if (diff_main < 0) {
            continue; // wrong direction
        }

        score = diff_main;

        if ((dir == DIRECTION_UP) || (dir == DIRECTION_DOWN)) {
            if ((wo_sec < (*it)->getX()) || (wo_sec > (*it)->getRX())) {
                score += PScreen::instance()->getHeight() / 2;
            }
            score += abs (static_cast<long> (wo_sec - ((*it)->getX ()
                                             + (*it)->getWidth () / 2)));

        } else {
            if ((wo_sec < (*it)->getY()) || (wo_sec > (*it)->getBY())) {
                score += PScreen::instance()->getWidth() / 2;
            }

            score += abs (static_cast<long> (wo_sec - ((*it)->getY ()
                                             + (*it)->getHeight () / 2)));
        }

        if (score < score_min) {
            found_wo = *it;
            score_min = score;
        }
    }

    return found_wo;
}

