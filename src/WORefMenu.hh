//
// WORefMenu.hh for pekwm
// Copyright (C) 2009 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef MENUS

#ifndef _WOREFMENU_HH_
#define _WOREFMENU_HH_

#include <string>

class PWinObj;
class PMenu;
class PScreen;
class Theme;

class WORefMenu : public PMenu
{
public:
    WORefMenu(PScreen *scr, Theme *theme,
              const std::wstring &title, const std::string &name,
              const std::string &decor_name = "MENU");
    virtual ~WORefMenu(void);

    inline PWinObj *getWORef(void) const { return _wo_ref; }
    void setWORef(PWinObj *wo);

protected:
    PWinObj *_wo_ref;

    std::wstring _title_base;
    std::wstring _title_pre, _title_post;
};

#endif // _WOREFMENU_HH_

#endif // MENUS
