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
class WindowManager;

class ActionMenu : public WORefMenu
{
public:
    ActionMenu(WindowManager *wm, MenuType type,
               const std::wstring &title, const std::string &name,
               const std::string &decor_name = "MENU");
    virtual ~ActionMenu(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    virtual void unmapWindow(void);
    // END - PWinObj interface.

    virtual void handleItemExec(PMenu::Item *item);

    virtual void insert(PMenu::Item *item);
    virtual void insert(const std::wstring &or_name, PWinObj *op_wo_ref = NULL);
    virtual void insert(const std::wstring &or_name, const ActionEvent &or_ae,
                        PWinObj *op_wo_ref = NULL);

    virtual void reload(CfgParser::Entry *section);

    virtual void remove(PMenu::Item *item);
    virtual void removeAll(void);

private:
    void parse(CfgParser::Entry *op_section, bool dynamic = false);

    void rebuildDynamic(void);
    void removeDynamic(void);

private:
    WindowManager *_wm;
    ActionHandler *_act;

    ActionOk _action_ok;
    std::list<PMenu::Item*>::iterator _insert_at;

    bool _has_dynamic;
};

#endif // _ACTIONMENU_HH_

#endif // MENUS
