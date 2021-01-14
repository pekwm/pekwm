//
// FrameListMenu.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _FRAMELISTMENU_HH_
#define _FRAMELISTMENU_HH_

#include "config.h"

#include "pekwm.hh"
#include "PMenu.hh"

#include <string>
#include <list>

class WORefMenu;
class PScreen;
class Theme;
class Frame;
class Client;

class FrameListMenu : public WORefMenu
{
public:
    FrameListMenu(MenuType type,
                  const std::wstring &title, const std::string &name,
                  const std::string &decor_name = "MENU");
    virtual ~FrameListMenu(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    // END - PWinObj interface.

    virtual void handleItemExec(PMenu::Item *item);

private:
    void updateFrameListMenu(void);

private:
    void buildName(Frame *frame, std::wstring &name);
    void buildFrameNames(Frame *frame, std::wstring &pre_name);

    void handleGotomenu(Client *client);
    void handleIconmenu(Client *client);
    void handleAttach(Client *client_to, Client *client_from, bool frame);
};

#endif //  _FRAMELISTMENU_HH_

