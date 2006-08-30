//
// ActionMenu.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef MENUS

#ifndef _ACTIONMENU_HH_
#define _ACTIONMENU_HH_

#include "pekwm.hh"
#include "BaseConfig.hh"
#include "Config.hh" // For ActionOk

#include <string>
#include <list>

class BaseMenu;
class BaseMenuItem;
class ScreenInfo;
class Theme;
class Config;
class ActionHandler;
class Client;
class WindowManager;

class ActionMenu : public BaseMenu
{
public:
	ActionMenu(WindowManager *w, MenuType type, const std::string &name);
	virtual ~ActionMenu();

	// START - WindowObject interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	// END - WindowObject interface.

	virtual void handleButton1Release(BaseMenuItem *curr);

	virtual void reload(void);
	virtual void insert(BaseMenuItem *item);

	virtual void remove(BaseMenuItem *item);
	virtual void removeAll(void);

	inline Client* getClient(void) { return _client; }
	inline void setClient(Client *c) { _client = c; }

private:
	void parse(BaseConfig::CfgSection *cs, bool dynamic = false);

	void rebuildDynamic(void);
	void removeDynamic(void);

private:
	WindowManager *_wm;
	Config *_cfg;
	ActionHandler *_act;

	Client *_client; // to perform actions on

	ActionOk _action_ok;
	std::vector<BaseMenuItem*>::iterator _insert_at;

	bool _has_dynamic, _insert_dynamic;
};

#endif // _ACTIONMENU_HH_

#endif // MENUS
