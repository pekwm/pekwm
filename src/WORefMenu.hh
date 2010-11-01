//
// WORefMenu.hh for pekwm
// Copyright © 2004-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef _WOREFMENU_HH_
#define _WOREFMENU_HH_

#include <string>

#include "PWinObjReference.hh"

class PWinObj;
class PMenu;
class PScreen;
class Theme;

class WORefMenu : public PMenu, public PWinObjReference
{
public:
    WORefMenu(PScreen *scr, Theme *theme,
              const std::wstring &title, const std::string &name,
              const std::string &decor_name = "MENU");
    virtual ~WORefMenu(void);

    virtual void notify(Observable *observable, Observation *observation);

    virtual void setWORef(PWinObj *wo_ref);

protected:
    std::wstring _title_base;
    std::wstring _title_pre, _title_post;
};

#endif // _WOREFMENU_HH_
