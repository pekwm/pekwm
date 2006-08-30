//
// Workspaces.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "Workspaces.hh"

#include "ScreenInfo.hh"
#include "Atoms.hh"
#include "Config.hh"
#include "WindowObject.hh"
#include "Frame.hh"
#include "FrameWidget.hh"
#include "Client.hh" // For isSkip()

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

extern "C" {
#include <X11/Xatom.h> // for XA_WINDOW
}

using std::list;
using std::vector;

Workspaces::Workspaces(ScreenInfo *scr, Config *cfg, EwmhAtoms *ewmh,
											 unsigned int number) :
_scr(scr), _cfg(cfg), _ewmh(ewmh),
_active(0)
{
	if (number < 1)
		number = 1;

	// create new workspaces
	for (unsigned int i = 0; i < number; ++i)
		_workspace_list.push_back(new Workspace("", i));
}

Workspaces::~Workspaces()
{
	vector<Workspace*>::iterator it = _workspace_list.begin();
	for (; it != _workspace_list.end(); ++it)
		delete *it;

	_workspace_list.clear();
}

//! @fn    setNumWorkspaces(unsigned int number)
//! @brief Sets number of workspaces
//! @param number New number of workspacs
void
Workspaces::setNumber(unsigned int number)
{
	if (number < 1)
		number = 1;

	if (number == _workspace_list.size())
		return; // no need to change number of workspaces to the current number

	unsigned int before = _workspace_list.size();
	if (_active >= number)
		_active = number - 1;

	// We have more workspaces than we want, lets remove the last ones
	if (before > number) {
		list<WindowObject*>::iterator it = _wo_list.begin();
		for (; it != _wo_list.end(); ++it) {
			if ((*it)->getWorkspace() > (number - 1))
				(*it)->setWorkspace(number - 1);
		}

		for (unsigned int i = before - 1; i >= number; --i)
			delete _workspace_list[i];

		_workspace_list.resize(number, NULL);

	} else { // We need more workspaces, lets create some
		for (unsigned int i = before; i < number; ++i) {
			_workspace_list.push_back(new Workspace("", i));
		}
	}

	// Tell the rest of the world how many workspaces we have.
	XChangeProperty(_scr->getDisplay(), _scr->getRoot(),
									_ewmh->getAtom(NET_NUMBER_OF_DESKTOPS),
									XA_CARDINAL, 32, PropModeReplace,
									(unsigned char *) &number, 1);
}

//! @fn    void insert(WindowObject* wo)
//! @brief Adds a WindowObject to the stacking list.
//! @param raise Defaults to true, wheter to check for bottom or top.
void
Workspaces::insert(WindowObject* wo, bool raise)
{
	bool inserted = false;

	list<WindowObject*>::iterator it = _wo_list.begin();
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
		XRaiseWindow(_scr->getDisplay(), wo->getWindow());
	}
}

//! @fn    void remove(WindowObject* wo)
//! @brief Removes a WindowObject from the stacking list.
void
Workspaces::remove(WindowObject* wo)
{
	_wo_list.remove(wo);

	// remove from last focused
	vector<Workspace*>::iterator it = _workspace_list.begin();
	for (; it != _workspace_list.end(); ++it) {
		if (wo == (*it)->getLastFocused())
			(*it)->setLastFocused(NULL);
	}
}

//! @fn    void hideAll(unsigned int workspace)
//! @brief Hides all non-sticky Frames on the workspace.
void
Workspaces::hideAll(unsigned int workspace)
{
	list<WindowObject*>::iterator it = _wo_list.begin();
	for (; it != _wo_list.end(); ++it) {
		if (!(*it)->isSticky() && ((*it)->getWorkspace() == workspace))
			(*it)->unmapWindow();
	}
}

//! @fn    void unhideAll(unsigned int workspace, bool focus)
//! @brief Unhides all hidden WindowObjects on the workspace.
void
Workspaces::unhideAll(unsigned int workspace, bool focus)
{
	_active = workspace;

	list<WindowObject*>::iterator it = _wo_list.begin();
	for (; it != _wo_list.end(); ++it) {
		if (!(*it)->isMapped() && !(*it)->isIconified() &&
				((*it)->getWorkspace() == workspace))
			(*it)->mapWindow(); // don't restack ontop windows
	}

	if (focus) {
		WindowObject *wo = _workspace_list[workspace]->getLastFocused();
		
		if (!wo)
			wo = getTopWO(WindowObject::WO_FRAME); // TO-DO: Bitmask instead?

		if (wo) // Activate the WindowObject
			wo->giveInputFocus();
	}
}

//! @fn    void raise(WindowObject* wo)
//! @brief Raises a WindowObject and restacks windows.
void
Workspaces::raise(WindowObject* wo)
{
	list<WindowObject*>::iterator it =
		find(_wo_list.begin(), _wo_list.end(), wo);

	if (it == _wo_list.end()) // no Frame to raise.
		return;
	_wo_list.erase(it);

	insert(wo, true); // reposition and restack
}

//! @fn    void lower(WindowObject* wo)
//! @brief Lower a WindowObject and restacks windows.
void
Workspaces::lower(WindowObject* wo)
{
	list<WindowObject*>::iterator it =
		find(_wo_list.begin(), _wo_list.end(), wo);

	if (it == _wo_list.end()) // no Frame to raise.
		return;
	_wo_list.erase(it);

	insert(wo, false); // reposition and restack
}

//! @fn    void stackAbove(WindowObject* wo, Window win)
//! @brief Places the WindowObject above the window win
//! @param wo WindowObject to place.
//! @param win Window to place Frame above.
//! @param restack Restack the X windows, defaults to true.
void
Workspaces::stackAbove(WindowObject *wo, Window win, bool restack)
{
	list<WindowObject*>::iterator old_pos =
		find(_wo_list.begin(), _wo_list.end(), wo);

	if (old_pos != _wo_list.end()) {
		list<WindowObject*>::iterator it = _wo_list.begin();
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

//! @fn    void stackBelow(WindowObject* wo, Window win)
//! @brief Places the WindowObject below the window win
//! @param wo WindowObject to place.
//! @param win Window to place Frame under
//! @param restack Restack the X windows, defaults to true
void
Workspaces::stackBelow(WindowObject* wo, Window win, bool restack)
{
	list<WindowObject*>::iterator old_pos =
		find(_wo_list.begin(), _wo_list.end(), wo);

	if (old_pos != _wo_list.end()) {
		list<WindowObject*>::iterator it = _wo_list.begin();
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


//! @fn    WindowObject* getLastFocused(unsigned int workspace); 
//! @brief
WindowObject*
Workspaces::getLastFocused(unsigned int workspace)
{
	if (workspace >= _workspace_list.size())
		return NULL;
	return _workspace_list[workspace]->getLastFocused();
}

//! @fn    void setLastFocused(unsigned int workspace, WindowObject* wo);
//! @brief
void
Workspaces::setLastFocused(unsigned int workspace, WindowObject* wo)
{
	if (workspace >= _workspace_list.size())
		return;
	_workspace_list[workspace]->setLastFocused(wo);
}

//! @fn    void stackWinUnderWin(Window win_over, Window win_under)
//! @brief Helper function to stack a window below another
//! @param win_over Window to place win_under under
//! @param win_under Window to place under win_over
void
Workspaces::stackWinUnderWin(Window over, Window under)
{
	if (over == under)
		return;

	Window windows[2] = { over, under };
	XRestackWindows(_scr->getDisplay(), windows, 2);
}

// MISC METHODS

//! @fn    void getTopWO(unsigned int type_mask)
//! @brief Returns the WindowObject with the highest stacking
WindowObject*
Workspaces::getTopWO(unsigned int type_mask)
{
	list<WindowObject*>::reverse_iterator r_it = _wo_list.rbegin();
	for (; r_it != _wo_list.rend(); ++r_it) {
		if ((*r_it)->isMapped() && ((*r_it)->getType()&type_mask))
			return (*r_it);
	}
	return NULL;
}

//! @fn    void updateClientStackingList(bool client, bool stacking)
//! @brief Updates the Ewmh Client list and Stacking list hint.
void
Workspaces::updateClientStackingList(bool client, bool stacking)
{
	Frame *it_frame;
	list<Window> win_list;

	// Find clients we are going to include in the list
	list<WindowObject*>::iterator it = _wo_list.begin();
	for (; it != _wo_list.end(); ++it) {
		if ((*it)->getType() != WindowObject::WO_FRAME)
			continue;

		it_frame = (Frame*) (*it);
		if (!it_frame->getActiveClient()->skipTaskbar())
			win_list.push_back(it_frame->getActiveClient()->getWindow());
	}

	if (!win_list.size())
		return;

	Window *windows = new Window[win_list.size()];
	copy(win_list.begin(), win_list.end(), windows);

	if (client) {
		XChangeProperty(_scr->getDisplay(), _scr->getRoot(),
										_ewmh->getAtom(NET_CLIENT_LIST),
										XA_WINDOW, 32, PropModeReplace,
										(unsigned char*) windows, win_list.size());
	}
	if (stacking) {
		XChangeProperty(_scr->getDisplay(), _scr->getRoot(),
										_ewmh->getAtom(NET_CLIENT_LIST_STACKING),
										XA_WINDOW, 32, PropModeReplace,
										(unsigned char*) windows, win_list.size());
	}

	delete [] windows;
}

//! @fn    void checkFrameSnap(Geometry &gm, Frame *frame)
//! @brief Tries to snap the frame against a nearby frame.
void
Workspaces::checkFrameSnap(Geometry &f_gm, Frame *frame)
{
	Frame *it_frame;
	FrameWidget *fw = frame->getFrameWidget(); // convenience

	Geometry gm = f_gm;
	gm.height = fw->getHeight(); // make sure it works with shaded frame too

	int x = gm.x + fw->getWidth();
	int y = gm.y + fw->getHeight();
	int snap = _cfg->getFrameSnap();

	bool x_snapped, y_snapped;

	list<WindowObject*>::reverse_iterator it = _wo_list.rbegin();
	for (; it != _wo_list.rend(); ++it) {
		if (!(*it)->isMapped() || ((*it)->getType() != WindowObject::WO_FRAME))
			continue;

		it_frame = (Frame*) (*it);
		if ((it_frame == frame) || it_frame->isSkip(SKIP_FRAME_SNAP))
			continue;

		x_snapped = y_snapped = false;
		fw = it_frame->getFrameWidget();

		// check snap
		if ((x >= (fw->getX() - snap)) && (x <= (fw->getX() + snap))) {
			if (isBetween(gm.y, y, fw->getY(), fw->getY() + fw->getHeight())) {
				f_gm.x = fw->getX() - gm.width;
				if (y_snapped)
					break;
				x_snapped = true;
			}
		} else if ((gm.x >= signed(fw->getX() + fw->getWidth() - snap)) &&
							 (gm.x <= signed(fw->getX() + fw->getWidth() + snap))) {
			if (isBetween(gm.y, y, fw->getY(), fw->getY() + fw->getHeight())) {
				f_gm.x = fw->getX() + fw->getWidth();
				if (y_snapped)
					break;
				x_snapped = true;
			}
		}

		if (y >= (fw->getY() - snap) && (y <= fw->getY() + snap)) {
			if (isBetween(gm.x, x, fw->getX(), fw->getX() + fw->getWidth())) {
				f_gm.y = fw->getY() - gm.height;
				if (x_snapped)
					break;
				y_snapped = true;
			}
		} else if ((gm.y >= signed(fw->getY() + fw->getHeight() - snap)) &&
							 (gm.y <= signed(fw->getY() + fw->getHeight() + snap))) {
			if (isBetween(gm.x, x, fw->getX(), fw->getX() + fw->getWidth())) {
				f_gm.y = fw->getY() + fw->getHeight();
				if (x_snapped)
					break;
				y_snapped = true;
			}
		}
	}
}

// PLACEMENT ROUTINES

//! @fn    void placeFrame(Frame *frame, Geometry &gm)
//! @brief Tries to place the Frame.
void
Workspaces::placeFrame(Frame *frame, Geometry &gm)
{
	if (!frame)
		return;

	list<unsigned int> *pl = _cfg->getPlacementModelList();
	list<unsigned int>::iterator it = pl->begin();

	for (bool placed = false; !placed && (it != pl->end()); ++it) {
		switch(*it) {
		case SMART:
			placed = placeSmart(frame, gm);
			break;
		case MOUSE_CENTERED:
			placed = placeCenteredUnderMouse(gm);
			break;
		case MOUSE_TOP_LEFT:
			placed = placeTopLeftUnderMouse(gm);
			break;
		default:
			// do nothing
			break;
		}
	}
}

//! @fn    bool placeSmart(WindowObject* wo_place, Geometry &gm)
//! @brief Tries to find empty space to place the client in
//! @return true if client got placed, else false
//! @todo What should we do about Xinerama as when we don't have it enabled we care about the struts.
bool
Workspaces::placeSmart(WindowObject* wo_place, Geometry &gm)
{
	WindowObject *wo;
	bool placed = false;

	int step_x = (_cfg->getPlacementLtR()) ? 1 : -1;
	int step_y = (_cfg->getPlacementTtB()) ? 1 : -1;
	int start_x, start_y, test_x = 0, test_y = 0;

	Geometry head;
#ifdef XINERAMA
	_scr->getHeadInfo(_scr->getCurrHead(), head);
#else //! XINERAMA
	head.x = _scr->getStrut()->left;
	head.y = _scr->getStrut()->top;
	head.width = _scr->getWidth() - head.x  - _scr->getStrut()->right;
	head.height = _scr->getHeight() - head.y - _scr->getStrut()->bottom;
#endif // XINERAMA

	start_x = (_cfg->getPlacementLtR())
		? (head.x)
		: (head.x + head.width - gm.width);
	start_y = (_cfg->getPlacementTtB())
		? (head.y)
		: (head.y + head.height - gm.height);

	if (_cfg->getPlacementRow()) { // row placement
		test_x = start_x;
		while (!placed && (_cfg->getPlacementLtR()
										 ? ((test_x + gm.width) < (head.x + head.width))
										 : (test_x >= head.x))) {
			test_y = start_y;
			while (!placed && (_cfg->getPlacementTtB()
												 ? ((test_y + gm.height) < (head.y + head.height))
												 : (test_y >= head.y))) {
				// see if we can place the window here
				if ((wo = isEmptySpace(test_x, test_y, wo_place, gm))) {
					placed = false;
					test_y = _cfg->getPlacementTtB()
 						? (wo->getY() + wo->getHeight()) : (wo->getY() - gm.height);
				} else {
					placed = true;
					gm.x = test_x;
					gm.y = test_y;
				}
			}
			test_x += step_x;
		}
	} else { // column placement
		test_y = start_y;
		while (!placed && (_cfg->getPlacementTtB()
											 ? ((test_y + gm.height) < (head.y + head.height))
											 : (test_y >= head.y))) {
			test_x = start_x;
			while (!placed && (_cfg->getPlacementLtR()
												 ? ((test_x + gm.width) < (head.x + head.width))
												 : (test_x >= head.x))) {
				// see if we can place the window here
				if ((wo = isEmptySpace(test_x, test_y, wo_place, gm))) {
					placed = false;
					test_x = _cfg->getPlacementLtR()
						? (wo->getX() + wo->getWidth()) : (wo->getX() - gm.width);
				} else {
					placed = true;
					gm.x = test_x;
					gm.y = test_y;
				}
			}
			test_y += step_y;
		}
	}

	return placed;
}

//! @fn    bool placeCenteredUnderMouse(void)
//! @brief Places the client centered under the mouse
bool
Workspaces::placeCenteredUnderMouse(Geometry &gm)
{
	int mouse_x, mouse_y;
	_scr->getMousePosition(mouse_x, mouse_y);

	gm.x = mouse_x - (gm.width / 2);
	gm.y = mouse_y - (gm.height / 2);

	placeInsideScreen(gm); // make sure it's within the screens border

	return true;
}

//! @fn    bool placeTopLeftUnderMouse(void)
//! @brief Places the client like the menu gets placed
bool
Workspaces::placeTopLeftUnderMouse(Geometry &gm)
{
	int mouse_x, mouse_y;
	_scr->getMousePosition(mouse_x, mouse_y);

	gm.x = mouse_x;
	gm.y = mouse_y;

	placeInsideScreen(gm); // make sure it's within the screens border

	return true;
}

//! @fn    void placeInsideScreen(Geometry &gm)
//! @brief Makes sure the window is inside the screen.
void
Workspaces::placeInsideScreen(Geometry &gm)
{
#ifdef XINERAMA
	Geometry head;
	_scr->getHeadInfo(_scr->getCurrHead(), head);

	if (gm.x < head.x)
		gm.x = head.x;
	else if ((gm.x + gm.width) > (head.x + head.width))
		gm.x = head.x + head.width - gm.width;

	if (gm.y < head.y)
		gm.y = head.y;
	else if ((gm.y + gm.height) > (head.y + head.height))
		gm.y = head.y + head.height - gm.height;
#else // !XINERAMA
	if (gm.x < 0)
		gm.x = 0;
	else if ((gm.x + gm.width) > _scr->getWidth())
		gm.x = _scr->getWidth() - gm.width;

	if (gm.y < 0)
		gm.y = 0;
	else if ((gm.y + gm.height) > _scr->getHeight())
		gm.y = _scr->getHeight() - gm.height;
#endif // XINERAMA
}

//! @fn    WindowObject* isEmptySpace(int x, int y, const WindowObject* wo, const Geometry &gm)
//! @brief
WindowObject*
Workspaces::isEmptySpace(int x, int y, const WindowObject* wo, const Geometry &gm)
{
	if (!wo)
		return NULL;

	// say that it's placed, now check if we are wrong!
	list<WindowObject*>::iterator it = _wo_list.begin();
	for (; it != _wo_list.end(); ++it) {
		if (wo == (*it))
			continue; // we don't wanna take ourself into account

		// Make sure clients are visible and _not_ iconified. This maybe doesn't
		// make sense but to "hide" wo's from placement I set _iconified to true
		if ((*it)->isMapped() && !(*it)->isIconified() &&
				((*it)->getLayer() != LAYER_DESKTOP)) {
			// check if we are "intruding" on some other window's place
			if (((*it)->getX() < signed(x + gm.width)) &&
					(signed((*it)->getX() + (*it)->getWidth()) > x) &&
					((*it)->getY() < signed(y + gm.height)) &&
					(signed((*it)->getY() + (*it)->getHeight()) > y)) {
				return (*it);
			}
		}
	}

	return NULL; // we passed the test, no frames in the way
}
