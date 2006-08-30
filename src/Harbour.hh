//
// Harbour.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HARBOUR

#ifndef _HARBOUR_HH_
#define _HARBOUR_HH_

#include "pekwm.hh"

class ScreenInfo;
class Theme;
class Config;
class Workspaces;
class DockApp;
#ifdef MENUS
class BaseMenu;
class HarbourMenu;
#endif // MENUS

#include <list>

class Harbour
{
public:
	Harbour(ScreenInfo *s, Config *c, Theme *t, Workspaces *w);
	~Harbour();

	void addDockApp(DockApp* da);
	void removeDockApp(DockApp* da);
	void removeAllDockApps(void);

	DockApp* findDockApp(Window win);
	DockApp* findDockAppFromFrame(Window win);

#ifdef MENUS
	HarbourMenu* getHarbourMenu(void) { return _harbour_menu; }
#endif // MENUS

	inline unsigned int getSize(void) const { return _size; }

	void restack(void);
	void rearrange(void);
	void loadTheme(void);
	void updateHarbourSize(void);

	void handleButtonEvent(XButtonEvent* ev, DockApp* da);
	void handleMotionNotifyEvent(XMotionEvent* ev, DockApp* da);
	void handleConfigureRequestEvent(XConfigureRequestEvent* ev, DockApp* da);

private:
	void placeDockApp(DockApp* da);

private:
	ScreenInfo *_scr;
	Config *_cfg;
	Theme *_theme;
	Workspaces *_workspaces;

	std::list<DockApp*> _dockapp_list;
#ifdef MENUS
	HarbourMenu *_harbour_menu;
#endif // MENUS

	unsigned int _size;
	int _last_button_x, _last_button_y;
};

#endif // _HARBOUR_HH_

#endif // HARBOUR
