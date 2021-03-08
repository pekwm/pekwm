//
// FrameListMenu.hh for pekwm
// Copyright (C) 2003-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"

#include <string>
#include <list>

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
    virtual void unmapWindow(void);
    // END - PWinObj interface.

    virtual void handleItemExec(PMenu::Item *item);

private:
    void updateFrameListMenu(void);

private:
    void buildName(Frame *frame, std::wstring &name);
    void buildFrameNames(Frame *frame, std::wstring &pre_name,
                         bool insert_separator);

    void handleGotomenu(Client *client);
    void handleIconmenu(Client *client);
    void handleAttach(Client *client_to, Client *client_from, bool frame);
};
