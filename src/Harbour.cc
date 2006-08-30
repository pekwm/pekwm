//
// Harbour.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef HARBOUR

#include "Harbour.hh"

#include "ScreenInfo.hh"
#include "Config.hh"
#include "WindowObject.hh"
#include "DockApp.hh"
#include "Workspaces.hh"
#ifdef MENUS
#include "BaseMenu.hh"
#include "HarbourMenu.hh"
#endif // MENUS

#include <algorithm>
#include <functional>

using std::list;
using std::mem_fun;
#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

Harbour::Harbour(ScreenInfo *s, Config *c, Theme *t, Workspaces *w) :
_scr(s), _cfg(c), _theme(t), _workspaces(w),
#ifdef MENUS
_harbour_menu(NULL),
#endif // MENUS
_size(0),
_last_button_x(0), _last_button_y(0)
{
#ifdef MENUS
	_harbour_menu = new HarbourMenu(_scr, _theme, this, _workspaces);
#endif // MENUS
}

Harbour::~Harbour()
{
	removeAllDockApps();
#ifdef MENUS
	if (_harbour_menu)
		delete _harbour_menu;
#endif // MENUS
}

//! @fn    void addDockApp(DockApp *da)
//! @brief Adds a DockApp to the Harbour
void
Harbour::addDockApp(DockApp *da)
{
	if (!da)
		return;
	_dockapp_list.push_back(da);

	da->setLayer(_cfg->getHarbourOntop() ? LAYER_DOCK : LAYER_DESKTOP);
	_workspaces->insert(da); // add the dockapp to the stacking list

	placeDockApp(da); // place it in a empty space
	if (!da->isMapped()) // make sure it's visible
		da->mapWindow();

	updateHarbourSize();
}

//! @fn    void removeDockApp(DockApp *da)
//! @brief Removes a DockApp from the Harbour
void
Harbour::removeDockApp(DockApp *da)
{
	if (!da)
		return;

	list<DockApp*>::iterator it =
		find(_dockapp_list.begin(), _dockapp_list.end(), da);

	if (it != _dockapp_list.end()) {
		_dockapp_list.remove(da);
		_workspaces->remove(da); // remove the dockapp to the stacking list
		delete da;
	}

	updateHarbourSize();
}

//! @fn    void removeAllDockApps(void)
//! @brief Removes all DockApps from the Harbour
void
Harbour::removeAllDockApps(void)
{
	list<DockApp*>::iterator it = _dockapp_list.begin();
	for (; it != _dockapp_list.end(); ++it) {
		_workspaces->remove(*it); // remove the dockapp to the stacking list
		delete (*it);
	}
	_dockapp_list.clear();
}

//! @fn    DockApp* findDockApp(Window win);
//! @brief Tries to find a dockapp which uses the window win
DockApp*
Harbour::findDockApp(Window win)
{
	DockApp *dockapp = NULL;

	list<DockApp*>::iterator it = _dockapp_list.begin();
	for (; it != _dockapp_list.end(); ++it) {
		if ((*it)->findDockApp(win)) {
			dockapp = (*it);
			break;
		}
	}

	return dockapp;
}

//! @fn    DockApp* findDockAppFromFrame(Window win);
//! @brief Tries to find a dockapp which has the window win as frame.
DockApp*
Harbour::findDockAppFromFrame(Window win)
{
	DockApp *dockapp = NULL;

	list<DockApp*>::iterator it = _dockapp_list.begin();
	for (; it != _dockapp_list.end(); ++it) {
		if ((*it)->findDockAppFromFrame(win)) {
			dockapp = (*it);
			break;
		}
	}

	return dockapp;
}

//! @fn    void restack(void)
//! @brief Lowers or Raises all the DockApps in the harbour.
void
Harbour::restack(void)
{
	if (!_dockapp_list.size())
		return;

	list<DockApp*>::iterator it = _dockapp_list.begin();
	for (; it != _dockapp_list.end(); ++it) {
		if (_cfg->getHarbourOntop())
			_workspaces->raise(*it);
		else
			_workspaces->lower(*it);
	}
}

//! @fn    void rearrange(void)
//! @brief Goes through the DockApp list and places the dockapp.
void
Harbour::rearrange(void)
{
	if (!_dockapp_list.size())
		return;

	list<DockApp*>::iterator it = _dockapp_list.begin();
	for (; it != _dockapp_list.end(); ++it)
		placeDockApp(*it);
}

//! @fn    void loadTheme(void)
//! @brief Repaints all dockapps with the new theme
void
Harbour::loadTheme(void)
{
	if (!_dockapp_list.size())
		return;

	for_each(_dockapp_list.begin(), _dockapp_list.end(),
					 mem_fun(&DockApp::loadTheme));
}

//! @fn    void updateHarbourSize(void)
//! @brief Updates the harbour max size variable.
void
Harbour::updateHarbourSize(void)
{
	_size = 0;

	if (_dockapp_list.size()) {
		list<DockApp*>::iterator it = _dockapp_list.begin();

		for (; it != _dockapp_list.end(); ++it) {
			switch (_cfg->getHarbourPlacement()){
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
				// do nothing
				break;
			}
		}
	}
}

//! @fn    void handleButtonEvent(XButtonEvent* ev, DockApp* da)
//! @brief Handles XButtonEvents made on the DockApp's frames.
void
Harbour::handleButtonEvent(XButtonEvent* ev, DockApp* da)
{
	if (!ev || !da)
		return;

	_last_button_x = ev->x;
	_last_button_y = ev->y;

#ifdef MENUS
	// TO-DO: Make configurable
	if (ev->type == ButtonPress) {
		if (ev->button == BUTTON3) {
			if (_harbour_menu->isMapped()) {
				_harbour_menu->unmapWindow();
			} else {
				_harbour_menu->setDockApp(da);
				_harbour_menu->mapUnderMouse();
			}
		} else if (_harbour_menu->isMapped()) {
			_harbour_menu->unmapWindow();
		}
	}
#endif // MENUS
}

//! @fn    void handleMotionNotifyEvent(XMotionEvent* ev, DockApp* da)
//! @brief Initiates moving of a DockApp based on info from a XMotionEvent.
void
Harbour::handleMotionNotifyEvent(XMotionEvent* ev, DockApp* da)
{
	if (!da)
		return;

	int x = 0, y = 0;

	switch(_cfg->getHarbourPlacement()) {
	case TOP:
	case BOTTOM:
		x = ev->x_root - _last_button_x;
		y = da->getY();
		if (x < 0)
			x = 0;
		else if ((x + da->getWidth()) > _scr->getWidth())
			x = _scr->getWidth() - da->getWidth();
		break;
	case LEFT:
	case RIGHT:
		x = da->getX();
		y = ev->y_root - _last_button_y;
		if (y < 0)
			y = 0;
		else if ((y + da->getHeight()) > _scr->getHeight())
			y = _scr->getHeight() - da->getHeight();
		break;
	default:
		// Do nothing
		break;
	}

	da->move(x, y);
}

//! @fn    void handleConfigureRequestEvent(XConfigureRequestEvent* ev, DockApp* da)
//! @brief Handles XConfigureRequestEvents.
void
Harbour::handleConfigureRequestEvent(XConfigureRequestEvent* ev, DockApp* da)
{
	if (!ev || !da)
		return;

	list<DockApp*>::iterator it =
		find(_dockapp_list.begin(), _dockapp_list.end(), da);

	if (it != _dockapp_list.end()) {
		// Thing is that we doesn't listen to border width, position or
		// stackign so the only thing that we'll alter is size if that's
		// what we want to configure
		//
		// TO-DO: Add Boundary checking for screen and screen edge.
		unsigned int width =
			(ev->value_mask&CWWidth) ? ev->width : da->getWidth();
		unsigned int height =
			(ev->value_mask&CWHeight) ? ev->height : da->getHeight();

		da->resize(width, height);
	}
}

//! @fn    void placeDockApp(DockApp *da)
//! @brief Tries to find a empty spot for the DockApp
void
Harbour::placeDockApp(DockApp *da)
{
	if (!da || !_dockapp_list.size())
		return;

	bool l_o = (_cfg->getHarbourOrientation() == BOTTOM_TO_TOP);

	int test, x = 0, y = 0;
	bool placed = false, increase = false, x_place = false;

	switch (_cfg->getHarbourPlacement()) {
	case TOP:
		x_place = true;
		break;
	case BOTTOM:
		x_place = true;
		y = _scr->getHeight() - da->getHeight();
		break;
	case RIGHT:
		x = _scr->getWidth() - da->getWidth();
		break;
	}

	list<DockApp*>::iterator it;

	if (x_place) {
		x = test = l_o ? _scr->getWidth() - da->getWidth() : 0;

		while (!placed &&
					 (l_o
						? (test >= 0)
						: ((test + da->getWidth()) < _scr->getWidth()))) {
			placed = increase = true;

			it = _dockapp_list.begin();
			for (; placed && (it != _dockapp_list.end()); ++it) {
				if ((*it) == da)
					continue; // exclude ourselves

				if (((*it)->getX() < signed(test + da->getWidth())) &&
						((*it)->getRX() > test)) {
					placed = increase = false;
					test = l_o ? (*it)->getX() - da->getWidth() : (*it)->getRX();
				}
			}

			if (placed)
				x = test;
			else if (increase)
				test += l_o ? -1 : 1;
		}
	} else {
		y = test = l_o ? _scr->getHeight() - da->getHeight() : 0;

		while (!placed &&
					 (l_o
						? (test >= 0)
						: ((test + da->getHeight()) < _scr->getHeight()))) {
			placed = increase = true;

			it = _dockapp_list.begin();
			for (; placed && (it != _dockapp_list.end()); ++it) {
				if ((*it) == da)
					continue; // exclude ourselves

				if (((*it)->getY() < signed(test + da->getHeight())) &&
						((*it)->getRY() > test)) {
					placed = increase = false;
					test = l_o ? (*it)->getY() - da->getHeight() : (*it)->getRY();
				}
			}

			if (placed)
				y = test;
			else if (increase) {
				test += l_o ? -1 : 1;
			}
		}
	}

	da->move(x, y);
}

#endif // HARBOUR
