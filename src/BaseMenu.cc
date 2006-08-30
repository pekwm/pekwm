//
// BaseMenu.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// basemenu.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowObject.hh"
#include "BaseMenu.hh"

#include "ScreenInfo.hh"
#include "Theme.hh"
#include "PekwmFont.hh"
#include "Util.hh"
#include "Workspaces.hh"

#include <algorithm>

extern "C" {
#include <X11/cursorfont.h>
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::string;
using std::vector;
using std::map;

map<Window,BaseMenu*> BaseMenu::_menu_map = map<Window, BaseMenu*>();

//! param n Menus name, defaults to ""
BaseMenu::BaseMenu(ScreenInfo *s, Theme *t, Workspaces *w, string n) :
WindowObject(s->getDisplay()),
_scr(s), _theme(t), _workspaces(w),
_gc(None),
_parent(NULL), _curr(NULL),
_menu_type(NO_MENU_TYPE),
_name(n),
_title_x(0), _widget_side(0),
_item_width(0), _item_height(0)
{
	_scr->grabServer();

	// WindowObject attributes
	_type = WO_MENU;
	_layer = LAYER_MENU;
	_iconified = true; // We set ourself iconified for workspace switching.
	_sticky = true;

	// Setup the menu
	XSetWindowAttributes attrib;
	attrib.background_pixel = _theme->getMenuBackground().pixel;
	attrib.border_pixel = _theme->getMenuBorderColor().pixel;
	attrib.override_redirect = true;
	attrib.event_mask =
		ButtonPressMask|ButtonReleaseMask|PointerMotionMask|
		ExposureMask|EnterWindowMask|LeaveWindowMask|
		KeyPressMask|FocusChangeMask;

	_window =
		XCreateWindow(_dpy, _scr->getRoot(), _gm.x, _gm.y, _gm.width, _gm.height,
									0, CopyFromParent,
									InputOutput, CopyFromParent,
									CWBackPixel|CWBorderPixel|CWOverrideRedirect|CWEventMask,
									&attrib);

	loadTheme();

	// setup cursor
	XDefineCursor(_dpy, _window, _scr->getCursor(ScreenInfo::CURSOR_ARROW));

	_scr->ungrabServer(true);

	_workspaces->insert(this); // add to workspaces stacking
	_menu_map[_window] = this; // add to menu map
}

BaseMenu::~BaseMenu()
{
	_menu_map.erase(_window); // remove from menu map
	_workspaces->remove(this); // remove from workspaces stacking

	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for (; it != _item_list.end(); ++it)
		delete *it;
	_item_list.clear();

	XDestroyWindow(_dpy, _window);
	if (_gc != None)
		XFreeGC(_dpy, _gc);
}

// START - WindowObject interface.

//! @fn    void unmapWindow(void)
//! @brief Overloaded mapWindow to make _iconified always set to true
void
BaseMenu::mapWindow(void)
{
	if (_mapped)
		return;

	WindowObject::mapWindow();
	_iconified = true;
}

//! @fn    void unmapWindow(void)
//! @brief Hides the Menu and sets the current item to NULL.
void
BaseMenu::unmapWindow(void)
{
	if (!_mapped)
		return;
	_curr = NULL;
	_iconified = true; // We set ourself iconified for workspace switching.

	WindowObject::unmapWindow();
}


//! @fn    void giveInputFocus(void)
//! @brief Gives focus to the absolute parent menu.
void
BaseMenu::giveInputFocus(void)
{
	if (_parent)
		_parent->giveInputFocus();
	else if (_mapped)
		XSetInputFocus(_dpy, _window, RevertToPointerRoot, CurrentTime);		
}

// END - WindowObject interface.

//! @fn    void loadTheme(void)
//! @brief
void
BaseMenu::loadTheme(void)
{
	if (_gc != None)
		XFreeGC(_dpy, _gc);

	XSetWindowBackground(_dpy, _window, _theme->getMenuBackground().pixel);
	XSetWindowBorder(_dpy, _window, _theme->getMenuBorderColor().pixel);
	XSetWindowBorderWidth(_dpy, _window, _theme->getMenuBorderWidth());

	XGCValues gv;
	gv.function = GXcopy;
	gv.foreground = _theme->getMenuBackground().pixel;

	_gc = XCreateGC(_dpy, _window, GCFunction|GCForeground, &gv);

	// initialize item sizes
	_item_width = _gm.width + _theme->getMenuPadding();
	_item_height = _theme->getMenuFont()->getHeight("@y") +
		_theme->getMenuPadding();
	_widget_side = _theme->getMenuFont()->getHeight("@y") / 2;

	// initialze triangle size
	_triangle[0].x = _triangle[0].y = 0;
	_triangle[1].x = 0;
	_triangle[1].y = _widget_side * 2;
	_triangle[2].x = _widget_side;
	_triangle[2].y = - _widget_side;

	// reload all submenus
	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for (; it != _item_list.end(); ++it) {
		if ((*it)->submenu) {
			(*it)->submenu->loadTheme();
		}
	}
}

//! @fn    BaseMenuItem* findMenuItem(int x, int y)
//! @brief Tries to find a menu item at position xXy
BaseMenu::BaseMenuItem*
BaseMenu::findMenuItem(int x, int y)
{
	if (!_item_list.size())
		return NULL;

	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for (; it != _item_list.end(); ++it) {
		if ((x >= 0) && (x <= signed(_gm.width)) &&
				(y >= signed((*it)->y - _item_height)) &&
				(y <= (*it)->y)) {
			return (*it);
		}
	}

	return NULL;
}

//! @fn    void updateMenu(void)
//! @brief Sets the menu's max width , height etc.
void
BaseMenu::updateMenu(void)
{
	if (_item_list.size()) {
		int tmp_width = 0;
		int padding = _theme->getMenuPadding();

		PekwmFont *font = _theme->getMenuFont(); // convenience

		if (_name.size())
			_gm.width = font->getWidth(_name.c_str());
		else
			_gm.width = 0;

		// check for longest name in the menu
		vector<BaseMenuItem*>::iterator it = _item_list.begin();
		for (; it != _item_list.end(); ++it) {
			tmp_width = font->getWidth((*it)->name.c_str());

			if (tmp_width > signed(_gm.width)) {
				_gm.width = tmp_width;
			}
		}

		// setup item geometry
		_gm.width += padding * 2;
		unsigned int width = _gm.width;
		unsigned int total_height;

		_gm.width += _widget_side + _theme->getMenuPadding();
		_item_width = _gm.width;

		// for the menu title
		if (_name.size()) {
			total_height = _item_height;

			switch (_theme->getMenuFontJustify()) {
			case LEFT_JUSTIFY:
				_title_x = padding;
				break;
			case CENTER_JUSTIFY:
				_title_x = (_gm.width - font->getWidth(_name.c_str())) / 2;
				break;
			case RIGHT_JUSTIFY:
				_title_x = _gm.width - font->getWidth(_name.c_str()) - padding;
				break;
			default:
				// do nothing
				break;
			}
		} else
			total_height = 0;

		// setup item cordinates
		for (it = _item_list.begin(); it != _item_list.end(); ++it) {
			if ((*it)->ae.isOnlyAction(DYNAMIC_MENU)) // TO-DO: Move?
				continue;

			tmp_width = _theme->getMenuFont()->getWidth((*it)->name.c_str());

			switch (_theme->getMenuFontJustify()) {
			case LEFT_JUSTIFY:
				(*it)->x = padding;
				break;
			case CENTER_JUSTIFY:
				(*it)->x = (width - tmp_width) / 2;
				break;
			case RIGHT_JUSTIFY:
				(*it)->x = width - tmp_width - padding;
				break;
			default:
				(*it)->x = padding;
				break;
			}
			total_height += _item_height;
			(*it)->y = total_height;
		}

		_gm.height = total_height;

		makeMenuInsideScreen();

		XClearWindow(_dpy, _window);
		XMoveResizeWindow(_dpy, _window, _gm.x, _gm.y, _gm.width, _gm.height);

	} else {
		_gm.width = 1;
		_gm.height = 1;
		XClearWindow(_dpy, _window);
		XMoveResizeWindow(_dpy, _window, _gm.x, _gm.y, _gm.width, _gm.height);
	}
}

//! @fn    void insert(BaseMenuItem *item)
//! @brief Insert an allready created menuitem in the menu
//! @param item BaseMenuItem to insert
void
BaseMenu::insert(BaseMenuItem *item)
{
	if(item)
		_item_list.push_back(item);
}

//! @fn    void insert(const string &n, BaseMenu *sub)
//! @brief Creates and insert an object in the menu
//! @param n Menu item's name
//! @param sub Submenu this item should "relate" to
void
BaseMenu::insert(const string& name, BaseMenu* sub)
{
	BaseMenuItem *item = new BaseMenuItem(name);

	sub->_parent = this;
	item->submenu = sub;

	insert(item);
}

//! @fn    void insert(const string& name, const ActionEvent& ae)
//! @brief Creates and insert an object in the menu
//! @param name Menu item's name
//! @param ae ActionEvent to associate entry with
void
BaseMenu::insert(const string& name, const ActionEvent& ae)
{
	BaseMenuItem *item = new BaseMenuItem(name);

	item->ae = ae;

	insert(item);
}

//! @fn    void insert(const string& name, const ActionEvent& ae, Client* client)
//! @brief Creates and insert an object in the menu
void
BaseMenu::insert(const string& name, const ActionEvent& ae, Client* client)
{
	BaseMenuItem *item = new BaseMenuItem(name);

	item->ae = ae;
	item->client = client;

	insert(item);
}

//! @fn    void remove(BaseMenuItem *item)
//! @brief Removes a BaseMenuItem from the menu.
void
BaseMenu::remove(BaseMenuItem *item)
{
	if (!item)
		return;

	vector<BaseMenuItem*>::iterator it =
		find(_item_list.begin(), _item_list.end(), item);

	if (it != _item_list.end()) {
		if (*it == _curr)
			_curr = NULL; // make sure we don't point anywhere dangerous

		delete *it;
		_item_list.erase(it);
	}
}

void
BaseMenu::removeAll(void)
{
	if (!_item_list.size())
		return;

	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for (; it != _item_list.end(); ++it)
		delete *it;
	_item_list.clear();

	_curr = NULL; // make sure we don't point anywhere dangerous
	updateMenu();
}

//! @fn    void selectNextItem(void)
//! @brief Selects the next item, wraps.
void
BaseMenu::selectNextItem(void)
{
	if (!_item_list.size())
		return;

	BaseMenuItem *next = NULL;

	if (_curr) {
		vector<BaseMenuItem*>::iterator it =
			find(_item_list.begin(), _item_list.end(), _curr);

		if (++it == _item_list.end())
			it = _item_list.begin();

		next = *it;
	} else
		next = _item_list.front();

	selectItem(next);
}

//! @fn    void selectNextItem(void)
//! @brief Selects the next item, wraps.
void
BaseMenu::selectPrevItem(void)
{
	if (!_item_list.size())
		return;

	BaseMenuItem *prev = NULL;

	if (_curr) {
		vector<BaseMenuItem*>::iterator it =
			find(_item_list.begin(), _item_list.end(), _curr);

		if (it == _item_list.begin())
			prev = _item_list.back();
		else
			prev = (*--it);
	} else
		prev = _item_list.back();

	selectItem(prev);
}

//! @fn		 void selectItem(unsigned int num)
//! @brief Select item number.
void
BaseMenu::selectItem(unsigned int num)
{
	if (num >= _item_list.size())
		return;

	selectItem(_item_list[num]);
}

//! @fn    void selectItem(BaseMenuItem *item)
//! @brief Select item *item
void
BaseMenu::selectItem(BaseMenuItem *item)
{
	if (!item)
		return;

	BaseMenuItem *old_item = _curr;
	_curr = item;

	if (old_item && (old_item != _curr)) {
		if (old_item->submenu && old_item->submenu->isMapped()) {
			old_item->submenu->unmapSubmenus();
			old_item->submenu->unmapWindow();
		}
		redraw(old_item);
	}

	redraw(_curr);
	if (_curr->submenu)
		mapSubmenu(_curr->submenu);
}

//! @fn    void execItem(BaseMenuItem *item)
//! @brief Emulates a Button1 press
void
BaseMenu::execItem(BaseMenuItem *item)
{
	if (!item)
		return;

	XButtonEvent ev;
	ev.button = BUTTON1;

	handleButtonReleaseEvent(&ev, item);
}


//! @fn    void handleExposeEvent(XExposeEvent *e)
//! @brief
void
BaseMenu::handleExposeEvent(XExposeEvent *e)
{
	if (e->count == 0)
		redraw();
}

//! @fn    void handleLeaveEvent(XCrossingEvent *e)
//! @brief Handles XLeaveWindowEvents
void
BaseMenu::handleLeaveEvent(XCrossingEvent *e)
{
	if (!e || !_curr)
		return;

	// If the active item doesn't have a submenu, let's deselect the item.
	if (!_curr->submenu) {
		BaseMenuItem *item = _curr;
		_curr = NULL;
		redraw(item);
	}
}

//! @fn
//! @brief
void
BaseMenu::handleMotionNotifyEvent(XMotionEvent *e)
{
	if (!e || (e->window != _window))
		return;

	// first, lets see if we have the pointer on the menu's title if we have one
	// if that would be the case make sure no item is selected.
	if (_curr && _name.size()) {
		if (e->y < signed(_item_height)) {
			selectItem((BaseMenuItem*) NULL);
		}
	}

	BaseMenuItem *item = findMenuItem(e->x, e->y);
	if (item && (item != _curr)) {
		selectItem(item);
	}
}

//! @fn void handleButtonPressEvent(XButtonEvent *e)
//! @brief Handles button press events
void
BaseMenu::handleButtonPressEvent(XButtonEvent *e)
{
	if (!e || !_curr)
		return;

	// well, if it's a submenu, we'll hide/show that. that's nr1 prioriy
	if (_curr->submenu) {
		if (_curr->submenu->isMapped())
			_curr->submenu->unmapWindow();
		else
			_curr->submenu->mapWindow();
	}	else if (_parent) {
		_parent->handleButtonPressEvent(e, _curr);
	} else {
		handleButtonPressEvent(e, _curr);
	}
}

//! @fn void handleButtonPressEvent(XButtonEvent *e, BaseMenuItem *item)
//! @brief Handles button press events sent by a child
void
BaseMenu::handleButtonPressEvent(XButtonEvent *e, BaseMenuItem *item)
{
	if (!e || !item)
		return;

	if (_parent) {
		_parent->handleButtonPressEvent(e, item);
	} else {
		switch (e->button) {
		case BUTTON1:
			handleButton1Press(item);
			break;
		case BUTTON2:
			handleButton2Press(item);
			break;
		case BUTTON3:
			handleButton3Press(item);
			break;
		}
	}
}

void
BaseMenu::handleButtonReleaseEvent(XButtonEvent *e)
{
	if (!e || !_curr)
		return;

	// if it's a submenu we hide it when we got the button press event
	if (_curr->submenu)
		return;

	// if we have a parent menu, well send the event to that one
	if (_parent) {
		_parent->handleButtonReleaseEvent(e, _curr);
	} else {
		handleButtonReleaseEvent(e, _curr);
	}
}

void
BaseMenu::handleButtonReleaseEvent(XButtonEvent *e, BaseMenuItem *item)
{
	if (!e || !item)
		return;

	if (_parent) {
		_parent->handleButtonReleaseEvent(e, item);
	} else {
		unmapAll();

		switch (e->button) {
		case BUTTON1:
			handleButton1Release(item);
			break;
		case BUTTON2:
			handleButton1Release(item);
			break;
		case BUTTON3:
			handleButton3Release(item);
			break;
		}
	}
}

//! @fn    void mapSub(BaseMenu *submenu)
//! @brief Maps the BaseMenu sub aligned to this menu.
void
BaseMenu::mapSubmenu(BaseMenu *submenu)
{
	if (!submenu || (submenu == this))
		return;

	// Setup the menu's position
	submenu->_gm.x = _gm.x + _gm.width + (_theme->getMenuBorderWidth() * 2);
	if (_curr) {
		submenu->_gm.y = _gm.y + _curr->y - _item_height -
			(submenu->getName().size() ? _item_height : 0);
	} else {
		submenu->_gm.y = _gm.y;
	}
	submenu->makeMenuInsideScreen();

	submenu->move(submenu->getX(), submenu->getY());
	submenu->mapWindow();
}

//! @fn    void mapUnderMouse(void)
//! @brief Maps the window under the mouse.
void
BaseMenu::mapUnderMouse(void)
{
	_gm.x = _gm.y = 0;
	_scr->getMousePosition(_gm.x, _gm.y);
	makeMenuInsideScreen();

	move(_gm.x, _gm.y);

	mapWindow();
}


//! @fn    void unmapSubmenus(void)
//! @brief
void
BaseMenu::unmapSubmenus(void)
{
	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for (;it != _item_list.end(); ++it) {
		if ((*it)->submenu) {
			(*it)->submenu->unmapSubmenus();
			(*it)->submenu->unmapWindow();
		}
	}
}

//! @fn    void unmapAll(void)
//! @brief
void
BaseMenu::unmapAll(void)
{
	if (_parent) {
		_parent->unmapAll();
	} else {
		unmapSubmenus();
		unmapWindow();
	}
}

//! @fn    void redraw(void)
//! @brief Redraws the entire contents of the menu.
void
BaseMenu::redraw(void)
{
	if (!_item_list.size())
		return;

	XClearWindow(_dpy, _window);

	PekwmFont *font = _theme->getMenuFont(); // convenience

	// draw the title
	if (_name.size()) {
		XSetForeground(_dpy, _gc, _theme->getMenuBackgroundTi().pixel);
		XFillRectangle(_dpy, _window, _gc, 0, 0, _item_width, _item_height);
		XSetForeground(_dpy, _gc, _theme->getMenuBorderColor().pixel);
		XDrawLine(_dpy, _window, _gc,
							0, _item_height - 1, _item_width, _item_height - 1);
		font->setColor(_theme->getMenuTextColorTi());
		font->draw(_window, _title_x, _theme->getMenuPadding() / 2, _name.c_str());
		font->setColor(_theme->getMenuTextColor());
	}

	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for(; it != _item_list.end(); ++it) {
		if ((*it) == _curr) { // selected item
			XSetForeground(_dpy, _gc, _theme->getMenuBackgroundSe().pixel);
			XFillRectangle(_dpy, _window, _gc,
										 0, _curr->y - _item_height, _item_width, _item_height);
			XSetForeground(_dpy, _gc, _theme->getMenuBorderColor().pixel);
			XDrawLine(_dpy, _window, _gc,
								0, _curr->y - 1, _item_width, _curr->y - 1);
			XDrawLine(_dpy, _window, _gc,
								0, _curr->y - _item_height,
								_item_width, _curr->y - _item_height);

			font->setColor(_theme->getMenuTextColorSe());
		}

		font->draw(_window,
							 (*it)->x,
							 (*it)->y - _item_height + (_theme->getMenuPadding() / 2),
							 (*it)->name.c_str());

		if ((*it) == _curr) // restore the font color
			font->setColor(_theme->getMenuTextColor());

		if ((*it)->submenu) {
			_triangle[0].x = _gm.width - _widget_side - _theme->getMenuPadding();
			_triangle[0].y = (*it)->y -  _item_height +
				(_theme->getMenuPadding() / 2);

			XSetForeground(_dpy, _gc, _theme->getMenuTextColor().pixel);
			XFillPolygon(_dpy, _window, _gc, _triangle, 3,
									 Convex, CoordModePrevious);
			XSetForeground(_dpy, _gc, _theme->getMenuBackground().pixel);
		}
	}
}

//! @fn    void redraw(BaseMenuItem *item)
//! @brief Redraws a single menu item on the menu.
void
BaseMenu::redraw(BaseMenuItem *item)
{
	if (!item)
		return;

	if (item == _curr) { // selected item
		XSetForeground(_dpy, _gc, _theme->getMenuBackgroundSe().pixel);
		XFillRectangle(_dpy, _window, _gc,
									 0, _curr->y - _item_height, _item_width, _item_height);
		XSetForeground(_dpy, _gc, _theme->getMenuBorderColor().pixel);
		XDrawLine(_dpy, _window, _gc,
							0, item->y - 1, _item_width, item->y - 1);
		XDrawLine(_dpy, _window, _gc,
							0, item->y - _item_height, _item_width, item->y - _item_height);

		_theme->getMenuFont()->setColor(_theme->getMenuTextColorSe());		
	} else {
		XSetForeground(_dpy, _gc, _theme->getMenuBackground().pixel);
		XFillRectangle(_dpy, _window, _gc,
									 0, item->y - _item_height, _item_width, _item_height);
	}


	_theme->getMenuFont()->draw(_window,
														 item->x, item->y - _item_height +
														 (_theme->getMenuPadding() / 2),
														 item->name.c_str());

	if (item == _curr) // restore the font color
		_theme->getMenuFont()->setColor(_theme->getMenuTextColor());

	if (item->submenu) {
		_triangle[0].x = _gm.width - _widget_side - _theme->getMenuPadding();
		_triangle[0].y = item->y -  _item_height + (_theme->getMenuPadding() / 2);

		XSetForeground(_dpy, _gc, _theme->getMenuTextColor().pixel);
		XFillPolygon(_dpy, _window, _gc, _triangle, 3, Convex, CoordModePrevious);
		XSetForeground(_dpy, _gc, _theme->getMenuBackground().pixel);
	}
}



//! @fn    void makeMenuInsideScreen(void)
//! @brief Makes sure the menu is insede the screen/(current head).
//! Makes sure the menu is insede the screen/(current head).
//! NOTE: it doesn't move the window, just updates it's coordinates
void
BaseMenu::makeMenuInsideScreen(void)
{
	unsigned int width = _gm.width + (_theme->getMenuBorderWidth() * 2);
	unsigned int height = _gm.height + (_theme->getMenuBorderWidth() * 2);

#ifdef XINERAMA
	Geometry head;
	unsigned int head_nr = 0;
	if (_scr->hasXinerama())
		head_nr = _scr->getCurrHead();
	_scr->getHeadInfo(head_nr, head);

	if ((_gm.x + width) >= (head.x + head.width)) { // right edge
		if (_parent)
			_gm.x = _parent->getX() - width;
		else
			_gm.x = head.x + head.width - width;
	}

	if ((_gm.y + height) >= (head.y + head.height)) // bottom edge
		_gm.y = head.y + head.height - height;

#else // !XINERAMA
	if ((_gm.x + width) >= _scr->getWidth()) { // right edge
		if (_parent)
			_gm.x = _parent->getX() - width;
		else
			_gm.x = _scr->getWidth() - width;
	}

	if ((_gm.y + height) >= _scr->getHeight()) // bottom edge
		_gm.y = _scr->getHeight() - height;
#endif // XINERAMA
}
