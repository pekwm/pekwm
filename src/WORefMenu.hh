//
// WORefMenu.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _WOREFMENU_HH_
#define _WOREFMENU_HH_

#include "config.h"

#include <string>

#include "PWinObjReference.hh"

class PWinObj;
class PMenu;

class WORefMenu : public PMenu, public PWinObjReference
{
public:
    WORefMenu(const std::wstring &title, const std::string &name,
              const std::string &decor_name = "MENU");
    virtual ~WORefMenu(void);

    virtual void notify(Observable *observable, Observation *observation);

    virtual void setWORef(PWinObj *wo_ref);

protected:
    std::wstring _title_base;
    std::wstring _title_pre;
    std::wstring _title_post;
};

#endif // _WOREFMENU_HH_
