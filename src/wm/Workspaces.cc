//
// Workspaces.cc for pekwm
// Copyright (C) 2002-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Compat.hh"
#include "Debug.hh"
#include "Workspaces.hh"
#include "Config.hh"
#include "Frame.hh"
#include "Client.hh" // For isSkip()
#include "ManagerWindows.hh"
#include "WinLayouter.hh"
#include "WorkspaceIndicator.hh"
#include "X11.hh"

#include "tk/PWinObj.hh"

#include <iostream>
#include <sstream>
#ifdef PEKWM_HAVE_LIMITS
#include <limits>
#endif // PEKWM_HAVE_LIMITS

extern "C" {
#include <assert.h>
#include <sys/time.h>

#include <X11/Xatom.h> // for XA_WINDOW
}

static inline bool
isFocusable(PWinObj *wo)
{
	return wo && wo->isMapped() && wo->isFocusable();
}

// Workspace

Workspace::Workspace(void)
	: _last_focused(nullptr)
{
}

Workspace::Workspace(const Workspace &w)
	: _name(w._name),
	  _last_focused(w._last_focused)
{
}

Workspace::~Workspace(void)
{
}

Workspace &Workspace::operator=(const Workspace &w)
{
	_name = w._name;
	_last_focused = w._last_focused;
	return *this;
}

/**
 * Return last focused window object, checks that the object still
 * exists and is mapped and focusable before returning it.
 */
PWinObj*
Workspace::getLastFocused(bool verify) const
{
	if (! verify || _last_focused == nullptr) {
		return _last_focused;
	}

	if (PWinObj::windowObjectExists(_last_focused)
	    && isFocusable(_last_focused)) {
		return _last_focused;
	}
	return nullptr;
}

/**
 * Set last focused window object for workspace.
 */
void
Workspace::setLastFocused(PWinObj* wo)
{
	_last_focused = wo;
}

// Workspaces

uint Workspaces::_active;
uint Workspaces::_previous;
uint Workspaces::_per_row;
std::vector<WinLayouter*> Workspaces::_layout_models;
std::vector<PWinObj*> Workspaces::_wobjs;
std::vector<Workspace> Workspaces::_workspaces;
std::vector<Frame*> Workspaces::_mru;
WorkspaceIndicator* Workspaces::_workspace_indicator = nullptr;
Workspaces::win_layouter_map Workspaces::_win_layouters;

void
Workspaces::init()
{
	int type_i = 1;
	while (type_i != static_cast<int>(WIN_LAYOUTER_NO)) {
		enum WinLayouterType type =
			static_cast<enum WinLayouterType>(type_i);
		_win_layouters[type] = mkWinLayouter(type);
		assert(_win_layouters[type]);
		type_i <<= 1;
	}
}

void
Workspaces::cleanup()
{
	clearLayoutModels();
	delete _workspace_indicator;

	std::map<enum WinLayouterType, WinLayouter*>::iterator
		it(_win_layouters.begin());
	for (; it != _win_layouters.end(); ++it) {
		delete it->second;
	}
	_win_layouters.clear();

	X11::deleteProperty(X11::getRoot(), NET_CLIENT_LIST);
	X11::deleteProperty(X11::getRoot(), NET_CLIENT_LIST_STACKING);
	X11::deleteProperty(X11::getRoot(), PEKWM_CLIENT_LIST);
}

//! @brief Sets total amount of workspaces to number
void
Workspaces::setSize(uint number)
{
	if (number < 1) {
		number = 1;
	}

	if (number == _workspaces.size()) {
		// no need to change number of workspaces to the current number
		return;
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
	std::vector<Workspace>::size_type i = 0;
	std::vector<Workspace>::size_type num_workspaces=_workspaces.size();
	for (; i < num_workspaces; ++i) {
		_workspaces[i].setName(pekwm::config()->getWorkspaceName(i));
	}
}

/**
 * Set layout models from provided string vector.
 */
void
Workspaces::setLayoutModels(const std::vector<std::string> &models)
{
	clearLayoutModels();
	std::vector<std::string>::const_iterator it(models.begin());
	for (; it != models.end(); ++it) {
		enum WinLayouterType type = win_layouter_type_from_string(*it);
		if (type != WIN_LAYOUTER_NO) {
			_layout_models.push_back(_win_layouters[type]);
			assert(_layout_models.back());
		}
	}
}

void
Workspaces::clearLayoutModels(void)
{
	_layout_models.clear();
}

/**
 * Activates Workspace workspace (or previous if back_and_forth is true) and
 * update EWMH hints.
 *
 * @param num Workspace to activate
 * @param back_and_forth If true, and the workspace being activated is the
 *        current one instead go to the previously active one.
 * @param focus whether or not to focus a window after switch
 * @return true if workspace was changed, else false.
 */
bool
Workspaces::setWorkspace(uint num, bool focus, bool back_and_forth)
{
	if (num >= _workspaces.size()) {
		return false;
	} else if (num == _active) {
		if (! back_and_forth || _active == _previous) {
			return false;
		}
		num = _previous;
	}

	X11::grabServer();

	PWinObj *wo = PWinObj::getFocusedPWinObj();
	// Make sure that sticky windows gets unfocused on workspace change,
	// it will be set back after switch is done.
	if (wo) {
		if (wo->isType(PWinObj::WO_CLIENT)) {
			wo->getParent()->setFocused(false);
		} else {
			wo->setFocused(false);
		}
	}

	// Save the focused window object
	setLastFocused(_active, wo);
	PWinObj::setFocusedPWinObj(nullptr);
	pekwm::rootWo()->giveInputFocus();

	// switch workspace
	hideAll(_active);
	X11::setCardinal(X11::getRoot(), NET_CURRENT_DESKTOP, num);

	_previous = _active;
	_active = num;

	unhideAll(num, focus);

	X11::ungrabServer(true);

	showWorkspaceIndicator();

	return true;
}

void
Workspaces::showWorkspaceIndicator(void)
{
	int timeout = pekwm::config()->getShowWorkspaceIndicator();
	if (timeout < 1) {
		return;
	}

	WorkspaceIndicator *wsi = getWorkspaceIndicator();
	wsi->render();
	wsi->mapWindowRaised();
	PWinObj::setSkipEnterAfter(wsi);

	TimeoutAction action(ACTION_HIDE_WORKSPACE_INDICATOR, timeout);
	pekwm::timeouts()->replace(action);
}

void
Workspaces::hideWorkspaceIndicator(void)
{
	getWorkspaceIndicator()->unmapWindow();
}

bool
Workspaces::gotoWorkspace(uint direction, bool focus, bool warp)
{
	uint workspace;
	int dir = 0;
	// Using a bool flag to detect changes due to special workspaces such
	// as PREV
	bool switched = true;
	bool back_and_forth = false;
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
		switched = _active != workspace;
		break;
	default:
		workspace = direction;
		switched = _active != workspace;
		back_and_forth = pekwm::config()->isWorkspacesBackAndForth();
	}

	if (switched || back_and_forth) {
		if (warp) {
			warpToWorkspace(workspace, dir);
		} else {
			switched = setWorkspace(workspace, focus,
						back_and_forth);
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
	Config* cfg = pekwm::config();

	if (dir != 0) {
		int edge_size;
		switch(dir) {
		case 1:
			edge_size = cfg->getScreenEdgeSize(SCREEN_EDGE_LEFT);
			x = X11::getWidth() - std::max(edge_size + 2, 2);
			break;
		case 2:
			edge_size = cfg->getScreenEdgeSize(SCREEN_EDGE_RIGHT);
			x = std::max(edge_size + 2, 2);
			break;
		case -1:
			edge_size = cfg->getScreenEdgeSize(SCREEN_EDGE_BOTTOM);
			y = X11::getHeight() - std::max(edge_size + 2, 2);
			break;
		case -2:
			edge_size = cfg->getScreenEdgeSize(SCREEN_EDGE_TOP);
			y = std::max(edge_size + 2, 2);
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
	const_iterator it = find(pwo);
	if (it == _wobjs.end()) {
		return;
	}

	if (++it == _wobjs.end()) {
		X11::raiseWindow(pwo->getWindow());
	} else {
		std::vector<Window> winlist;
		winlist.push_back((*it)->getWindow());
		winlist.push_back(pwo->getWindow());
		X11::stackWindows(winlist);
	}
}

/**
 * Place window based on the current placement model.
 */
void
Workspaces::layout(Frame *frame, Window parent, int win_layouter_types)
{
	if (frame == nullptr) {
		return;
	}

	frame->updateDecor();

	// Collect the information which head has a fullscreen window. To be
	// conservative for now we ignore fullscreen windows on the desktop or
	// normal layer, because it might be a file manager in desktop mode,
	// for example.
	std::vector<bool> fsHead(X11::getNumHeads(), false);
	Workspaces::const_iterator it(Workspaces::begin()),
		w_end(Workspaces::end());
	for (; it != w_end; ++it) {
		if ((*it)->isMapped() && (*it)->isType(PWinObj::WO_FRAME)) {
			Client *client =
				static_cast<Frame*>(*it)->getActiveClient();
			if (client && client->isFullscreen()
			    && client->getLayer()>LAYER_NORMAL) {
				fsHead[client->getHead()] = true;
			}
		}
	}

	// Try to place the window
	Geometry gm;
	CurrHeadSelector chs = pekwm::config()->getCurrHeadSelector();
	int head_nr = X11Util::getCurrHead(chs);

	// update pointer position cache, used in layout models.
	int ptr_x, ptr_y;
	X11::getMousePosition(ptr_x, ptr_y);

	Generator::RangeWrap<int> range(head_nr, X11::getNumHeads());
	for (; ! range.is_end(); ++range) {
		if (! fsHead[*range]) {
			pekwm::rootWo()->getHeadInfoWithEdge(*range, gm);
			if (layoutOnHead(frame, win_layouter_types, parent,
					 gm, ptr_x, ptr_y)) {
				return;
			}
		}
	}

	// Failed to place the window, so put it in the top-left corner
	// trying to avoid heads with a fullscreen window on it.
	for (range.reset(); ! range.is_end(); ++range) {
		if (! fsHead[*range]) {
			break;
		}
	}

	pekwm::rootWo()->getHeadInfoWithEdge(*range, gm);
	frame->move(gm.x, gm.y);
}

bool
Workspaces::layoutOnHead(PWinObj *wo, int win_layouter_types, Window parent,
			 const Geometry &gm, int ptr_x, int ptr_y)
{
	if (win_layouter_types) {
		return layoutOnHeadTypes(wo, win_layouter_types, parent, gm,
					 ptr_x, ptr_y);
	}
	std::vector<WinLayouter*>::iterator it(_layout_models.begin());
	for (; it != _layout_models.end(); ++it) {
		if ((*it)->layout(wo, parent, gm, ptr_x, ptr_y)) {
			return true;
		}
	}
	return false;
}

/**
 * Place PWinObj on the given head using win_layouter_types instead of the
 * globally configured types.
 */
bool
Workspaces::layoutOnHeadTypes(PWinObj *wo, int win_layouter_types,
			      Window parent, const Geometry &gm,
			      int ptr_x, int ptr_y)
{
	enum WinLayouterType type;
	do {
		type =  static_cast<enum WinLayouterType>(
				win_layouter_types & WIN_LAYOUTER_MASK);
		win_layouter_map::iterator it(_win_layouters.find(type));
		if (it != _win_layouters.end()
		    && it->second->layout(wo, parent, gm, ptr_x, ptr_y)) {
			return true;

		}
		win_layouter_types >>= WIN_LAYOUTER_SHIFT;
	} while (win_layouter_types);
	return false;
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
	Frame *wo_frame = dynamic_cast<Frame*>(wo);
	iterator it;

	if (! raise
	    && wo_frame && wo_frame->getTransFor()
	    && wo_frame->getTransFor()->getLayer() == wo_frame->getLayer()) {
		// Lower only to the top of the transient_for window.
		it = find(wo_frame->getTransFor()->getParent());
		++it;
		top_obj = it!=_wobjs.end() ? *it : nullptr;
	} else {
		it = _wobjs.begin();
		for (; it != _wobjs.end(); ++it) {
			if (raise) {
				// If raising, make sure the inserted wo gets
				// below the first window in the next layer.
				if ((*it)->getLayer() > wo->getLayer()) {
					top_obj = *it;
					break;
				}
			} else if (wo->getLayer() <= (*it)->getLayer()) {
				// If lowering, put the window below the first
				// window with the same level.
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
			Frame *frame = dynamic_cast<Frame*>(*it);
			if (frame) {
				t_it = std::find(wo_frame->getTransBegin(),
						 wo_frame->getTransEnd(),
						 frame->getActiveClient());
				if (t_it != wo_frame->getTransEnd()) {
					winstack.push_back(frame);
					it = _wobjs.erase(it);
					continue;
				}
			}
			++it;
		}

		it = find(wo);
		++it;
		_wobjs.insert(it, winstack.begin()+1, winstack.end());
	}

	if (top_obj) {
		winstack.push_back(top_obj);
	} else {
		X11::raiseWindow(winstack.back()->getWindow());
	}

	std::vector<Window> wins;
	std::vector<PWinObj*>::reverse_iterator wit(winstack.rbegin());
	for (; wit != winstack.rend(); ++wit) {
		wins.push_back((*wit)->getWindow());
	}
	X11::stackWindows(wins);
}

//! @brief Removes a PWinObj from the stacking list.
void
Workspaces::remove(const PWinObj* wo)
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
		if (wo == it->getLastFocused(false)) {
			it->setLastFocused(nullptr);
		}
	}
}

//! @brief Hides all non-sticky Frames on the workspace.
void
Workspaces::hideAll(uint workspace)
{
	const_iterator it(_wobjs.begin());
	for (; it != _wobjs.end(); ++it) {
		if (! ((*it)->isSticky())
		    && ! (*it)->isHidden()
		    && ((*it)->getWorkspace() == workspace)) {
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
		if (! (*it)->isMapped()
		    && ! (*it)->isIconified()
		    && ! (*it)->isHidden()
		    && ((*it)->getWorkspace() == workspace)) {
			(*it)->mapWindow(); // don't restack ontop windows
			if ((*it)->isType(PWinObj::WO_FRAME)) {
				static_cast<Frame*>(*it)->updateDecor();
			}
		}
	}

	if (focus) {
		// Try to focus last focused window and if that
		// fails, get the top-most Frame if any and give it
		// focus.
		PWinObj *wo = _workspaces[workspace].getLastFocused(true);
		if (wo == nullptr) {
			wo = getTopFocusableWO(PWinObj::WO_FRAME);
		}

		if (wo) {
			// Render as focused
			if (wo->isType(PWinObj::WO_CLIENT)) {
				wo->getParent()->setFocused(true);
			} else {
				wo->setFocused(true);
			}

			// Get the active child if a frame, to get
			// correct focus behavior
			if (wo->isType(PWinObj::WO_FRAME)) {
				wo = static_cast<Frame*>(wo)->getActiveChild();
			}

			// Focus
			giveInputFocus(wo);
			PWinObj::setFocusedPWinObj(wo);

			// update the MRU list
			if (wo->isType(PWinObj::WO_CLIENT)) {
				Frame *frame =
					static_cast<Frame*>(wo->getParent());
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

/**
 * Raises a PWinObj and restacks windows.
 *
 * @return true if windows were restacked, else false.
 */
bool
Workspaces::raise(PWinObj* wo)
{
	iterator it = find(wo);
	if (it == _wobjs.end()) {
		return false;
	}
	if (handleFullscreenBeforeRaise(wo)) {
		it = find(wo);
	}
	_wobjs.erase(it);

	insert(wo, true /* raise */);
	return true;
}

/**
 * Swap position of wo_under and wo_over in the stacking order making wo_over
 * placed above wo_under.
 *
 * @return true if stacking was changed, false if objects belong to different
 *         layers.
 */
bool
Workspaces::swapInStack(PWinObj* wo_under, PWinObj* wo_over)
{
	if (wo_under->getLayer() != wo_under->getLayer()) {
		P_TRACE("not swapping in stack, not on the same layer");
		return false;
	}

	iterator it_under = find(wo_under);
	assert(it_under != _wobjs.end());
	iterator it_over = find(wo_over);
	assert(it_over != _wobjs.end());
	*it_under = wo_over;
	*it_over = wo_under;
	stackAt(it_over);
	stackAt(it_under);
	return true;
}

/**
 * Move wo_under in the stack and place it above wo.
 */
bool
Workspaces::stackAbove(PWinObj* wo_under, PWinObj* wo)
{
	if (wo_under->getLayer() != wo->getLayer()) {
		P_TRACE("not stacking above, not on the same layer");
		return false;
	}

	iterator it_under = find(wo_under);
	assert(it_under != _wobjs.end());
	_wobjs.erase(it_under);
	iterator it = find(wo);
	assert(it != _wobjs.end());
	it_under = _wobjs.insert(it + 1, wo_under);
	stackAt(it_under);
	return true;
}

/**
 * Get iterator for PWinObj from _wobjs.
 */
Workspaces::iterator
Workspaces::find(const PWinObj *wo)
{
	return std::find(_wobjs.begin(), _wobjs.end(), wo);
}

void
Workspaces::stackAt(iterator it)
{
	iterator it_above = it + 1;
	if (it_above == _wobjs.end()) {
		X11::raiseWindow((*it)->getWindow());
	} else {
		std::vector<Window> wins;
		wins.push_back((*it_above)->getWindow());
		wins.push_back((*it)->getWindow());
		X11::stackWindows(wins);
	}
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
		// which could reduce a couple of std::find() calls.
		//
		// But we want the higher fullscreen windows to _stay_ in
		// _wobjs, so that they can be the (top_obj) anchor point for
		// restacking. And since that anchor is most likely fullscreen
		// and hides everything else, no flickering.
		iterator it = find(*wo);
		if (it != _wobjs.end()) {
			_wobjs.erase(it);
			insert(*wo, true);
		}
	}
	return !fs_wobjs.empty();
}

/**
 * Lower a PWinObj and restacks windows.
 *
 * @return true if lowered, else false.
 */
bool
Workspaces::lower(PWinObj* wo)
{
	iterator it = find(wo);
	if (it == _wobjs.end()) {
		return false;
	}

	_wobjs.erase(it);
	insert(wo, false /* lower, raise is false */);
	return true;
}

/**
 * Restack wo relative to sibling using detail as position.
 */
void
Workspaces::restack(PWinObj* wo, PWinObj* sibling, long detail)
{
	bool restacked;
	if (sibling) {
		restacked = restackSibling(wo, sibling, detail);
	} else {
		restacked = restack(wo, detail);
	}

	if (restacked) {
		updateClientStackingList();
	}
}

bool
Workspaces::restack(PWinObj *wo, long detail)
{
	switch (detail) {
	case Above:
		return raise(wo);
	case Below:
		return lower(wo);
	case TopIf:
		return restackTopIf(wo);
	case BottomIf:
		return restackBottomIf(wo);
	case Opposite:
		if (restackTopIf(wo)) {
			return true;
		}
		return restackBottomIf(wo);
	default:
		P_TRACE("unsupported detail " << detail << " in "
			"_NET_RESTACK_WINDOW request");
		return false;
	}
}

/**
 * If any sibling occludes the window, the window is placed at
 * the top of the stack (for that layer).
 */
bool
Workspaces::restackTopIf(PWinObj *wo)
{
	for (iterator it = find(wo) + 1; it < _wobjs.end(); ++it) {
		if ((*it)->getLayer() != wo->getLayer()) {
			break;
		} else if (isOccluding((*it), wo)) {
			raise(wo);
			return true;
		}
	}
	return false;
}

/**
 * If the window occludes any sibling, the window is placed at the bottom of the
 * stack (for that layer).
 */
bool
Workspaces::restackBottomIf(PWinObj *wo)
{
	for (iterator it = find(wo) - 1; it >= _wobjs.begin(); --it) {
		if ((*it)->getLayer() != wo->getLayer()) {
			break;
		} else if (isOccluding(wo, (*it))) {
			lower(wo);
			return true;
		}
	}
	return false;
}

bool
Workspaces::restackSibling(PWinObj *wo, PWinObj *sibling, long detail)
{
	switch (detail) {
	case Above:
		return stackAbove(wo, sibling);
	case Below:
		return stackAbove(sibling, wo);
	case TopIf:
		if (isOccluding(sibling, wo)) {
			return raise(wo);
		}
		return false;
	case BottomIf:
		if (isOccluding(wo, sibling)) {
			return lower(wo);
		}
		return false;
	case Opposite:
		if (isOccluding(wo, sibling)) {
			return swapInStack(wo, sibling);
		} else if (isOccluding(sibling, wo)) {
			return swapInStack(sibling, wo);
		}
		return false;
	default:
		P_TRACE("unsupported detail " << detail << " in "
			"_NET_RESTACK_WINDOW request");
		return false;
	}
}

/**
 * Check if wo is above in the stacking order and has any part of
 * it's geometry in the area of sibling.
 */
bool
Workspaces::isOccluding(const PWinObj* wo, const PWinObj* sibling)
{
	iterator it_wo = find(wo);
	iterator it_sibling = find(sibling);
	if (it_wo == _wobjs.end() || it_sibling == _wobjs.end()) {
		return false;
	} else if (!wo->isMapped()) {
		// can't occlude if it is not mapped
		return false;
	}
	return it_wo > it_sibling
		&& sibling->getGeometry().isOverlap(wo->getGeometry());
}

PWinObj*
Workspaces::getLastFocused(uint workspace)
{
	if (workspace >= _workspaces.size()) {
		return nullptr;
	}
	return _workspaces[workspace].getLastFocused(true);
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

/**
 * Returns the first focusable PWinObj with the highest stacking.
 */
PWinObj*
Workspaces::getTopFocusableWO(uint type_mask)
{
	reverse_iterator r_it = _wobjs.rbegin();
	for (; r_it != _wobjs.rend(); ++r_it) {
		if (isFocusable(*r_it) && ((*r_it)->getType()&type_mask)) {
			return *r_it;
		}
	}
	return nullptr;
}

/**
 * Builds a list of all clients in stacking order, clients in the same
 * frame come after each other.
 */
void
Workspaces::buildClientList(std::vector<Window> &windows, bool report_all)
{
	Frame *frame;
	Client *client, *client_active;

	iterator it_f;
	const_iterator it_c;
	for (it_f = _wobjs.begin(); it_f != _wobjs.end(); ++it_f) {
		if (! (*it_f)->isType(PWinObj::WO_FRAME)) {
			continue;
		}

		frame = static_cast<Frame*>(*it_f);
		client_active = frame->getActiveClient();

		if (report_all) {
			for (it_c = frame->begin();
			     it_c != frame->end();
			     ++it_c) {
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
}

/**
 * Updates EWMH and pekwm client list hints.
 */
void
Workspaces::updateClientList(void)
{
	bool report_all = pekwm::config()->isReportAllClients();
	std::vector<Window> windows;
	buildClientList(windows, report_all);
	// previously, the lists where unset when they ended up empty
	// however some applications does not support this, one
	// example being tint2 on Debian Stretch
	X11::setWindows(X11::getRoot(), NET_CLIENT_LIST, windows);
	X11::setWindows(X11::getRoot(), NET_CLIENT_LIST_STACKING, windows);
	if (! report_all) {
		windows.clear();
		buildClientList(windows, true);
	}
	X11::setWindows(X11::getRoot(), PEKWM_CLIENT_LIST, windows);
}

/**
 * Updates the Ewmh Stacking list hint.
 */
void
Workspaces::updateClientStackingList(void)
{
	bool report_all = pekwm::config()->isReportAllClients();
	std::vector<Window> windows;
	buildClientList(windows, report_all);
	P_TRACE("updating _NET_CLIENT_LIST_STACKING with " << windows.size()
		<< " window(s)");
	X11::setWindows(X11::getRoot(), NET_CLIENT_LIST_STACKING, windows);
}

/**
 * Make sure window is inside screen boundaries. If window is in
 * fullscreen mode or maximized, resize to make sure it covers the new
 * screen.
 */
void
Workspaces::placeWoInsideScreen(PWinObj *wo)
{
	Geometry gm_before(wo->getX(), wo->getY(),
			   wo->getWidth(), wo->getHeight());
	Geometry gm_after(gm_before);

	Strut *strut = 0;
	bool maximized_virt = false;
	bool maximized_horz = false;
	if (wo->isType(PWinObj::WO_FRAME)) {
		Client *client = static_cast<Frame*>(wo)->getActiveClient();
		if (client) {
			strut = client->getStrut();
			maximized_virt = client->isMaximizedVert();
			maximized_horz = client->isMaximizedHorz();
		}
	}

	pekwm::rootWo()->placeInsideScreen(gm_after, strut, wo->isFullscreen(),
					   maximized_virt, maximized_horz);
	if (gm_before != gm_after) {
		wo->moveResize(gm_after.x, gm_after.y,
			       gm_after.width, gm_after.height);
	}
}

void
Workspaces::giveInputFocus(PWinObj *wo, bool force)
{
	if (wo->isFocusable()) {
		// check for current focused window object, warping to an
		// already focused window is confusing.
		if ((force || ! wo->isFocused())
		    && pekwm::config()->isWarpPointerOn(WARP_ON_FOCUS_CHANGE)
		    && wo != pekwm::rootWo()) {
			wo->warpPointer();
		}
		wo->giveInputFocus();
	}
}

/**
 * Searches for a PWinObj to focus, and then gives it input focus
 */
void
Workspaces::findWOAndFocus(PWinObj *search)
{
	PWinObj *focus = nullptr;
	if (PWinObj::windowObjectExists(search) && isFocusable(search)) {
		focus = search;
	} else {
		// search window object didn't exist, find candidate
		bool stacking = pekwm::config()->isOnCloseFocusStacking();
		focus = findWOAndFocusFind(stacking);
	}

	if (focus) {
		giveInputFocus(focus);
		findWOAndFocusRaise(focus);
	}  else if (! PWinObj::getFocusedPWinObj()) {
		pekwm::rootWo()->giveInputFocus();
		pekwm::rootWo()->setEwmhActiveWindow(None);
	}
}

void
Workspaces::findWOAndFocusRaise(PWinObj *wo)
{
	switch (pekwm::config()->getOnCloseFocusRaise()) {
	case ON_CLOSE_FOCUS_RAISE_ALWAYS:
		wo->raise();
		break;
	case ON_CLOSE_FOCUS_RAISE_IF_COVERED: {
		uint percent = overlapPercent(wo);
		if (percent > 50) {
			P_TRACE("raising " << wo << " " << percent
				 << "% overlap");
			wo->raise();
		} else {
			P_TRACE("NOT raising " << wo << " " << percent
				<< "% overlap");
		}
		break;
	}
	default:
		// do nothing
		break;
	};
}

PWinObj*
Workspaces::findWOAndFocusFind(bool stacking)
{
	if (!stacking) {
		PWinObj *wo = findWOAndFocusMRU();
		if (wo != nullptr) {
			return wo;
		}
	}
	return findWOAndFocusStacking();
}

PWinObj*
Workspaces::findWOAndFocusStacking()
{
	std::vector<PWinObj*>::reverse_iterator it(_wobjs.rbegin());
	for (; it != _wobjs.rend(); ++it) {
		if (! isFocusable(*it)
		    || ! (*it)->isType(PWinObj::WO_FRAME)) {
			continue;
		}

		switch ((*it)->getWinType()) {
		case WINDOW_TYPE_DESKTOP:
		case WINDOW_TYPE_UTILITY:
		case WINDOW_TYPE_DIALOG:
		case WINDOW_TYPE_NORMAL:
			return *it;
		default:
			/* do nothing */
			break;
		}
	}
	return nullptr;
}

PWinObj*
Workspaces::findWOAndFocusMRU()
{
	std::vector<Frame*>::iterator it(_mru.begin());
	for (; it != _mru.end(); ++it) {
		if (isFocusable(*it)) {
			return *it;
		}
	}
	return nullptr;
}

/**
 * Calculate how many percentage of PWinObj is covered by windows above it.
 */
uint
Workspaces::overlapPercent(PWinObj *wo)
{
	iterator it = find(wo);
	assert(it != _wobjs.end());

	GeometryOverlap overlap(wo->getGeometry());
	for (++it; it != _wobjs.end(); ++it) {
		if ((*it)->isMapped() && (*it)->isType(PWinObj::WO_FRAME)) {
			overlap.addOverlap((*it)->getGeometry());
		}
	}
	return overlap.getOverlapPercent();
}

/**
 * Find window object under the mouse, favouring higher stacked
 * windows over window below it
 */
PWinObj*
Workspaces::findUnderPointer(void)
{
	int x, y;
	X11::getMousePosition(x, y);

	reverse_iterator it;
	for (it = _wobjs.rbegin(); it != _wobjs.rend(); ++it) {
		if (isFocusable(*it) && (*it)->isUnder(x, y)) {
			return *it;
		}
	}
	return nullptr;
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
	if (wo->isType(PWinObj::WO_CLIENT)) {
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
		if (! (*it)->isType(PWinObj::WO_FRAME) ||
		    static_cast<Frame*>(*it)->isSkip(skip)) {
			continue; // only include frames and not having skip set
		}

		// check main direction, making sure it's at the right side we
		// check against the middle of the window as it gives a saner
		// feeling than the edges IMHO
		switch (dir) {
		case DIRECTION_UP:
			diff_main = wo_main
				- ((*it)->getY() + (*it)->getHeight() / 2);
			diff_pos = wo->getY() - (*it)->getY();
			break;
		case DIRECTION_DOWN:
			diff_main = ((*it)->getY() + (*it)->getHeight() / 2)
				- wo_main;
			diff_pos = (*it)->getBY() - wo->getBY();
			break;
		case DIRECTION_LEFT:
			diff_main = wo_main
				- ((*it)->getX() + (*it)->getWidth() / 2);
			diff_pos = wo->getX() - (*it)->getX();
			break;
		case DIRECTION_RIGHT:
			diff_main = ((*it)->getX() + (*it)->getWidth() / 2)
				- wo_main;
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
			if ((wo_sec < (*it)->getX())
			    || (wo_sec > (*it)->getRX())) {
				score += X11::getHeight() / 2;
			}
			score += abs (static_cast<long>(
						wo_sec - ((*it)->getX ()
						+ (*it)->getWidth () / 2)));

		} else {
			if ((wo_sec < (*it)->getY())
			    || (wo_sec > (*it)->getBY())) {
				score += X11::getWidth() / 2;
			}

			score += abs(static_cast<long>(
						wo_sec - ((*it)->getY ()
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
			if (! (*n_it)->isSkip(mask)
			    && (! mapped || (*n_it)->isMapped())) {
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
			if (! (*n_it)->isSkip(mask)
			    && (! mapped || (*n_it)->isMapped())) {
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
