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
#include "WORefMenu.hh"

#include <string>

class ActionHandler;

class ActionMenu : public WORefMenu
{
public:
    ActionMenu(MenuType type, ActionHandler *act,
               const std::wstring &title, const std::string &name,
               const std::string &decor_name = "MENU");
    virtual ~ActionMenu(void);

    // START - PWinObj interface.
    virtual void mapWindow(void) override;
    virtual void unmapWindow(void) override;
    // END - PWinObj interface.

    virtual void handleItemExec(PMenu::Item *item) override;

    virtual void insert(PMenu::Item *item) override;
    using PMenu::insert;

    virtual void reload(CfgParser::Entry *section) override;

    virtual void remove(PMenu::Item *item) override;
    virtual void removeAll(void) override;

protected:
    void rebuildDynamic(void);
    void removeDynamic(void);

    virtual CfgParser::Entry* runDynamic(CfgParser& parser,
                                         const std::string& src);

private:
    void parse(CfgParser::Entry *section, PMenu::Item *parent=0);
    PTexture *getIcon(CfgParser::Entry *value);

private:
    ActionHandler *_act;

    ActionOk _action_ok;
    std::vector<PMenu::Item*>::size_type _insert_at;

    /** Set to true if any of the entries in the menu is dynamic. */
    bool _has_dynamic;
};
