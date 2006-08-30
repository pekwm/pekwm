//
// HarbourMenu.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef HARBOUR
#ifdef MENUS

#include "WindowObject.hh"
#include "Action.hh"
#include "BaseMenu.hh"
#include "HarbourMenu.hh"
#include "DockApp.hh"

HarbourMenu::HarbourMenu(ScreenInfo *s, Theme *t, Harbour *h, Workspaces *w) :
BaseMenu(s, t, w, "Harbour"),
_scr(s), _theme(t),
_harbour(h),
_dockapp(NULL)
{
	ActionEvent ae;
	Action action;

	action.action = ICONIFY;
	ae.action_list.clear();
	ae.action_list.push_back(action);

	action.action = CLOSE;
	ae.action_list.clear();
	ae.action_list.push_back(action);

	updateMenu();
}

HarbourMenu::~HarbourMenu()
{
}

void
HarbourMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (!curr || !_dockapp)
		return;


	if (curr->ae.isOnlyAction(ICONIFY)) {
		//		_dockapp->iconify();
	} else if (curr->ae.isOnlyAction(CLOSE)) {
		_dockapp->kill();
		_dockapp = NULL;
	}
}

#endif // MENUS
#endif // HARBOUR
