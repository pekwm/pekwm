//
// dockapp.cc for pekwm
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

#include "dockapp.hh"
#include <X11/Xutil.h>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

const unsigned int DOCKAPP_DEFAULT_SIDE = 64;
const unsigned int DOCKAPP_BORDER_WIDTH = 2;

DockApp::DockApp(ScreenInfo *s, Theme *t, Window win) :
scr(s), theme(t),
m_window(win), m_frame_window(None),
m_client_window(win), m_icon_window(None),
m_background(None),
m_x(0), m_y(0),
m_width(1), m_height(1),
m_is_alive(true), m_is_hidden(true)
{
	// First, we need to figure out which window that actually belongs to the
	// dockapp. This we do by checking if it has the IconWindowHint set in it's
	// WM Hint.
	XWMHints *wm_hints = XGetWMHints(scr->getDisplay(), m_window);
	if (wm_hints) {
		if ((wm_hints->flags&IconWindowHint) &&
				(wm_hints->icon_window != None)) {
			// let us hide the m_client_window window, as we won't use it.
			XUnmapWindow(scr->getDisplay(), m_client_window);

			m_icon_window = wm_hints->icon_window;
			m_window = wm_hints->icon_window;
		}
		XFree(wm_hints);
	}

	// Now, when we now what window id we should use, set the size up.
	XWindowAttributes attr;
	if (XGetWindowAttributes(scr->getDisplay(), m_window, &attr)) {
		m_width = attr.width;
		m_height = attr.height;
	} else {
		m_width = DOCKAPP_DEFAULT_SIDE;
		m_height =  DOCKAPP_DEFAULT_SIDE;
	}

	// Add the border to the size
	m_width += DOCKAPP_BORDER_WIDTH * 2;
	m_height += DOCKAPP_BORDER_WIDTH * 2;

	// Okie, now lets create it's parent window which is going to hold the border
	XSetWindowAttributes sattr;
	sattr.override_redirect = true;
	sattr.event_mask = SubstructureRedirectMask|ButtonPressMask|ButtonMotionMask;

	m_frame_window =
		XCreateWindow(scr->getDisplay(), scr->getRoot(),
									m_x, m_y, m_width, m_height, 0,
									CopyFromParent, InputOutput, CopyFromParent,
									CWOverrideRedirect|CWEventMask, &sattr);

	// Initial makeup
	repaint();
	XSetWindowBorderWidth(scr->getDisplay(), m_window, 0);

	// Move the dockapp to it's new parent, we filter this so that we won't
	// get any UnmapEvents
	XSelectInput(scr->getDisplay(), m_window, NoEventMask);
	XReparentWindow(scr->getDisplay(), m_window, m_frame_window,
									DOCKAPP_BORDER_WIDTH, DOCKAPP_BORDER_WIDTH);
	XSelectInput(scr->getDisplay(), m_window,
							 StructureNotifyMask|SubstructureNotifyMask);
}

DockApp::~DockApp()
{
	// if the client still is alive, we should reparent it to the root
	// window, else we don't have to care about that.
	if (m_is_alive) {
		if (m_icon_window != None)
			XUnmapWindow(scr->getDisplay(), m_icon_window);

		XReparentWindow(scr->getDisplay(), m_window, scr->getRoot(), m_x, m_y);
		XMapWindow(scr->getDisplay(), m_client_window);
	}

	// Clean up
	XDestroyWindow(scr->getDisplay(), m_frame_window);
	if (m_background != None)
		XFreePixmap(scr->getDisplay(), m_background);
}

//! @fn    void kill(void)
//! @brief Kills the DockApp
void
DockApp::kill(void)
{
	XKillClient(scr->getDisplay(), m_window);
}

//! @fn    void move(int x, int y)
//! @brief Moves the DockApp to x,y
void
DockApp::move(int x, int y)
{
	m_x = x;
	m_y = y;

	XMoveWindow(scr->getDisplay(), m_frame_window, m_x, m_y);
}

//! @fn    void resize(unsigned int width, unsigned int height)
//! @breif Resizes the DockApp, size excludes the border.
//! @todo Make sure it's inside the screen!
void
DockApp::resize(unsigned int width, unsigned int height)
{
	if ((m_width == width) && (m_height == height))
		return;

	m_width = width + (DOCKAPP_BORDER_WIDTH * 2);
	m_height = height + (DOCKAPP_BORDER_WIDTH * 2);

	XResizeWindow(scr->getDisplay(), m_frame_window, m_width, m_height);
	XResizeWindow(scr->getDisplay(), m_window, width, height);

	repaint();
}

//! @fn    void hide(void)
//! @brief Hides the DockApp
void
DockApp::hide(void)
{
	if (m_is_hidden == true)
		return;
	m_is_hidden = true;

	XSelectInput(scr->getDisplay(), m_window, NoEventMask);
	XUnmapWindow(scr->getDisplay(), m_window);
	XSelectInput(scr->getDisplay(), m_window,
							 StructureNotifyMask|SubstructureNotifyMask);

	XUnmapWindow(scr->getDisplay(), m_frame_window);
}

//! @fn    void unhide(void)
//! @brief Unhides the DockApp
void
DockApp::unhide(void)
{
	if (m_is_hidden == false)
		return;
	m_is_hidden = false;

	XMapWindow(scr->getDisplay(), m_frame_window);

	XSelectInput(scr->getDisplay(), m_window, NoEventMask);
	XMapWindow(scr->getDisplay(), m_window);
	XSelectInput(scr->getDisplay(), m_window,
							 StructureNotifyMask|SubstructureNotifyMask);
}

//! @fn    void loadTheme(void)
//! @brief Loads the current theme and repaints the DockApp.
void
DockApp::loadTheme(void)
{
	// Note, here we are going to check if the DockApp borderwidth have
	// changed etc in the future but for now we'll only have to repaint it
	repaint();
}

//! @fn    void repaint(void)
//! @brief Repaints the DockApp's background.
void
DockApp::repaint(void)
{
	if (m_background != None)
		XFreePixmap(scr->getDisplay(), m_background);

	m_background = XCreatePixmap(scr->getDisplay(), scr->getRoot(),
															 m_width, m_height, scr->getDepth());
	theme->getHarbourImage()->draw(m_background, 0, 0, m_width, m_height);

	XSetWindowBackgroundPixmap(scr->getDisplay(), m_frame_window, m_background);
	XClearWindow(scr->getDisplay(), m_frame_window);
}

#endif // HARBOUR
