//
// dockapp.hh for pekwm
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

#ifndef _DOCKAPP_HH_
#define _DOCKAPP_HH_

#include "screeninfo.hh"
#include "theme.hh"

class DockApp
{
public:
	DockApp(ScreenInfo *s, Theme *t, Window win);
	~DockApp();

	inline int getX(void) const { return m_x; }
	inline int getRX(void) const { return m_x + m_width; }
	inline int getY(void) const { return m_y; }
	inline int getRY(void) const { return m_y + m_height; }
	inline unsigned int getWidth(void) const { return m_width; }
	inline unsigned int getHeight(void) const { return m_height; }

	inline Window getFrameWindow(void) const { return m_frame_window; }

	inline bool isHidden(void) const { return m_is_hidden; }
	inline void setAlive(bool a) { m_is_alive = a; }

	inline bool findDockApp(Window win) {
		if ((win != None) &&
				(win == m_client_window) || (win == m_icon_window))
			return true;
		return false;
	}
	inline bool findDockAppFromFrame(Window win) {
		if ((win != None) && (win == m_frame_window))
			return true;
		return false;
	}

	void kill(void);
	void move(int x, int y);
	void resize(unsigned int width, unsigned int height);

	void hide(void);
	void unhide(void);

	void loadTheme(void);

private:
	void repaint(void);

private:
	ScreenInfo *scr;
	Theme *theme;

	Window m_window, m_frame_window;
	Window m_client_window, m_icon_window;

	Pixmap m_background;

	int m_x, m_y;
	unsigned int m_width, m_height;

	bool m_is_alive;
	bool m_is_hidden;
};

#endif // _DOCKAPP_HH_

#endif // HARBOUR
