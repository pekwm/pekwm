//
// FrameListMenu.hh for pekwm
// Copyright (C) 2003 Claes Nasten <pekdon@gmx.net>
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

class BaseMenu;
class BaseMenuItem;
class WindowManager;
class Frame;
class Client;

class FrameListMenu : public BaseMenu
{
public:
	FrameListMenu(WindowManager *w, MenuType type);
	virtual ~FrameListMenu();

	// START - WindowObject interface.
	virtual void mapWindow(void);
	// END - WindowObject interface.

	inline Client* getClient(void) { return _client; }
	inline void setClient(Client* client) { _client = client; }

	virtual void handleButton1Release(BaseMenuItem* curr);

	void updateFrameListMenu(void);

private:
	void buildName(Frame* frame, std::string& name);
	void buildFrameNames(Frame* frame, std::string& pre_name);

	void handleGotomenu(Client* client);
	void handleIconmenu(Client* client);
	void handleAttachClient(Client* client);
	void handleAttachFrame(Client* client);
	void handleAttachClientInFrame(Client* client);
	void handleAttachFrameInFrame(Client* client);

private:
	WindowManager* _wm;
	Client *_client; // holds the focused client ( as menus steals focus )
};

#endif //  _FRAMELISTMENU_HH_

#endif // MENUS
