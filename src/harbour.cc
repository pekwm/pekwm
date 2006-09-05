//
// harbour.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifdef HARBOUR

#include "harbour.hh"

#include <algorithm>
#include <functional>

using std::list;
using std::mem_fun;
#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

Harbour::Harbour(ScreenInfo *s, Config *c, Theme *t) :
scr(s), cfg(c), theme(t),
#ifdef MENUS
m_harbour_menu(NULL),
#endif // MENUS
m_last_button_x(0), m_last_button_y(0)
{
#ifdef MENUS
	m_harbour_menu = new HarbourMenu(scr, theme, this);
#endif // MENUS
}

Harbour::~Harbour()
{
	removeAllDockApps();
#ifdef MENUS
	if (m_harbour_menu)
		delete m_harbour_menu;
#endif // MENUS
}

//! @fn    void addDockApp(DockApp *da)
//! @brief Adds a DockApp to the Harbour
void
Harbour::addDockApp(DockApp *da)
{
	if (!da)
		return;
	m_dockapp_list.push_back(da);

	placeDockApp(da); // place it in a empty space

	if (da->isHidden()) // make sure it's visible
		da->unhide();

	if (cfg->getHarbourOntop()) // fix stacking
		XRaiseWindow(scr->getDisplay(), da->getFrameWindow());
	else
		XLowerWindow(scr->getDisplay(), da->getFrameWindow());
}

//! @fn    void removeDockApp(DockApp *da)
//! @brief Removes a DockApp from the Harbour
void
Harbour::removeDockApp(DockApp *da)
{
	if (!da)
		return;

	list<DockApp*>::iterator it =
		find(m_dockapp_list.begin(), m_dockapp_list.end(), da);

	if (it != m_dockapp_list.end()) {
		m_dockapp_list.remove(da);
		delete da;
	}
}

//! @fn    void removeAllDockApps(void)
//! @brief Removes all DockApps from the Harbour
void
Harbour::removeAllDockApps(void)
{
	list<DockApp*>::iterator it = m_dockapp_list.begin();
	for (; it != m_dockapp_list.end(); ++it)
		delete (*it);
	m_dockapp_list.clear();
}

//! @fn    DockApp* findDockApp(Window win);
//! @brief Tries to find a dockapp which uses the window win
DockApp*
Harbour::findDockApp(Window win)
{
	DockApp *dockapp = NULL;

	list<DockApp*>::iterator it = m_dockapp_list.begin();
	for (; it != m_dockapp_list.end(); ++it) {
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

	list<DockApp*>::iterator it = m_dockapp_list.begin();
	for (; it != m_dockapp_list.end(); ++it) {
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
	if (!m_dockapp_list.size())
		return;

	list<DockApp*>::iterator it = m_dockapp_list.begin();
	for (; it != m_dockapp_list.end(); ++it) {
		if (cfg->getHarbourOntop())
			XRaiseWindow(scr->getDisplay(), (*it)->getFrameWindow());
		else
			XLowerWindow(scr->getDisplay(), (*it)->getFrameWindow());
	}
}

//! @fn    void rearrange(void)
//! @brief Goes through the DockApp list and places the dockapp.
void
Harbour::rearrange(void)
{
	if (!m_dockapp_list.size())
		return;

	list<DockApp*>::iterator it = m_dockapp_list.begin();
	for (; it != m_dockapp_list.end(); ++it)
		placeDockApp(*it);
}

//! @fn    void loadTheme(void)
//! @brief Repaints all dockapps with the new theme
void
Harbour::loadTheme(void)
{
	if (!m_dockapp_list.size())
		return;

	for_each(m_dockapp_list.begin(), m_dockapp_list.end(),
					 mem_fun(&DockApp::loadTheme));
}

//! @fn    void stackWindowOverOrUnder(Window win, bool raise)
//! @brief This method is used when rasing or lowering frames
void
Harbour::stackWindowOverOrUnder(Window win, bool raise)
{
	if (m_dockapp_list.size() && (raise == cfg->getHarbourOntop())) {
		unsigned int size = m_dockapp_list.size(); // convinience
		Window windows[size + 1];

		list<DockApp*>::iterator it = m_dockapp_list.begin();

		if (raise) {
			for (unsigned int i = 0; it != m_dockapp_list.end(); ++i, ++it)
				windows[i] = (*it)->getFrameWindow();
			windows[size] = win;
		} else {
			for (unsigned int i = 1; it != m_dockapp_list.end(); ++i, ++it)
				windows[i] = (*it)->getFrameWindow();
			windows[0] = win;
		}

		XRestackWindows(scr->getDisplay(), windows, size + 1);
	} else {
		XRaiseWindow(scr->getDisplay(), win);
	}
}

//! @fn    void handleButtonEvent(XButtonEvent *ev, DockApp *da)
//! @brief Handles XButtonEvents made on the DockApp's frames.
void
Harbour::handleButtonEvent(XButtonEvent *ev, DockApp *da)
{
	if (!ev || !da)
		return;

	m_last_button_x = ev->x;
	m_last_button_y = ev->y;

#ifdef MENUS
	// TO-DO: Make configurable
	if (ev->type == ButtonPress) {
		if (ev->button == Button3) {
			if (m_harbour_menu->isVisible()) {
				m_harbour_menu->hide();
			} else {
				m_harbour_menu->setDockApp(da);
				m_harbour_menu->showUnderMouse();
			}
		} else if (m_harbour_menu->isVisible()) {
			m_harbour_menu->hide();
		}
	}
#endif // MENUS
}

//! @fn    void handleMotionNotifyEvent(XMotionEvent *ev, DockApp *da)
//! @brief Initiates moving of a DockApp based on info from a XMotionEvent.
void
Harbour::handleMotionNotifyEvent(XMotionEvent *ev, DockApp *da)
{
	if (!da)
		return;

	int x = 0, y = 0;

	switch(cfg->getHarbourPlacement()) {
	case TOP:
	case BOTTOM:
		x = ev->x_root - m_last_button_x;
		y = da->getY();
		if (x < 0)
			x = 0;
		else if ((x + da->getWidth()) > scr->getWidth())
			x = scr->getWidth() - da->getWidth();
		break;
	case LEFT:
	case RIGHT:
		x = da->getX();
		y = ev->y_root - m_last_button_y;
		if (y < 0)
			y = 0;
		else if ((y + da->getHeight()) > scr->getHeight())
			y = scr->getHeight() - da->getHeight();
		break;
	default:
		// Do nothing
		break;
	}

	da->move(x, y);
}

//! @fn    void handleConfigureRequestEvent(XConfigureRequestEvent *ev, DockApp *da)
//! @brief Handles XConfigureRequestEvents.
void
Harbour::handleConfigureRequestEvent(XConfigureRequestEvent *ev, DockApp *da)
{
	if (!ev || !da)
		return;

	list<DockApp*>::iterator it =
		find(m_dockapp_list.begin(), m_dockapp_list.end(), da);

	if (it != m_dockapp_list.end()) {
		// Thing is that we doesn't listen to border width, position or
		// stackign so the only thing that we'll alter is size if that's
		// what we want to configure
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
	if (!da || !m_dockapp_list.size())
		return;

	bool l_o = (cfg->getHarbourOrientation() == BOTTOM_TO_TOP);

	int test, x = 0, y = 0;
	bool placed = false, increase = false, x_place = false;

	switch (cfg->getHarbourPlacement()) {
	case TOP:
		x_place = true;
		break;
	case BOTTOM:
		x_place = true;
		y = scr->getHeight() - da->getHeight();
		break;
	case RIGHT:
		x = scr->getWidth() - da->getWidth();
		break;
	}

	list<DockApp*>::iterator it;

	if (x_place) {
		x = test = l_o ? scr->getWidth() - da->getWidth() : 0;

		while (!placed &&
					 (l_o
						? (test >= 0)
						: ((test + da->getWidth()) < scr->getWidth()))) {
			placed = increase = true;

			it = m_dockapp_list.begin();
			for (; placed && (it != m_dockapp_list.end()); ++it) {
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
		y = test = l_o ? scr->getHeight() - da->getHeight() : 0;

		while (!placed &&
					 (l_o
						? (test >= 0)
						: ((test + da->getHeight()) < scr->getHeight()))) {
			placed = increase = true;

			it = m_dockapp_list.begin();
			for (; placed && (it != m_dockapp_list.end()); ++it) {
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
