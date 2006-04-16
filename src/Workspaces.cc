//
// Workspaces.cc for pekwm
// Copyright (C) 2002-2004 Claes Nasten <pekdo at pekdon dot net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "Workspaces.hh"

#include "PScreen.hh"
#include "Atoms.hh"
#include "ParseUtil.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "Frame.hh"
#include "Client.hh" // For isSkip()
#include "Viewport.hh"

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
#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

// Workspaces::Workspace

Workspaces::Workspace::Workspace(const std::string &name, uint number,
																 const std::list<PWinObj*> &wo_list) :
_name(name), _number(number),
_viewport(NULL), _wo_list(wo_list), _last_focused(NULL)
{
	_viewport = new Viewport(number, _wo_list);
}

Workspaces::Workspace::~Workspace(void)
{
	delete _viewport;
}

// Workspaces

Workspaces *Workspaces::_instance = NULL;


//! @brief Workpsaces constructor
Workspaces::Workspaces(uint number) :
_active(0), _previous(0)
{
#ifdef DEBUG
	if (_instance != NULL) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "Workspaces(" << this << ")::Workspaces(" << number << ")" << endl
				 << " *** _instance allready set: " << _instance << endl;
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
		_workspace_list.push_back(new Workspace("", i, _wo_list));
	}
}

//! @brief Workspaces destructor
Workspaces::~Workspaces(void)
{
	vector<Workspace*>::iterator it(_workspace_list.begin());
	for (; it != _workspace_list.end(); ++it) {
		delete *it;
	}

	_instance = NULL;
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

		_workspace_list.resize(number, NULL);

	} else { // We need more workspaces, lets create some
		for (uint i = before; i < number; ++i) {
			_workspace_list.push_back(new Workspace("", i, _wo_list));
		}
	}

	// Tell the rest of the world how many workspaces we have.
	XChangeProperty(PScreen::instance()->getDpy(),
									PScreen::instance()->getRoot(),
									EwmhAtoms::instance()->getAtom(NET_NUMBER_OF_DESKTOPS),
									XA_CARDINAL, 32, PropModeReplace,
									(uchar *) &number, 1);

	// make sure we aren't on an non-existent workspace
	if (number <= _active) {
		setWorkspace(number - 1, true);
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

	// save the focused window object
	setLastFocused(_active, PWinObj::getFocusedPWinObj());
	PWinObj::setFocusedPWinObj(NULL);

	// switch workspace
	hideAll(_active);
	AtomUtil::setLong(PScreen::instance()->getRoot(),
										EwmhAtoms::instance()->getAtom(NET_CURRENT_DESKTOP),
										num);
	unhideAll(num, focus);

	PScreen::instance()->ungrabServer(true);
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

	switch (direction) {
	case WORKSPACE_LEFT:
	case WORKSPACE_PREV:
		dir = 1;

		if (_active > 0) {
			workspace = _active - 1;
		} else if (direction == WORKSPACE_PREV) {
			workspace = _workspace_list.size() - 1;
		} else {
			switched = false;
		}
		break;
	case WORKSPACE_NEXT:
	case WORKSPACE_RIGHT:
		dir = -1;

		if ((_active + 1) < _workspace_list.size()) {
			workspace = _active + 1;
		} else if (direction == WORKSPACE_NEXT) {
			workspace = 0;
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

	int x, y, warp;
	PScreen::instance()->getMousePosition(x, y);

	warp = Config::instance()->getScreenEdgeSize();
	warp = (warp > 0) ? (warp * 2) : 2;

	if (dir != 0) {
		if (dir > 0) {
			x = PScreen::instance()->getWidth() - warp;
		} else {
			x = warp;
		}

		// warp pointer
		XWarpPointer(PScreen::instance()->getDpy(), None,
								 PScreen::instance()->getRoot(),
								 0, 0, 0, 0, x, y);
	}

	// set workpsace
	setWorkspace(num, true);

	return true;
}

//! @brief Adds a PWinObj to the stacking list.
//! @param wo PWinObj to insert
//! @param raise Defaults to true, wheter to check for bottom or top.
void
Workspaces::insert(PWinObj* wo, bool raise)
{
	bool inserted = false;

	list<PWinObj*>::iterator it(_wo_list.begin());
	for (; it != _wo_list.end() && !inserted; ++it) {
		if (raise
				? (wo->getLayer() < (*it)->getLayer())
				: (wo->getLayer() <= (*it)->getLayer())) {
			_wo_list.insert(it, wo);

			stackWinUnderWin((*it)->getWindow(), wo->getWindow());

			inserted = true;
		}
	}

	if (!inserted) {
		_wo_list.push_back(wo);
		// we can't do raise here as it'll end up in an recursive loop
		// if we use Workspaces::raise to restack the PWinObj.
		XRaiseWindow(PScreen::instance()->getDpy(), wo->getWindow());
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
		if (wo == (*it)->getLastFocused())
			(*it)->setLastFocused(NULL);
	}
}

//! @brief Hides all non-sticky Frames on the workspace.
void
Workspaces::hideAll(uint workspace)
{
	list<PWinObj*>::iterator it(_wo_list.begin());
	for (; it != _wo_list.end(); ++it) {
		if (((*it)->isSticky() == false) && ((*it)->isHidden() == false) &&
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
		if (!(*it)->isMapped() && !(*it)->isIconified() && !(*it)->isHidden()
				&& ((*it)->getWorkspace() == workspace)) {
			(*it)->mapWindow(); // don't restack ontop windows
		}
	}

	// Try to focus last focused window and if that fails we get the top-most
	// Frame if any and give it focus.
	if (focus) {
		PWinObj *wo = _workspace_list[workspace]->getLastFocused();
		if (!wo || !PWinObj::windowObjectExists(wo))
			wo = getTopWO(PWinObj::WO_FRAME);

		if (wo) {
			if (wo->getType() == PWinObj::WO_FRAME) {
				wo = static_cast<Frame*>(wo)->getActiveChild();
			}

			wo->giveInputFocus();
			PWinObj::setFocusedPWinObj(wo);
		}
		
		// If focusing fails, focus the root window.
		if (!PWinObj::getFocusedPWinObj()
				|| !PWinObj::getFocusedPWinObj()->isMapped()) {
			PWinObj::getRootPWinObj()->giveInputFocus();
		}
	}

	_workspace_list[_active]->getViewport()->hintsUpdate();
}

//! @brief Raises a PWinObj and restacks windows.
void
Workspaces::raise(PWinObj* wo)
{
	list<PWinObj*>::iterator it(find(_wo_list.begin(), _wo_list.end(), wo));

	if (it == _wo_list.end()) // no Frame to raise.
		return;
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
				if (restack)
					stackWinUnderWin((*--it)->getWindow(), wo->getWindow());
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

				if (restack)
					stackWinUnderWin((*it)->getWindow(), wo->getWindow());
				break;
			}
		}
	}
}


//! @brief
PWinObj*
Workspaces::getLastFocused(uint workspace)
{
	if (workspace >= _workspace_list.size())
		return NULL;
	return _workspace_list[workspace]->getLastFocused();
}

//! @brief
void
Workspaces::setLastFocused(uint workspace, PWinObj* wo)
{
	if (workspace >= _workspace_list.size())
		return;
	_workspace_list[workspace]->setLastFocused(wo);
}

//! @brief Helper function to stack a window below another
//! @param win_over Window to place win_under under
//! @param win_under Window to place under win_over
void
Workspaces::stackWinUnderWin(Window over, Window under)
{
	if (over == under)
		return;

	Window windows[2] = { over, under };
	XRestackWindows(PScreen::instance()->getDpy(), windows, 2);
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
	return NULL;
}

//! @brief Updates the Ewmh Client list and Stacking list hint.
void
Workspaces::updateClientStackingList(bool client, bool stacking)
{
	Frame *it_frame;
	list<Window> win_list;

	// Find clients we are going to include in the list
	list<PWinObj*>::iterator it(_wo_list.begin());
	for (; it != _wo_list.end(); ++it) {
		if ((*it)->getType() != PWinObj::WO_FRAME)
			continue;

		it_frame = (Frame*) (*it);
		if (!static_cast<Client*>(it_frame->getActiveChild())->skipTaskbar()) {
			win_list.push_back(it_frame->getActiveChild()->getWindow());
		}
	}

	if (!win_list.size())
		return;

	Window *windows = new Window[win_list.size()];
	copy(win_list.begin(), win_list.end(), windows);

	if (client) {
		AtomUtil::setWindows(PScreen::instance()->getRoot(),
												 EwmhAtoms::instance()->getAtom(NET_CLIENT_LIST),
												 windows, win_list.size());
	}
	if (stacking) {
		AtomUtil::setWindows(PScreen::instance()->getRoot(),
												 EwmhAtoms::instance()->getAtom(NET_CLIENT_LIST_STACKING),
												 windows, win_list.size());
	}

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
		while (!placed && (Config::instance()->getPlacementTtB()
											 ? ((test_y + wo_height) <= (head.y + head.height))
											 : (test_y >= head.y))) {
			test_x = start_x;
			while (!placed && (Config::instance()->getPlacementLtR()
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
		while (!placed && (Config::instance()->getPlacementLtR()
										 ? ((test_x + wo_width) <= (head.x + head.width))
										 : (test_x >= head.x))) {
			test_y = start_y;
			while (!placed && (Config::instance()->getPlacementTtB()
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
	if (wo_s != NULL) {
		wo->move(wo_s->getX() + wo_s->getWidth() / 2 - wo->getWidth() / 2,
						 wo_s->getY() + wo_s->getHeight() / 2 - wo->getHeight() / 2);
		return true;
	}

	return false;
}

//! @brief Makes sure the window is inside the screen.
void
Workspaces::placeInsideScreen(Geometry &gm)
{
	Geometry head;
	PScreen::instance()->getHeadInfoWithEdge(PScreen::instance()->getCurrHead(),
																					 head);

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
	if (wo == NULL) {
		return NULL;
	}

	// say that it's placed, now check if we are wrong!
	list<PWinObj*>::iterator it(_wo_list.begin());
	for (; it != _wo_list.end(); ++it) {
		if (wo == (*it))
			continue; // we don't wanna take ourself into account

		// Make sure clients are visible and _not_ iconified. This maybe doesn't
		// make sense but to "hide" wo's from placement I set _iconified to true
		if ((*it)->isMapped() && !(*it)->isIconified() &&
				((*it)->getLayer() != LAYER_DESKTOP)) {
			// check if we are "intruding" on some other window's place
			if (((*it)->getX() < signed(x + wo->getWidth())) &&
					(signed((*it)->getX() + (*it)->getWidth()) > x) &&
					((*it)->getY() < signed(y + wo->getHeight())) &&
					(signed((*it)->getY() + (*it)->getHeight()) > y)) {
				return (*it);
			}
		}
	}

	return NULL; // we passed the test, no frames in the way
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

	PWinObj *found_wo = NULL;

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
		if ((wo == (*it)) || ((*it)->isMapped() == false)) {
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
			return NULL; // no direction to search
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

