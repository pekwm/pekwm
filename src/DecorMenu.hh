//
// DecorMenu.hh for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon at pekdon dot>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef MENUS

#ifndef _DECORMENU_HH_
#define _DECORMENU_HH_

#include "pekwm.hh"

#include <string>

class WORefMenu;
class PScreen;
class Theme;
class ActionHandler;

class PMenu::Item;

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

#endif // MENUS
