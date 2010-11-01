//
// DecorMenu.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _DECORMENU_HH_
#define _DECORMENU_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "PMenu.hh"

#include <string>

class WORefMenu;
class PScreen;
class Theme;
class ActionHandler;

class DecorMenu : public WORefMenu
{
public:
    DecorMenu(PScreen *scr, Theme *theme, ActionHandler *act,
              const std::string &name);
    virtual ~DecorMenu(void);

    virtual void handleItemExec(PMenu::Item *item);
    virtual void reload(CfgParser::Entry *section);

private:
    ActionHandler *_act;
};

#endif //  _DECORMENU_HH_
