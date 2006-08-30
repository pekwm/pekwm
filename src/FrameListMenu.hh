//
// FrameListMenu.hh for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef MENUS

#ifndef _FRAMELISTMENU_HH_
#define _FRAMELISTMENU_HH_

#include "pekwm.hh"

#include <string>
#include <list>

class WORefMenu;
class PScreen;
class Theme;
class Frame;
class Client;

class PMenu::Item;

class FrameListMenu : public WORefMenu
{
public:
    FrameListMenu(PScreen *scr, Theme *theme,
                  const std::list<Frame*> &frame_list, MenuType type,
                  const std::string &title, const std::string &name,
                  const std::string &decor_name = "MENU");
    virtual ~FrameListMenu(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    // END - PWinObj interface.

    virtual void handleItemExec(PMenu::Item *item);

private:
    void updateFrameListMenu(void);

private:
    void buildName(Frame *frame, std::string &name);
    void buildFrameNames(Frame *frame, std::string &pre_name);

    void handleGotomenu(Client *client);
    void handleIconmenu(Client *client);
    void handleAttach(Client *client_to, Client *client_from, bool frame);

private:
    const std::list<Frame*> &_frame_list;
};

#endif //  _FRAMELISTMENU_HH_

#endif // MENUS
