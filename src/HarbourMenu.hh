//
// HarbourMenu.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HARBOUR
#ifdef MENUS

#ifndef _HARBOURMENU_HH_
#define _HARBOURMENU_HH_

#include "pekwm.hh"

class BaseMenu;
class ScreenInfo;
class Theme;
class Workspaces;
class Harbour;
class DockApp;

class HarbourMenu : public BaseMenu
{
public:
	HarbourMenu(ScreenInfo *s, Theme *t, Harbour *h, Workspaces *w);
	virtual ~HarbourMenu();

	virtual void handleButton1Release(BaseMenuItem *curr);

	inline void setDockApp(DockApp *da) { _dockapp = da; }

private:
	ScreenInfo *_scr;
	Theme *_theme;
	Harbour *_harbour;

	DockApp *_dockapp;
};

#endif // _HARBOURMENU_HH_

#endif // MENUS
#endif // HARBOUR
