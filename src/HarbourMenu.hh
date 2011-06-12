//
// HarbourMenu.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _HARBOURMENU_HH_
#define _HARBOURMENU_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "PMenu.hh"

class WORefMenu;
class Theme;
class Harbour;
class DockApp;

class HarbourMenu : public PMenu
{
public:
    HarbourMenu(Theme *theme, Harbour *harbour);
    virtual ~HarbourMenu(void);

    virtual void handleItemExec(PMenu::Item *item);

    inline void setDockApp(DockApp *da) { _dockapp = da; }

private:
    Harbour *_harbour;
    DockApp *_dockapp;
};

#endif // _HARBOURMENU_HH_
