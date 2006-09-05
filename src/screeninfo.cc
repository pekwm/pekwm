//
// screeninfo.cc for pekwm
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

#include "screeninfo.hh"

#include <iostream>

#include <X11/keysym.h> // For XK_ entries

using std::cerr;
using std::endl;

ScreenInfo::ScreenInfo(Display *d) :
dpy(d),
m_num_lock(0), m_scroll_lock(0)
{
	XGrabServer(dpy);

	m_screen = DefaultScreen(dpy);
	m_root = RootWindow(dpy, m_screen);

	m_depth = DefaultDepth(dpy, m_screen);
	m_visual = DefaultVisual(dpy, m_screen);
	m_colormap = DefaultColormap(dpy, m_screen);

	m_width = WidthOfScreen(ScreenOfDisplay(dpy, m_screen));
	m_height = HeightOfScreen(ScreenOfDisplay(dpy, m_screen));

#ifdef XINERAMA
	// check if we have Xinerama extension enabled
	if (XineramaIsActive(dpy)) {
		m_has_xinerama = true;
		m_xinerama_last_head = 0;
		m_xinerama_infos = XineramaQueryScreens(dpy, &m_xinerama_num_heads);
	} else {
		m_has_xinerama = false;
		m_xinerama_infos = 0; // make sure we don't point anywhere we shouldn't
	}
#endif // XINERAMA

	// Figure out what keys the Num and Scroll Locks are

	// This code is strongly based on code from WindowMaker
	int mask_table[8] = {
		ShiftMask,LockMask,ControlMask,Mod1Mask,
		Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};

	KeyCode num_lock = XKeysymToKeycode(dpy, XK_Num_Lock);
	KeyCode scroll_lock = XKeysymToKeycode(dpy, XK_Scroll_Lock);

	XModifierKeymap *modmap = XGetModifierMapping(dpy);
	if (modmap && (modmap->max_keypermod > 0)) {

		for (int i = 0; i < (8*modmap->max_keypermod); ++i) {
	    if (num_lock && (modmap->modifiermap[i] == num_lock)) {
				m_num_lock = mask_table[i/modmap->max_keypermod];
			} else if (scroll_lock && (modmap->modifiermap[i] == scroll_lock)) {
				m_scroll_lock = mask_table[i/modmap->max_keypermod];
			}
		}
	}

	if (modmap)
		XFreeModifiermap(modmap);
	// Stop WindowMaker like code

	XSync(dpy, false);
	XUngrabServer(dpy);
}

ScreenInfo::~ScreenInfo(void) {
#ifdef XINERAMA
	if (m_has_xinerama) { // only free if we first had it
		XFree(m_xinerama_infos);
		m_xinerama_infos = 0;
	}
#endif // XINERAMA
}

#ifdef XINERAMA

//! @fn    unsigned int getHead(int x, int y)
//! @brief Searches for the head at the coordinates x,y.
//! @return If it fails or Xinerama isn't on, return 0.
unsigned int
ScreenInfo::getHead(int x, int y) {
	// is Xinerama extensions enabled?
	if (m_has_xinerama) {
		// check if last head is still active
		if ((m_xinerama_infos[m_xinerama_last_head].x_org <= x) &&
				((m_xinerama_infos[m_xinerama_last_head].x_org +
				m_xinerama_infos[m_xinerama_last_head].width) > x) &&
				(m_xinerama_infos[m_xinerama_last_head].y_org <= y) &&
				((m_xinerama_infos[m_xinerama_last_head].y_org +
				m_xinerama_infos[m_xinerama_last_head].height) > y)) {
			return m_xinerama_last_head;

		} else {
			// go trough all the heads, and search
			for (int i = 0; i < (signed) m_xinerama_num_heads; i++) {
				if ((m_xinerama_infos[i].x_org <= x) &&
						((m_xinerama_infos[i].x_org + m_xinerama_infos[i].width) > x) &&
						(m_xinerama_infos[i].y_org <= y) &&
						((m_xinerama_infos[i].y_org + m_xinerama_infos[i].height) > y)) {
					return m_xinerama_last_head = i;
				}
			}
		}
	}

	return 0;
}

//! @fn    unsigned int getCurrHead(void)
//! @brief Searches for the head that the pointer currently is on.
//! @return Active head number
unsigned int
ScreenInfo::getCurrHead(void)
{

	// is Xinerama extensions enabled?
	if (m_has_xinerama) {
		Window d_root, d_win;
		int x = 0, y = 0, win_x, win_y;
		unsigned int mask;

		XQueryPointer(dpy, m_root, &d_root, &d_win, &x, &y, &win_x, &win_y, &mask);

		return getHead(x, y);
	}

	return 0;
}

//! @fn    bool getHeadInfo(unsigned int head, HeadInfo &head_info)
//! @brief Fills head_info with info about head nr head
//! @param head Head number to examine
//! @param head_info Returning info about the head
//! @return true if xinerama is off or head exists.
bool
ScreenInfo::getHeadInfo(unsigned int head, HeadInfo &head_info)
{
	if (m_has_xinerama) {
		if ((signed) head  < m_xinerama_num_heads) {
			head_info.x = m_xinerama_infos[head].x_org;
			head_info.y = m_xinerama_infos[head].y_org;
			head_info.width = m_xinerama_infos[head].width;
			head_info.height = m_xinerama_infos[head].height;
		} else {
#ifdef DEBUG
			cerr << __FILE__ << "@" << __LINE__ << ": "
					 << "Head: " << head << " doesn't exist!" << endl;
#endif // DEBUG
			return false;
		}
	} else { // fill it up with "ordinary info"
		head_info.x = 0;
		head_info.y = 0;
		head_info.width = m_width;
		head_info.height = m_height;
	}

	return true;
}

#endif // XINERAMA
