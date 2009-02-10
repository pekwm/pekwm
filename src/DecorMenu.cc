//
// FrameListMenu.cc for pekwm
// Copyright (C) 2002-2009 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef MENUS

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"
#include "DecorMenu.hh"

#include "Workspaces.hh"
#include "ActionHandler.hh"
#include "Theme.hh"

#include <map>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::map;

//! @brief Constructor for DecorMenu.
DecorMenu::DecorMenu(PScreen *scr, Theme *theme, ActionHandler *act,
                     const std::string &name) :
        WORefMenu(scr, theme, L"Decor Menu", name),
        _act(act)
{
    _menu_type = DECORMENU_TYPE;
}

//! @brief Destructor for DecorMenu
DecorMenu::~DecorMenu(void)
{
}

//! @brief Handles button1 release
void
DecorMenu::handleItemExec(PMenu::Item *item)
{
    if (! item) {
        return;
    }

    ActionPerformed ap(_wo_ref, item->getAE());
    _act->handleAction(ap);
}

//! @brief Rebuilds the menu.
void
DecorMenu::reload(CfgParser::Entry *section)
{
    // clear the menu before loading
    removeAll();

    // setup dummy action
    Action action;
    ActionEvent ae;

    action.setAction(ACTION_SET);
    action.setParamI(0, ACTION_STATE_DECOR);
    ae.action_list.push_back(action);

    map<string, Theme::PDecorData*>::const_iterator it(_theme->decor_begin());
    for (; it != _theme->decor_end(); ++it) {
        ae.action_list.back().setParamS(it->first);
        insert(Util::to_wide_str(it->first), ae, 0);
    }

    buildMenu(); // rebuild the menu
}

#endif // MENUS
