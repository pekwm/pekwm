//
// HarbourMenu.cc for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef HARBOUR
#ifdef MENUS

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "PScreen.hh"
//#include "Action.hh"
#include "HarbourMenu.hh"
#include "DockApp.hh"

//! @brief HarbourMenu constructor
HarbourMenu::HarbourMenu(PScreen *scr, Theme *theme, Harbour *harbour) :
PMenu(scr->getDpy(), theme, "Harbour", "HARBOUR"),
_harbour(harbour),
_dockapp(NULL)
{
	ActionEvent ae;
	Action action;

	action.setAction(ACTION_SET);
	action.setParamI(0, ACTION_STATE_ICONIFIED);
	ae.action_list.clear();
	ae.action_list.push_back(action);

	action.setAction(ACTION_CLOSE);
	ae.action_list.clear();
	ae.action_list.push_back(action);

	buildMenu();
}

//! @brief HarbourMenu destructor
HarbourMenu::~HarbourMenu(void)
{
}

//! @brief
void
HarbourMenu::handleItemExec(PMenu::Item *item)
{
	if ((item == NULL) || (_dockapp == NULL)) {
		return;
	}

	if (item->getAE().isOnlyAction(ACTION_SET)) {
		_dockapp->iconify();

	} else if (item->getAE().isOnlyAction(ACTION_CLOSE)) {
		_dockapp->kill();
		_dockapp = NULL;
	}
}

#endif // MENUS
#endif // HARBOUR
