//
// FrameListMenu.hh for pekwm
// Copyright (C) 2003-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_FRAMELISTMENU_HH_
#define _PEKWM_FRAMELISTMENU_HH_

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
		      const std::string &title, const std::string &name,
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
	void buildName(Frame *frame, std::string &name);
	void buildFrameNames(Frame *frame, const std::string &pre_name,
			     bool insert_separator);

	void handleGotomenu(Client *client);
	void handleIconmenu(Client *client);
	void handleAttach(Client *client_to, Client *client_from, bool frame);
};

#endif // _PEKWM_FRAMELISTMENU_HH_
