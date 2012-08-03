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
#include "WORefMenu.hh"
#include "CfgParser.hh"

#include <string>

class Theme;

class DecorMenu : public WORefMenu
{
public:
    DecorMenu(Theme *theme, const std::string &name);
    virtual ~DecorMenu(void);

    virtual void handleItemExec(PMenu::Item *item);
    virtual void reload(CfgParser::Entry *section);
};

#endif //  _DECORMENU_HH_
