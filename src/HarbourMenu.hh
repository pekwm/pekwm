//
// HarbourMenu.hh for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
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

class WORefMenu;
class PScreen;
class Theme;
class Harbour;
class DockApp;

class PMenu::Item;

class HarbourMenu : public PMenu
{
public:
	HarbourMenu(PScreen *scr, Theme *theme, Harbour *harbour);
	virtual ~HarbourMenu(void);

	virtual void handleItemExec(PMenu::Item *item);

	inline void setDockApp(DockApp *da) { _dockapp = da; }

private:
	Harbour *_harbour;
	DockApp *_dockapp;
};

#endif // _HARBOURMENU_HH_

#endif // MENUS
#endif // HARBOUR
