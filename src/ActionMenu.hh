//
// ActionMenu.hh for pekwm
// Copyright (C) 2002-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "Action.hh" // For ActionOk
#include "CfgParser.hh"
#include "PMenu.hh"

#include <string>

class WORefMenu;
class PScreen;
class Theme;
class ActionHandler;

class ActionMenu : public WORefMenu
{
public:
    ActionMenu(MenuType type, ActionHandler *act,
               const std::wstring &title, const std::string &name,
               const std::string &decor_name = "MENU");
    virtual ~ActionMenu(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    virtual void unmapWindow(void);
    // END - PWinObj interface.

    virtual void handleItemExec(PMenu::Item *item);

    virtual void insert(PMenu::Item *item);
    using PMenu::insert;

    virtual void reload(CfgParser::Entry *section);

    virtual void remove(PMenu::Item *item);
    virtual void removeAll(void);

private:
    void parse(CfgParser::Entry *section, PMenu::Item *parent=0);

    PTexture *getIcon(CfgParser::Entry *value);
    void rebuildDynamic(void);
    void removeDynamic(void);

private:
    ActionHandler *_act;

    ActionOk _action_ok;
    std::vector<PMenu::Item*>::size_type _insert_at;

    /** Set to true if any of the entries in the menu is dynamic. */
    bool _has_dynamic;
};
