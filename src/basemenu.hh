//
// basemenu.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// basemenu.hh for aewm++
// Copyright (C) 2000 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifdef MENUS
 
#ifndef _BASEMENU_HH_
#define _BASEMENU_HH_

#include "pekwm.hh"
#include "screeninfo.hh"
#include "actionhandler.hh"

#include <string>
#include <vector>

class Theme;
class Client;

class BaseMenu
{
public:
	class BaseMenuItem 
	{
	public:	
		BaseMenuItem() : m_name(""),
										 m_x(0), m_y(0),
										 m_is_selected(false),
										 m_client(NULL), m_submenu(NULL) { }

		inline const std::string &getName(void) const { return m_name; }
		inline int getX(void) const { return m_x; }
		inline int getY(void) const { return m_y; }
		inline bool isSelected(void) const { return m_is_selected; }
		inline Action *getAction(void) { return &m_action; }

		inline Client* getClient(void) const { return m_client; }
		inline BaseMenu* getSubmenu(void) const { return m_submenu; }

		inline void setX(int x) { m_x = x; }
		inline void setY(int y) { m_y = y; }
		inline void setSelected(bool selected) { m_is_selected = selected; }
		inline void setName(const std::string &name) { m_name = name; }
		inline void setClient(Client *client) { m_client = client; }
		inline void setSubmenu(BaseMenu *submenu) { m_submenu = submenu; }

	private:
		std::string m_name; // name showing on menu.
		int m_x, m_y; // position

		bool m_is_selected;

		Action m_action; // action to do

		Client *m_client;  // client this item will do actions on
		BaseMenu *m_submenu; // submenu this item points to.
	};


	BaseMenu(ScreenInfo *s, Theme *t);
	virtual ~BaseMenu();
	void loadTheme(void);

	inline int getX(void) const { return m_x; }
	inline int getY(void) const { return m_y; }
	inline unsigned int getWidth(void) const { return m_width; }
	inline unsigned int getHeight(void) const { return m_height; }

	inline std::vector<BaseMenuItem*> *getMenuItemList(void) {
		return &m_item_list; }
	inline int getItemCount(void) const { return m_item_list.size(); }
	inline const Window &getMenuWindow(void) const { return m_item_window; }
	
	inline bool isVisible(void) const { return m_is_visible; }
					
	void setMenuPos(int x, int y);
	void show(void);
	void show(int x, int y);
	void showSub(BaseMenu *sub, int x, int y);
	void hide(BaseMenu *sub);
	void hideAll(void);
	
	void updateMenu(void);

	virtual void insert(const std::string &n, BaseMenu *sub);
	virtual void insert(const std::string &n,
											const std::string &exec, Actions action);
	virtual void insert(const std::string &n, int param, Actions action);
	virtual void insert(BaseMenuItem *item);

	unsigned int remove(BaseMenuItem *element);
	void removeAll(void);

	BaseMenuItem *findMenuItem(int x, int y);

	virtual void handleButtonPressEvent(XButtonEvent *e);
	virtual void handleButtonPressEvent(XButtonEvent *e, BaseMenuItem *item);
	virtual void handleButtonReleaseEvent(XButtonEvent *e);
	virtual void handleButtonReleaseEvent(XButtonEvent *e, BaseMenuItem *item);

	void handleEnterNotify(XCrossingEvent *e);
	void handleLeaveNotify(XCrossingEvent *e);
	void handleExposeEvent(XExposeEvent *e);
	void handleMotionNotifyEvent(XMotionEvent *e);

	// The menu item behavoir is defined with these
	// virtual functions in a derived class.

	// all these have no default behaviour
	virtual void handleButton1Press(BaseMenuItem *curr) { }
	virtual void handleButton2Press(BaseMenuItem *curr) { }
	virtual void handleButton3Press(BaseMenuItem *curr) { }
	virtual void handleButton1Release(BaseMenuItem *curr) { }
	virtual void handleButton2Release(BaseMenuItem *curr) { }
	virtual void handleButton3Release(BaseMenuItem *curr) { }

	virtual void redraw(void);
	void hide(void);

private:
	virtual void redraw(BaseMenuItem *item);
		
	void selectMenuItem(bool select);
	bool getMousePosition(int *x, int *y);
	void makeMenuInsideScreen(void);

	// Not meant to be called directly by subclasses! Used internally.
	void hideSubmenus(); 

protected:
	ScreenInfo *scr;
	Theme *theme;

	Display *dpy;
	Window  root;

	std::vector<BaseMenuItem*> m_item_list;
	BaseMenu *m_parent;

	Window m_item_window;
	int m_x, m_y;
	int x_move, y_move;
	unsigned int m_width, m_height, m_total_item_height;
	bool m_is_visible, m_theme_is_loaded;

	Cursor m_curs;
	GC m_gc;
	
	unsigned int m_item_width, m_item_height;
	unsigned int m_widget_side;
	
	// Used to know which item to paint.
	BaseMenuItem *m_curr;

	// This is for our synthetic enter event and we only want this to happen once
	// This is set to true once we detect the mouse has entered the item window
	bool m_enter_once;	
	bool m_bottom_edge, m_right_edge;
};

#endif // _BASEMENU_HH_

#endif // MENUS
