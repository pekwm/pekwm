//
// harbourmenu.hh for pekwm
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
#ifdef MENUS

#ifndef _HARBOURMENU_HH_
#define _HARBOURMENU_HH_

#include "screeninfo.hh"
#include "theme.hh"
#include "genericmenu.hh"
#include "dockapp.hh"

class Harbour;

class HarbourMenu : public GenericMenu
{
public:
	HarbourMenu(ScreenInfo *s, Theme *t, Harbour *h);
	virtual ~HarbourMenu();

	virtual void handleButton1Release(BaseMenuItem *curr);

	inline void setDockApp(DockApp *da) { m_dockapp = da; }

private:
	ScreenInfo *scr;
	Theme *theme;
	Harbour *harbour;

	DockApp *m_dockapp;
};

#endif // _HARBOURMENU_HH_

#endif // MENUS
#endif // HARBOUR
