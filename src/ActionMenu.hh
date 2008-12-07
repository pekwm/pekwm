//
// ActionMenu.hh for pekwm
// Copyright © 2002-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _ACTIONMENU_HH_
#define _ACTIONMENU_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef MENUS

#include "pekwm.hh"
#include "Action.hh" // For ActionOk
#include "CfgParser.hh"
#include "PMenu.hh"

#include <string>
#include <list>

class WORefMenu;
class PScreen;
class Theme;
class ActionHandler;

class ActionMenu : public WORefMenu
{
public:
    ActionMenu(MenuType type,
               const std::wstring &title, const std::string &name,
               const std::string &decor_name = "MENU");
    virtual ~ActionMenu(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    virtual void unmapWindow(void);
    // END - PWinObj interface.

    virtual void handleItemExec(PMenu::Item *item);

    virtual void insert(PMenu::Item *item);
    virtual void insert(const std::wstring &name, PWinObj *wo_ref = 0, PTexture *icon = 0);
    virtual void insert(const std::wstring &name, const ActionEvent &ae,
                        PWinObj *wo_ref = 0, PTexture *icon = 0);

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
    std::list<PMenu::Item*>::iterator _insert_at;

    bool _has_dynamic; /**< Set to true if any of the entries in the menu is dynamic. */
};

#endif // _ACTIONMENU_HH_

#endif // MENUS
