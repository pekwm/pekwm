//
// basemenu.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// basemenu.cc for aewm++
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

#include "pekwm.hh"
#include "basemenu.hh"
#include "theme.hh"
#include "util.hh"

#include <algorithm>
#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

#include <X11/cursorfont.h>

using std::string;
using std::vector;

//! param n Menus name, defaults to ""
BaseMenu::BaseMenu(ScreenInfo *s, Theme *t, string n) :
scr(s), theme(t),
m_parent(NULL),
m_name(n),
m_item_window(None),
m_x(0), m_y(0), m_width(1), m_height(1),
m_total_item_height(0), m_title_x(0),
m_is_visible(false), m_theme_is_loaded(false),
m_item_width(0), m_item_height(0), m_widget_side(0),
m_curr(NULL)
{
	dpy = scr->getDisplay();
	root = scr->getRoot();

	XGrabServer(dpy);

	// Setup the menu
	XSetWindowAttributes attrib;
	attrib.background_pixel = theme->getMenuBackground().pixel;
	attrib.border_pixel = theme->getMenuBorderColor().pixel;
	attrib.override_redirect = true;
	attrib.event_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		PointerMotionMask|ExposureMask|LeaveWindowMask;

	m_item_window =
		XCreateWindow(dpy, root, m_x, m_y, m_width, m_height,
									0, CopyFromParent,
									InputOutput, CopyFromParent,
									CWBackPixel|CWBorderPixel|CWOverrideRedirect|CWEventMask,
									&attrib);

	loadTheme();

	// setup cursor
	m_curs = XCreateFontCursor(dpy, XC_left_ptr);
	XDefineCursor(dpy, m_item_window, m_curs);

	XSync(dpy, false);
	XUngrabServer(dpy);
}

BaseMenu::~BaseMenu()
{
	vector<BaseMenuItem*>::iterator it = m_item_list.begin();
	for (; it != m_item_list.end(); ++it) {
		delete *it;
	}
	m_item_list.clear();

	XDestroyWindow(dpy, m_item_window);
	XFreeGC(scr->getDisplay(), m_gc);
	XFreeCursor(scr->getDisplay(), m_curs);
}

void
BaseMenu::loadTheme(void)
{
	if (m_theme_is_loaded)
		XFreeGC(scr->getDisplay(), m_gc);

	XSetWindowBackground(dpy, m_item_window, theme->getMenuBackground().pixel);
	XSetWindowBorder(dpy, m_item_window, theme->getMenuBorderColor().pixel);
	XSetWindowBorderWidth(dpy, m_item_window, theme->getMenuBorderWidth());

	XGCValues gv;
	gv.function = GXcopy;
	gv.foreground = theme->getMenuBackground().pixel;

	m_gc = XCreateGC(dpy, m_item_window, GCFunction|GCForeground, &gv);

	// initialize item sizes
	m_item_width = m_width + theme->getMenuPadding();
	m_item_height = theme->getMenuFont()->getHeight("@y") +
		theme->getMenuPadding();
	m_widget_side = theme->getMenuFont()->getHeight("@y") / 2;

	m_theme_is_loaded = true;

	// reload all submenus
	vector<BaseMenuItem*>::iterator it = m_item_list.begin();
	for (; it != m_item_list.end(); ++it) {
		if ((*it)->getSubmenu()) {
			(*it)->getSubmenu()->loadTheme();
		}
	}
}

//! @fn    void updateMenu(void)
//! @brief Sets the menu's max width , height etc.
void
BaseMenu::updateMenu(void)
{
	if (m_item_list.size()) {
		int tmp_width = 0;
		int padding = theme->getMenuPadding();

		PekFont *font = theme->getMenuFont(); // convinience

		if (m_name.size())
			m_width = font->getWidth(m_name);
		else
			m_width = 0;

		// check for longest name in the menu
		vector<BaseMenuItem*>::iterator it = m_item_list.begin();
		for (; it != m_item_list.end(); ++it) {
			tmp_width = font->getWidth((*it)->getName());

			if (tmp_width > (signed) m_width) {
				m_width = tmp_width;
			}
		}

		// setup item geometry
		m_width += padding * 2;
		unsigned int width = m_width;

		m_width += m_widget_side + theme->getMenuPadding();
		m_item_width = m_width;
		m_height = m_item_height * m_item_list.size();

		// for the menu title
		if (m_name.size()) {
			m_height += m_item_height;
			m_total_item_height = m_item_height;

			switch (theme->getMenuFontJustify()) {
			case LEFT_JUSTIFY:
				m_title_x = padding;
				break;
			case CENTER_JUSTIFY:
				m_title_x = (m_width - font->getWidth(m_name)) / 2;
				break;
			case RIGHT_JUSTIFY:
				m_title_x = m_width - font->getWidth(m_name) - padding;
				break;
			default:
				// do nothing
				break;
			}
		} else
			m_total_item_height = 0;

		// setup item cordinates
		for (it = m_item_list.begin(); it != m_item_list.end(); ++it) {
			tmp_width = theme->getMenuFont()->getWidth((*it)->getName());

			switch (theme->getMenuFontJustify()) {
			case LEFT_JUSTIFY:
				(*it)->setX(padding);
				break;
			case CENTER_JUSTIFY:
				(*it)->setX((width - tmp_width) / 2);
				break;
			case RIGHT_JUSTIFY:
				(*it)->setX(width - tmp_width - padding);
				break;
			default:
				(*it)->setX(padding);
				break;
			}
			m_total_item_height += m_item_height;
			(*it)->setY(m_total_item_height);
		}

		makeMenuInsideScreen();

		XClearWindow(dpy, m_item_window);
		XMoveResizeWindow(dpy, m_item_window, m_x, m_y, m_width, m_height);

	} else {
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "updateMenu(" << this << ") without content." << endl;
#endif // DEBUG

		m_width = 1;
		m_height = 1;
		XClearWindow(dpy, m_item_window);
		XMoveResizeWindow(dpy, m_item_window, m_x, m_y, m_width, m_height);
	}
}

//! @fn    void hide(void)
//! @brief Hides the menu
void
BaseMenu::hide(void)
{
	XUnmapWindow(dpy, m_item_window);

	m_is_visible = false;
}

//! @fn    void redraw(void)
//! @brief Redraws the entire contents of the menu.
void
BaseMenu::redraw(void)
{
	if (!m_item_list.size())
		return;

	XClearWindow(dpy, m_item_window);

	PekFont *font = theme->getMenuFont(); // convinience

	// draw the title
	if (m_name.size()) {
		XSetForeground(dpy, m_gc, theme->getMenuBackgroundSelected().pixel);
		XFillRectangle(dpy, m_item_window, m_gc,
									 0, 0, m_item_width, m_item_height);
		XSetForeground(dpy, m_gc, theme->getMenuBorderColor().pixel);
		XDrawLine(dpy, m_item_window, m_gc,
							0, m_item_height - 1, m_item_width, m_item_height - 1);
		font->draw(m_item_window, m_title_x, theme->getMenuPadding() / 2, m_name);
	}



	vector<BaseMenuItem*>::iterator it = m_item_list.begin();
	for(; it != m_item_list.end(); ++it) {
		if ((*it) == m_curr) { // selected item
			XSetForeground(dpy, m_gc, theme->getMenuBackgroundSelected().pixel);
			XFillRectangle(dpy, m_item_window, m_gc,
										 0, m_curr->getY() - m_item_height,
										 m_item_width, m_item_height);

			XSetForeground(dpy, m_gc, theme->getMenuBorderColor().pixel);
			XDrawLine(dpy, m_item_window, m_gc,
								0, m_curr->getY() - 1, m_item_width, m_curr->getY() - 1);
			XDrawLine(dpy, m_item_window, m_gc,
								0, m_curr->getY() - m_item_height,
								m_item_width, m_curr->getY() - m_item_height);
		}

		font->draw(m_item_window,
							 (*it)->getX(),
							 (*it)->getY() - m_item_height + (theme->getMenuPadding() / 2),
							 (*it)->getName());

		if ((*it)->getSubmenu()) {
			XPoint triangle[3];
			triangle[0].x = m_width - m_widget_side - theme->getMenuPadding();
			triangle[0].y = (*it)->getY() -  m_item_height +
				(theme->getMenuPadding() / 2);
			triangle[1].x = 0;
			triangle[1].y = (m_widget_side * 2);
			triangle[2].x = m_widget_side;
			triangle[2].y = -m_widget_side;

			XSetForeground(dpy, m_gc, theme->getMenuTextColor().pixel);
			XFillPolygon(dpy, m_item_window, m_gc, triangle, 3,
									 Convex, CoordModePrevious);
			XSetForeground(dpy, m_gc, theme->getMenuBackground().pixel);
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

	if (item == m_curr) { // selected item
		XSetForeground(dpy, m_gc, theme->getMenuBackgroundSelected().pixel);
		XFillRectangle(dpy, m_item_window, m_gc,
									 0, m_curr->getY() - m_item_height,
									 m_item_width, m_item_height);

		XSetForeground(dpy, m_gc, theme->getMenuBorderColor().pixel);
		XDrawLine(dpy, m_item_window, m_gc,
							0, item->getY() - 1, m_item_width, item->getY() - 1);
		XDrawLine(dpy, m_item_window, m_gc,
							0, item->getY() - m_item_height,
							m_item_width, item->getY() - m_item_height);
	} else {
		XSetForeground(dpy, m_gc, theme->getMenuBackground().pixel);
		XFillRectangle(dpy, m_item_window, m_gc,
									 0, item->getY() - m_item_height,
									 m_item_width, m_item_height);
	}


	theme->getMenuFont()->draw(m_item_window,
														 item->getX(), item->getY() - m_item_height +
														 (theme->getMenuPadding() / 2),
														 item->getName());

	if(item->getSubmenu()) {
		XPoint triangle[3];
		triangle[0].x = m_width - m_widget_side - theme->getMenuPadding();
		triangle[0].y = item->getY() -  m_item_height +
			(theme->getMenuPadding() / 2);
		triangle[1].x = 0;
		triangle[1].y = (m_widget_side * 2);
		triangle[2].x = m_widget_side;
		triangle[2].y = -m_widget_side;

		XSetForeground(dpy, m_gc, theme->getMenuTextColor().pixel);
		XFillPolygon(dpy, m_item_window, m_gc, triangle, 3,
								 Convex, CoordModePrevious);
		XSetForeground(dpy, m_gc, theme->getMenuBackground().pixel);
	}
}

//! @fn    void insert(const string &n, BaseMenu *sub)
//! @brief Creates and insert an object in the menu
//! @param n Menu item's name
//! @param sub Submenu this item should "relate" to
void
BaseMenu::insert(const string &n, BaseMenu *sub)
{
	BaseMenuItem *item = new BaseMenuItem();

	sub->m_parent = this;

	item->setName(n);
	item->setSubmenu(sub);

	// defaults
	item->setSelected(false);

	item->getAction()->action = NO_ACTION;
	item->getAction()->s_param = "";
	item->setX(0);
	item->setY(0);

	m_item_list.push_back(item);
}

//! @fn    void insert(const string &n, const string &exec, Actions action)
//! @brief Creates and insert an object in the menu
//! @param n Menu item's name
//! @param exec action's string param
//! @param action Action to execute on click
void
BaseMenu::insert(const string &n, const string &exec, Actions action)
{
	BaseMenuItem *item = new BaseMenuItem();

	item->setName(n);

	item->getAction()->action = action;
	item->getAction()->s_param = exec;
	Util::expandFileName(item->getAction()->s_param); // make sure we expand ~

	// defaults
	item->setSelected(false);
	item->setSubmenu(NULL);
	item->setX(0);
	item->setY(0);

	m_item_list.push_back(item);
}

//! @fn    void insert(const string &n, int param, Actions action)
//! @brief Creates and insert an object in the menu
//! @param n Menu item's name
//! @param param action's int param
//! @param action Action to execute on click
void
BaseMenu::insert(const string &n, int param, Actions action)
{
	BaseMenuItem *item = new BaseMenuItem();

	item->setName(n);
	item->getAction()->action = action;
	item->getAction()->i_param = param;

	// defaults
	item->setSelected(false);
	item->setSubmenu(NULL);
	item->setX(0);
	item->setY(0);

	m_item_list.push_back(item);
}

//! @fn    void insert(BaseMenuItem *item)
//! @brief Insert an allready created menuitem in the menu
//! @param item BaseMenuItem to insert
void
BaseMenu::insert(BaseMenuItem *item)
{
	if(item)
		m_item_list.push_back(item);
}

unsigned int
BaseMenu::remove(BaseMenuItem *element)
{
	if (!element)
		return m_item_list.size();

	vector<BaseMenuItem*>::iterator it =
		find(m_item_list.begin(), m_item_list.end(), element);

	if (it != m_item_list.end()) {
		if (*it == m_curr)
			m_curr = NULL; // make sure we don't point anywhere dangerous

		delete *it;
		m_item_list.erase(it);
	}

	return m_item_list.size();
}

void
BaseMenu::removeAll(void)
{
	if (!m_item_list.size())
		return;

	vector<BaseMenuItem*>::iterator it = m_item_list.begin();
	for (; it != m_item_list.end(); ++it)
		delete *it;
	m_item_list.clear();

	m_curr = NULL; // make sure we don't point anywhere dangerous

	updateMenu();
}

//! @fn    void show(void)
//! @brief Shows the window at it's current position.
void
BaseMenu::show(void)
{
	XMapRaised(dpy, m_item_window);
	m_is_visible = true;
}

//! @fn    void showUnderMouse(void)
//! @brief Shows the window under the mouse.
void
BaseMenu::showUnderMouse(void)
{
	if (!getMousePosition(&m_x, &m_y))
		m_x = m_y = 0;
	makeMenuInsideScreen();

	XMoveWindow(dpy, m_item_window, m_x, m_y);
	XMapRaised(dpy, m_item_window);

	m_is_visible = true;
}

//! @fn    void showSub(BaseMenu *sub)
//! @brief Shows the BaseMenu sub aligned to this menu.
void
BaseMenu::showSub(BaseMenu *sub)
{
	if (!sub || (sub == this))
		return;

	// Setup the menu's position
	sub->m_x = m_x + m_width + (theme->getMenuBorderWidth() * 2);
	if (m_curr) {
		sub->m_y = m_y + m_curr->getY() - m_item_height -
			(sub->getName().size() ? m_item_height : 0);
	} else {
		sub->m_y = m_y;
	}
	sub->makeMenuInsideScreen();

	XMoveWindow(dpy, sub->getMenuWindow(), sub->getX(), sub->getY());
	XMapRaised(dpy, sub->getMenuWindow());

	sub->m_is_visible = true;
}

void
BaseMenu::hide(BaseMenu *sub)
{
	if (sub->m_curr)
		sub->m_curr = NULL;

	if (sub->m_is_visible) {
		// hide the menu windows
		XUnmapWindow(dpy, sub->getMenuWindow());

		sub->m_is_visible = false;
	}
}

BaseMenu::BaseMenuItem*
BaseMenu::findMenuItem(int x, int y)
{
	if (!m_item_list.size())
		return NULL;

	vector<BaseMenuItem*>::iterator it = m_item_list.begin();
	for (; it != m_item_list.end(); ++it) {

		if ( (x >= 0) && (x <= (signed) m_width) &&
				 (y >= (signed) ((*it)->getY() - m_item_height)) &&
				 (y <= (*it)->getY())) {

			return (*it);
		}
	}

	return NULL;
}


//! @fn void handleButtonPressEvent(XButtonEvent *e)
//! @brief Handles button press events
//! Handles button presse events by sending the event to the parent if we
//! have any, else it'll handle the event itself
//! @param e XButtonEvent to handle
void
BaseMenu::handleButtonPressEvent(XButtonEvent *e)
{
	if (!e || !m_curr)
		return;

	// well, if it's a submenu, we'll hide/show that. that's nr1 prioriy
	if (m_curr->getSubmenu()) {
		if (m_curr->getSubmenu()->isVisible()) {
			m_curr->getSubmenu()->hide();
		} else {
			m_curr->getSubmenu()->show();
		}
	}	else if (m_parent) {
		m_parent->handleButtonPressEvent(e, m_curr);

	} else {
		switch (e->button) {
		case Button1:
			handleButton1Press(m_curr);
			break;

		case Button2:
			handleButton2Press(m_curr);
			break;

		case Button3:
			handleButton3Press(m_curr);
			break;
		}
	}
}

//! @fn void handleButtonPressEvent(XButtonEvent *e, BaseMenuItem *item)
//! @brief Handles button press events sent by a child
//! Handles button presse events by sending the event to the parent if we
//! have any, else it'll handle the event itself
//! @param e XButtonEvent to handle
//! @param item BaseMenuItem to execute the action on
void
BaseMenu::handleButtonPressEvent(XButtonEvent *e, BaseMenuItem *item)
{
	if (!e || !item)
		return;

	if (m_parent) {
		m_parent->handleButtonPressEvent(e, item);

	} else {

		switch (e->button) {
		case Button1:
			handleButton1Press(item);
			break;
		case Button2:
			handleButton2Press(item);
			break;

		case Button3:
			handleButton3Press(item);
			break;
		}
	}
}

void
BaseMenu::handleButtonReleaseEvent(XButtonEvent *e)
{
	if (!e || !m_curr)
		return;

	// if it's a submenu we hide it when we got the button press event
	if (m_curr->getSubmenu())
		return;

	// if we have a parent menu, well send the event to that one
	if (m_parent) {
		m_parent->handleButtonReleaseEvent(e, m_curr);

	} else {
		// hide() unsets the select item, therefore I need to store
		// the select one before hiding
		BaseMenuItem *item = m_curr;

		hideAll();

		switch (e->button) {
		case Button1:
			handleButton1Release(item);
			break;

		case Button2:
			handleButton2Release(item);
			break;

		case Button3:
			handleButton3Release(item);
			break;
		}
	}
}

void
BaseMenu::handleButtonReleaseEvent(XButtonEvent *e, BaseMenuItem *item)
{
	if (!e || !item)
		return;

	if (m_parent) {
		m_parent->handleButtonReleaseEvent(e, item);

	} else {
		hideAll();

		switch (e->button) {
		case Button1:
			handleButton1Release(item);
			break;

		case Button2:
			handleButton1Release(item);
			break;

		case Button3:
			handleButton3Release(item);
			break;
		}
	}
}

//! @fn    void move(int x, int y)
//! @brief Moves the menu to x and y
void
BaseMenu::move(int x, int y)
{
	m_x = x;
	m_y = y;

	XMoveWindow(dpy, m_item_window, m_x, m_y);
}

//! @fn
//! @brief
void
BaseMenu::hideAll(void)
{
	hide(this);
	hideSubmenus();
}

//! @fn
//! @brief
void
BaseMenu::hideSubmenus(void)
{
	vector<BaseMenuItem*>::iterator it = m_item_list.begin();
	for (; it != m_item_list.end(); ++it) {
		if ((*it)->getSubmenu()) {
			(*it)->getSubmenu()->hideAll();
		}
	}
}

//! @fn
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
	if (!e || !m_curr)
		return;

	// If the active item doesn't have a submenu, let's deselect the item.
	if (!m_curr->getSubmenu()) {
		BaseMenuItem *item = m_curr;
		m_curr = NULL;
		redraw(item);
	}
}

//! @fn
//! @brief
void
BaseMenu::handleMotionNotifyEvent(XMotionEvent *e)
{
	if (!e || (e->window != m_item_window))
		return;

	// first, lets see if we have the pointer on the menu's title if we have one
	// if that would be the case make sure no item is selected.
	if (m_name.size()) {
		if (e->y < (signed) m_item_height) {
			if (m_curr) {
				BaseMenuItem *old_item = m_curr;
				m_curr = NULL;

				// Deselect and hide the old item
				if (old_item) {
					if (old_item->getSubmenu() && old_item->getSubmenu()->isVisible())
						old_item->getSubmenu()->hideAll();
					redraw(old_item);
				}
			}
		}
	}

	BaseMenuItem *item = findMenuItem(e->x, e->y);
	if (item) {
		if (item != m_curr) {
			BaseMenuItem *old_item = m_curr;
			m_curr = item;

			// Deselect and hide the old item
			if (old_item) {
				if (old_item->getSubmenu() && old_item->getSubmenu()->isVisible())
					old_item->getSubmenu()->hideAll();
				redraw(old_item);
			}
			redraw(m_curr);

			if (m_curr->getSubmenu()) {
				showSub(m_curr->getSubmenu());
			}
		}
	}
}

bool
BaseMenu::getMousePosition(int *x, int *y)
{
	Window dummy_w1, dummy_w2;
	int t1, t2;
	unsigned int t3;

	return XQueryPointer(dpy, root, &dummy_w1, &dummy_w2, x, y, &t1, &t2, &t3);
}


//! @fn    void makeMenuInsideScreen(void)
//! @brief Makes sure the menu is insede the screen/(current head).
//! Makes sure the menu is insede the screen/(current head).
//! NOTE: it doesn't move the window, just updates it's coordinates
void
BaseMenu::makeMenuInsideScreen(void)
{
	unsigned int width = m_width + (theme->getMenuBorderWidth() * 2);
	unsigned int height = m_height + (theme->getMenuBorderWidth() * 2);

	bool right_edge, bottom_edge;

#ifdef XINERAMA
	int mouse_x = 0, mouse_y = 0;
	getMousePosition(&mouse_x, &mouse_y);

	ScreenInfo::HeadInfo head;
	unsigned int head_nr = 0;
	if (scr->hasXinerama()) {
		head_nr = scr->getHead(mouse_x, mouse_y);
	}
	scr->getHeadInfo(head_nr, head);

	right_edge = ((m_x + width) >= (head.x + head.width)) ? true : false;
	bottom_edge = ((m_y + height) >= (head.y + head.height)) ? true : false;

	if (right_edge) {
		if (m_parent) {
			m_x = m_parent->getX() - width;
		} else {
			m_x = head.x + head.width - width;
		}
	}

	if (bottom_edge) {
		m_y = head.y + head.height - height;
	}

#else // !XINERAMA
	right_edge = ((m_x + width) >= scr->getWidth()) ? true : false;
	bottom_edge = ((m_y + height) >= scr->getHeight()) ? true : false;

	if (right_edge) {
		if (m_parent) {
			m_x = m_parent->getX() - width;
		} else {
			m_x = scr->getWidth() - width;
		}
	}

	if (bottom_edge) {
		m_y = scr->getHeight() - height;
	}
#endif // XINERAMA
}

#endif // MENUS
