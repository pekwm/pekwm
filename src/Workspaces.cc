//
// Workspaces.cc for pekwm
// Copyright (C) 2002-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Compat.hh"
#include "Debug.hh"
#include "Workspaces.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "Frame.hh"
#include "Client.hh" // For isSkip()
#include "ManagerWindows.hh"
#include "WinLayouter.hh"
#include "WorkspaceIndicator.hh"
#include "X11.hh"

#include <iostream>
#include <sstream>
#ifdef PEKWM_HAVE_LIMITS
#include <limits>
#endif // PEKWM_HAVE_LIMITS

extern "C" {
#include <signal.h>
#include <sys/time.h>

#include <X11/Xatom.h> // for XA_WINDOW
}

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
std::vector<PWinObj*> Workspaces::_wobjs;
std::vector<Workspace> Workspaces::_workspaces;
std::vector<Frame*> Workspaces::_mru;
WorkspaceIndicator* Workspaces::_workspace_indicator = nullptr;

WinLayouter *Workspace::_default_layouter = WinLayouterFactory("SMART");

void
Workspaces::init(void)
{
	_workspace_indicator = new WorkspaceIndicator();
}

void
Workspaces::cleanup()
{
	delete _workspace_indicator;
}

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
	X11::setCardinal(X11::getRoot(), NET_NUMBER_OF_DESKTOPS,
			 static_cast<Cardinal>(number));

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
	std::vector<Workspace>::size_type i=0, size=_workspaces.size();
	for (; i<size; ++i) {
		_workspaces[i].setName(pekwm::config()->getWorkspaceName(i));
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
	X11::setCardinal(X11::getRoot(), NET_CURRENT_DESKTOP, num);

	_previous = _active;
	_active = num;

	unhideAll(num, focus);

	X11::ungrabServer(true);

	showWorkspaceIndicator();
}

void
Workspaces::showWorkspaceIndicator(void)
{
	int timeout = pekwm::config()->getShowWorkspaceIndicator();
	if (timeout > 0) {
		_workspace_indicator->render();
		_workspace_indicator->mapWindowRaised();
		PWinObj::setSkipEnterAfter(_workspace_indicator);

		struct itimerval value;
		timerclear(&value.it_value);
		timerclear(&value.it_interval);
		value.it_value.tv_sec += timeout / 1000;
		value.it_value.tv_usec += (timeout % 1000) * 1000;
		setitimer(ITIMER_REAL, &value, 0);
	}
}

void
Workspaces::hideWorkspaceIndicator(void)
{
	_workspace_indicator->unmapWindow();
}

bool
Workspaces::gotoWorkspace(uint direction, bool warp)
{
	uint workspace;
	int dir = 0;
	// Using a bool flag to detect changes due to special workspaces such
	// as PREV
	bool switched = true;
	uint per_row = pekwm::config()->getWorkspacesPerRow();

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
	case WORKSPACE_PREV_N:
	case WORKSPACE_LEFT_N:
		dir = 1;

		if (_active > 0) {
			workspace = _active - 1;
		} else if (direction == WORKSPACE_PREV_N) {
			workspace = _workspaces.size() - 1;
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
	case WORKSPACE_NEXT_N:
	case WORKSPACE_RIGHT_N:
		dir = 2;

		if (_active < (_workspaces.size() - 1)) {
			workspace = _active + 1;
		} else if (direction == WORKSPACE_NEXT_N) {
			workspace = 0;
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
			x = X11::getWidth() - std::max(pekwm::config()->getScreenEdgeSize(SCREEN_EDGE_LEFT) + 2, 2);
			break;
		case 2:
			x = std::max(pekwm::config()->getScreenEdgeSize(SCREEN_EDGE_RIGHT) * 2, 2);
			break;
		case -1:
			y = X11::getHeight() - std::max(pekwm::config()->getScreenEdgeSize(SCREEN_EDGE_BOTTOM) + 2, 2);
			break;
		case -2:
			y = std::max(pekwm::config()->getScreenEdgeSize(SCREEN_EDGE_TOP) + 2, 2);
			break;
		}

		// warp pointer
		X11::warpPointer(x, y);
	}

	// set workpsace
	setWorkspace(num, true);

	return true;
}

void
Workspaces::fixStacking(PWinObj *pwo)
{
	const_iterator it = find(_wobjs.begin(), _wobjs.end(), pwo);
	if (it == _wobjs.end()) {
		return;
	}

	if (++it == _wobjs.end()) {
		X11::raiseWindow(pwo->getWindow());
	} else {
		Window winlist[] = { (*it)->getWindow(), pwo->getWindow() };
		X11::stackWindows(winlist, 2);
	}
}

/**
 * Adds a PWinObj to the stacking list. Assumes that wo is not in _wobjs
 * already.
 * @param wo PWinObj to insert
 * @param raise Whether to insert at the bottom or top of the layer
 *        (defaults to true).
 */
void
Workspaces::insert(PWinObj *wo, bool raise)
{
	PWinObj *top_obj = 0;
	Frame *frame, *wo_frame = dynamic_cast<Frame*>(wo);
	iterator it;

	if (! raise
	    && wo_frame && wo_frame->getTransFor()
	    && wo_frame->getTransFor()->getLayer() == wo_frame->getLayer()) {
		// Lower only to the top of the transient_for window.
		it = find(_wobjs.begin(), _wobjs.end(), wo_frame->getTransFor()->getParent());
		++it;
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

	std::vector<PWinObj*> winstack;
	winstack.reserve(3);
	winstack.push_back(wo);

	if (wo_frame && wo_frame->hasTrans()) {
		std::vector<Client*>::const_iterator t_it;
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

		it = find(_wobjs.begin(), _wobjs.end(), wo);
		++it;
		_wobjs.insert(it, winstack.begin()+1, winstack.end());
	}

	if (top_obj) {
		winstack.push_back(top_obj);
	} else {
		X11::raiseWindow(winstack.back()->getWindow());
	}

	const unsigned size = winstack.size();
	Window *wins = new Window[size];
	std::vector<PWinObj*>::iterator wit(winstack.end());
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
	std::vector<Workspace>::iterator it = _workspaces.begin();
	for (; it != _workspaces.end(); ++it) {
		if (wo == it->getLastFocused()) {
			it->setLastFocused(0);
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
				addToMRUFront(frame);
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
	if (handleFullscreenBeforeRaise(wo)) {
		it = find(_wobjs.begin(), _wobjs.end(), wo);
	}
	_wobjs.erase(it);

	insert(wo, true); // reposition and restack
}

/**
 * Handles fullscreen windows when raising a window: if a normal
 * window is raised, fullscreen windows on LAYER_ABOVE_DOCK are
 * lowered. If a fullscreen window is raised, bring it back to
 * LAYER_ABOVE_DOCK.
 *
 * @return true if _wobjs iterators may be invalid
 */
bool
Workspaces::handleFullscreenBeforeRaise(PWinObj* wo)
{
	if (!pekwm::config()->isFullscreenAbove()
	    || wo->getLayer() == LAYER_DESKTOP
	    || wo->getLayer() >= LAYER_ABOVE_DOCK) {
		return false;
	}

	if (wo->isFullscreen()) {
		// Fullscreen windows may have been removed from
		// LAYER_ABOVE_DOCK by lowerFullscreenWindows(). Raise it back
		// to LAYER_ABOVE_DOCK
		wo->setLayer(LAYER_ABOVE_DOCK);
		return false;
	}

	// Move all mapped fullscreen windows at LAYER_ABOVE_DOCK back
	// to wo->getLayer() in order for "wo" to be visible.
	//
	// Since restacking is done one by one, this could lead to
	// some flickering, but quite minimmum.
	return lowerFullscreenWindows(wo->getLayer());
}

/**
 * Move all windows above new_layer back to new_layer and restack one
 * by one, from bottom to top.
 *
 * Since the top fullscreen window should hide everything else,
 * multiple restacking operations shouldn't produce any flickering
 * until the top one is restacked, which may show a few more windows
 * on top of it.
 *
 * @param new_layer the new layer for fullscreen windows
 * @return true if _wobjs iterators may be invalid
 */
bool
Workspaces::lowerFullscreenWindows(Layer new_layer)
{
	std::vector<PWinObj*> fs_wobjs;

	// Note, fullscreen windows are typically only in
	// LAYER_ABOVE_DOCK. However, a call to lowerFullscreenWindows()
	// with new_layer == LAYER_ONTOP could put fullscreen windows
	// there, so we need to check all layers above new_layer, not just
	// LAYER_ABOVE_DOCK.
	iterator wo = _wobjs.begin();
	for (; wo != _wobjs.end(); ++wo) {
		if ((*wo)->getLayer() > new_layer
		    && (*wo)->isMapped() && (*wo)->isFullscreen()) {
			fs_wobjs.push_back(*wo);
			(*wo)->setLayer(new_layer);
		}
	}

	for (wo = fs_wobjs.begin(); wo != fs_wobjs.end(); ++wo) {
		// We could have erased these windows in the first for loop,
		// which could reduce a couple of find() calls.
		//
		// But we want the higher fullscreen windows to _stay_ in
		// _wobjs, so that they can be the (top_obj) anchor point for
		// restacking. And since that anchor is most likely fullscreen
		// and hides everything else, no flickering.
		iterator it(std::find(_wobjs.begin(), _wobjs.end(), *wo));

		if (it != _wobjs.end()) {
			_wobjs.erase(it);
			insert(*wo, true);
		}
	}
	return !fs_wobjs.empty();
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

PWinObj*
Workspaces::getLastFocused(uint workspace)
{
	if (workspace >= _workspaces.size()) {
		return 0;
	}
	return _workspaces[workspace].getLastFocused();
}

void
Workspaces::setLastFocused(uint workspace, PWinObj* wo)
{
	if (workspace >= _workspaces.size()) {
		return;
	}
	_workspaces[workspace].setLastFocused(wo);
}

//! @brief Create name for workspace num
std::string
Workspaces::getWorkspaceName(uint num)
{
	std::ostringstream buf;
	buf << num + 1;
	buf << ": ";
	buf << pekwm::config()->getWorkspaceName(num);
	return buf.str();
}

// MISC METHODS

//! @brief Returns the first focusable PWinObj with the highest stacking
PWinObj*
Workspaces::getTopWO(uint type_mask)
{
	/* FIXME:
	   const_reverse_iterator r_it = _wobjs.rbegin();
	   for (; r_it != _wobjs.end(); ++r_it) {
	   if ((*r_it)->isMapped()
	   && (*r_it)->isFocusable()
	   && ((*r_it)->getType()&type_mask)) {
	   return (*r_it);
	   }
	   }
	*/
	return nullptr;
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

	std::vector<Window> windows;
	iterator it_f;
	const_iterator it_c;
	for (it_f = _wobjs.begin(); it_f != _wobjs.end(); ++it_f) {
		if ((*it_f)->getType() != PWinObj::WO_FRAME) {
			continue;
		}

		frame = static_cast<Frame*>(*it_f);
		client_active = frame->getActiveClient();

		if (pekwm::config()->isReportAllClients()) {
			for (it_c = frame->begin(); it_c != frame->end(); ++it_c) {
				client = dynamic_cast<Client*>(*it_c);
				if (client
				    && client != client_active
				    && ! client->isSkip(SKIP_TASKBAR)) {
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
	uint num;
	Window *windows = buildClientList(num);
	// previously, the lists where unset when they ended up empty
	// however some applications does not support this, one
	// example being tint2 on Debian Stretch
	X11::setWindows(X11::getRoot(), NET_CLIENT_LIST, windows, num);
	X11::setWindows(X11::getRoot(), NET_CLIENT_LIST_STACKING, windows, num);
	delete [] windows;
}

/**
 * Updates the Ewmh Stacking list hint.
 */
void
Workspaces::updateClientStackingList(void)
{
	uint num;
	Window *windows = buildClientList(num);
	X11::setWindows(X11::getRoot(), NET_CLIENT_LIST_STACKING, windows, num);
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

	pekwm::rootWo()->placeInsideScreen(gm_after, strut);
	if (gm_before != gm_after) {
		wo->move(gm_after.x, gm_after.y);
	}
}

/**
 * Searches for a PWinObj to focus, and then gives it input focus
 */
void
Workspaces::findWOAndFocus(PWinObj *search)
{
	PWinObj *focus = nullptr;
	if (PWinObj::windowObjectExists(search)
	    && search->isMapped()
	    && search->isFocusable())  {
		focus = search;
	}

	// search window object didn't exist, go through the MRU list
	if (! focus) {
		std::vector<Frame*>::iterator it = _mru.begin();
		for (; it != _mru.end(); ++it) {
			if ((*it)->isMapped() && (*it)->isFocusable()) {
				focus = *it;
				break;
			}
		}
	}

	if (focus) {
		focus->giveInputFocus();
	}  else if (! PWinObj::getFocusedPWinObj()) {
		pekwm::rootWo()->giveInputFocus();
		pekwm::rootWo()->setEwmhActiveWindow(None);
	}
}

/**
 * wo PWinObj to originate from when searching
 *
 * @param dir Direction to search
 * @param skip Bitmask for skipping window objects, defaults to 0
 */
PWinObj*
Workspaces::findDirectional(PWinObj *wo, DirectionType dir, uint skip)
{
	// search from the frame not client, if client
	if (wo->getType() == PWinObj::WO_CLIENT) {
		wo = static_cast<Client*>(wo)->getParent();
	}

	PWinObj *found_wo = 0;

	uint score, score_min;
	int wo_main, wo_sec;
	int diff_main, diff_pos;

	score_min = std::numeric_limits<uint>::max();

	// init wo variables
	if ((dir == DIRECTION_UP) || (dir == DIRECTION_DOWN)) {
		wo_main = wo->getY() + wo->getHeight() / 2;
		wo_sec = wo->getX() + wo->getWidth() / 2;
	} else {
		wo_main = wo->getX() + wo->getWidth() / 2;
		wo_sec = wo->getY() + wo->getHeight() / 2;
	}

	const_iterator it(_wobjs.begin());
	for (; it != _wobjs.end(); ++it) {
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
			diff_pos = wo->getY() - (*it)->getY();
			break;
		case DIRECTION_DOWN:
			diff_main = ((*it)->getY() + (*it)->getHeight() / 2) - wo_main;
			diff_pos = (*it)->getBY() - wo->getBY();
			break;
		case DIRECTION_LEFT:
			diff_main = wo_main - ((*it)->getX() + (*it)->getWidth() / 2);
			diff_pos = wo->getX() - (*it)->getX();
			break;
		case DIRECTION_RIGHT:
			diff_main = ((*it)->getX() + (*it)->getWidth() / 2) - wo_main;
			diff_pos = (*it)->getRX() - wo->getRX();
			break;
		default:
			return 0; // no direction to search
		}

		if (diff_main < 0) {
			continue; // wrong direction
		} else if (diff_pos <= 0) {
			continue; // no difference in direction
		}

		score = diff_main;

		if ((dir == DIRECTION_UP) || (dir == DIRECTION_DOWN)) {
			if ((wo_sec < (*it)->getX()) || (wo_sec > (*it)->getRX())) {
				score += X11::getHeight() / 2;
			}
			score += abs (static_cast<long> (wo_sec - ((*it)->getX ()
								   + (*it)->getWidth () / 2)));

		} else {
			if ((wo_sec < (*it)->getY()) || (wo_sec > (*it)->getBY())) {
				score += X11::getWidth() / 2;
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

/**
 * Finds the next frame in the list
 *
 * @param frame Frame to search from
 * @param mapped Match only agains mapped frames
 * @param mask Defaults to 0
 */
Frame*
Workspaces::getNextFrame(Frame* frame, bool mapped, uint mask)
{
	if (! frame || (Frame::frame_size() < 2)) {
		return nullptr;
	}

	Frame *next_frame = nullptr;
	Frame::frame_cit f_it =
		std::find(Frame::frame_begin(), Frame::frame_end(), frame);
	if (f_it != Frame::frame_end()) {
		Frame::frame_cit n_it(f_it);
		if (++n_it == Frame::frame_end()) {
			n_it = Frame::frame_begin();
		}

		while (! next_frame && n_it != f_it) {
			if (! (*n_it)->isSkip(mask) && (! mapped || (*n_it)->isMapped())) {
				next_frame =  (*n_it);
			}
			if (++n_it == Frame::frame_end()) {
				n_it = Frame::frame_begin();
			}
		}
	}

	return next_frame;
}

/**
 * Finds the previous frame in the list
 *
 * @param frame Frame to search from
 * @param mapped Match only agains mapped frames
 * @param mask Defaults to 0
 */
Frame*
Workspaces::getPrevFrame(Frame* frame, bool mapped, uint mask)
{
	if (! frame || Frame::frame_size() < 2) {
		return nullptr;
	}

	Frame *prev_frame = nullptr;
	Frame::frame_cit f_it =
		std::find(Frame::frame_begin(), Frame::frame_end(), frame);
	if (f_it != Frame::frame_end()) {
		Frame::frame_cit n_it(f_it);

		if (n_it == Frame::frame_begin()) {
			n_it = Frame::frame_end();
		}
		--n_it;

		while (! prev_frame && (n_it != f_it)) {
			if (! (*n_it)->isSkip(mask) && (! mapped || (*n_it)->isMapped())) {
				prev_frame =  (*n_it);
			}
			if (n_it == Frame::frame_begin()) {
				n_it = Frame::frame_end();
			}
			--n_it;
		}
	}

	return prev_frame;
}
