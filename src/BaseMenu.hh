//
// BaseMenu.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// basemenu.hh for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _BASEMENU_HH_
#define _BASEMENU_HH_

#include "pekwm.hh"
#include "Action.hh"

#include <string>
#include <vector>
#include <map>

class ScreenInfo;
class Theme;
class WindowObject;
class Workspaces;
class Client;

class BaseMenu : public WindowObject
{
public:
	class BaseMenuItem
	{
	public:
		BaseMenuItem(const std::string& nam) :
				name(nam), x(0), y(0),
				selected(false), dynamic(false), client(NULL), submenu(NULL) { }

		std::string name;
		int x, y;
		bool selected, dynamic;

		ActionEvent ae;

		Client *client;
		BaseMenu *submenu;
	};

	BaseMenu(ScreenInfo* s, Theme* t, Workspaces* w, std::string n = "");
	virtual ~BaseMenu();

	// START - WindowObject interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);

	virtual void giveInputFocus(void);
	// END - WindowObject interface.

	static inline BaseMenu *findMenu(Window win) {
		std::map<Window, BaseMenu *>::iterator it = _menu_map.find(win);
		if (it != _menu_map.end())
			return it->second;
		return NULL;
	}

	void loadTheme(void);
	virtual void reload(void) { }

	inline MenuType getMenuType(void) const { return _menu_type; }
	inline BaseMenu* getParent(void) { return _parent; }

	inline const std::string &getName(void) const { return _name; }
	inline void setName(const std::string &n) { _name = n; }

	inline BaseMenuItem *getCurrItem(void) { return _curr; }
	inline unsigned int size(void) const { return _item_list.size(); }
	inline std::vector<BaseMenuItem*> *getItemList(void) { return &_item_list; }

	BaseMenuItem *findMenuItem(int x, int y);

	// Inserts / Remove's
	void updateMenu(void);

	virtual void insert(BaseMenuItem *item);
	virtual void insert(const std::string& name, BaseMenu* sub);
	virtual void insert(const std::string& name, const ActionEvent& ae);
	virtual void insert(const std::string& name, const ActionEvent& ae,
											Client* client);

	virtual void remove(BaseMenuItem *item);
	virtual void removeAll(void);

	// Selection of items
	void selectNextItem(void);
	void selectPrevItem(void);
	void selectItem(unsigned int num);
	void selectItem(BaseMenuItem *item);

	// "Executes" the item
	void execItem(BaseMenuItem *item);

	// Event handlers
	void handleExposeEvent(XExposeEvent *e);
	void handleLeaveEvent(XCrossingEvent *e);
	void handleMotionNotifyEvent(XMotionEvent *e);

	// Virtual Event handlers
	virtual void handleButtonPressEvent(XButtonEvent *e);
	virtual void handleButtonPressEvent(XButtonEvent *e, BaseMenuItem *item);
	virtual void handleButtonReleaseEvent(XButtonEvent *e);
	virtual void handleButtonReleaseEvent(XButtonEvent *e, BaseMenuItem *item);

	// No default Virtual Event handlers
	virtual void handleButton1Press(BaseMenuItem *curr) { }
	virtual void handleButton2Press(BaseMenuItem *curr) { }
	virtual void handleButton3Press(BaseMenuItem *curr) { }
	virtual void handleButton1Release(BaseMenuItem *curr) { }
	virtual void handleButton2Release(BaseMenuItem *curr) { }
	virtual void handleButton3Release(BaseMenuItem *curr) { }

	// Normal control of the menu
	void mapSubmenu(BaseMenu *submenu);
	void mapUnderMouse(void);
	void unmapSubmenus(void);
	void unmapAll(void);

	virtual void redraw(void);
private:
	virtual void redraw(BaseMenuItem *item);

	void makeMenuInsideScreen(void);

private:
	static std::map<Window, BaseMenu*> _menu_map;

protected:
	ScreenInfo *_scr;
	Theme *_theme;
	Workspaces *_workspaces;

	GC _gc;
	XPoint _triangle[3];

	BaseMenu *_parent;
	BaseMenuItem *_curr;
	std::vector<BaseMenuItem*> _item_list;

	MenuType _menu_type;
	std::string _name;

	unsigned int _title_x, _widget_side;
	unsigned int _item_width, _item_height;
};

#endif // _BASEMENU_HH_
