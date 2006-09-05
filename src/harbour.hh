//
// harbour.hh for pekwm
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

#ifndef _HARBOUR_HH_
#define _HARBOUR_HH_

#include "screeninfo.hh"
#include "config.hh"
#include "theme.hh"
#include "dockapp.hh"
#include "harbourmenu.hh"

#include <list>

class Harbour
{
public:
	Harbour(ScreenInfo *s, Config *c, Theme *t);
	~Harbour();

	void addDockApp(DockApp *da);
	void removeDockApp(DockApp *da);
	void removeAllDockApps(void);

	DockApp *findDockApp(Window win);
	DockApp *findDockAppFromFrame(Window win);

#ifdef MENUS
	GenericMenu *getHarbourMenu(void) { return m_harbour_menu; }
#endif // MENUS

	void restack(void);
	void rearrange(void);
	void loadTheme(void);

	void stackWindowOverOrUnder(Window win, bool raise);

	void handleButtonEvent(XButtonEvent *ev, DockApp *da);
	void handleMotionNotifyEvent(XMotionEvent *ev, DockApp *da);
	void handleConfigureRequestEvent(XConfigureRequestEvent *ev, DockApp *da);

private:
	void placeDockApp(DockApp *da);

private:
	ScreenInfo *scr;
	Config *cfg;
	Theme *theme;

	std::list<DockApp*> m_dockapp_list;
#ifdef MENUS
	HarbourMenu *m_harbour_menu;
#endif // MENUS

	int m_last_button_x;
	int m_last_button_y;
};

#endif // _HARBOUR_HH_

#endif // HARBOUR
