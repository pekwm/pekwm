//
// HarbourMenu.cc for pekwm
// Copyright © 2003-2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "PScreen.hh"
//#include "Action.hh"
#include "HarbourMenu.hh"
#include "DockApp.hh"

//! @brief HarbourMenu constructor
HarbourMenu::HarbourMenu(Theme *theme, Harbour *harbour)
    : PMenu(theme, L"Harbour", "HARBOUR"),
      _harbour(harbour), _dockapp(0)
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
    if (! item || ! _dockapp) {
        return;
    }

    if (item->getAE().isOnlyAction(ACTION_SET)) {
        _dockapp->iconify();

    } else if (item->getAE().isOnlyAction(ACTION_CLOSE)) {
        _dockapp->kill();
        _dockapp = 0;
    }
}

