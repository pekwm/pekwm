//
// PMenu.cc for pekwm
// Copyright (C) 2022-2024 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2004-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "ActionHandler.hh"
#include "Config.hh"
#include "Iter.hh"
#include "Workspaces.hh"
#include "X11.hh"

#include "tk/PFont.hh"
#include "tk/PTexture.hh"
#include "tk/PTexturePlain.hh"
#include "tk/PWinObj.hh"
#include "tk/TextureHandler.hh"
#include "tk/Theme.hh"
#include "tk/X11Util.hh"

#include <algorithm>
#include <cstdlib>

PMenu::Item::Item(const std::string &name, PWinObj *wo_ref, PTexture *icon)
	: PWinObjReference(wo_ref),
	  _name(name),
	  _type(MENU_ITEM_NORMAL),
	  _icon(icon),
	  _creator(0)
{
	if (_icon) {
		pekwm::textureHandler()->referenceTexture(_icon);
	}
}

PMenu::Item::~Item(void)
{
	if (_icon) {
		pekwm::textureHandler()->returnTexture(&_icon);
	}
}

std::map<Window,PMenu*> PMenu::_menu_map = std::map<Window,PMenu*>();

//! @brief Constructor for PMenu class
PMenu::PMenu(const std::string &title,
	     const std::string &name, const std::string& decor_name,
	     bool init)
	: PDecor(None, true, init, decor_name),
	  _name(name),
	  _menu_parent(0), _class_hint("pekwm", "Menu", "", "", ""),
	  _item_curr(0),
	  _menu_wo(0),
	  _menu_width(0),
	  _item_height(0),
	  _item_width_max(0),
	  _item_pad_horz(0),
	  _icon_width(0),
	  _icon_height(0),
	  _separator_height(0),
	  _rows(0),
	  _cols(0),
	  _scroll(false),
	  _has_submenu(0)
{
	// PWinObj attributes
	_type = PWinObj::WO_MENU;
	setLayer(LAYER_MENU);
	_hidden = true;

	// create menu content child
	_menu_wo = new PWinObj(false);

	long event_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
		ExposureMask|FocusChangeMask|KeyPressMask|KeyReleaseMask|
		PointerMotionMask;
	Window window = X11::createWmWindow(_window, 0, 0, 1, 1,
					    InputOutput, event_mask);
	_menu_wo->setWindow(window);

	titleAdd(&_title);
	titleSetActive(0);
	setTitle(title);

	addChild(_menu_wo);
	addChildWindow(_menu_wo->getWindow());
	activateChild(_menu_wo);
	_menu_wo->mapWindow();

	Workspaces::insert(this);
	_menu_map[_window] = this; // add to menu map
	woListAdd(this);
	_wo_map[_window] = this;
	if (pekwm::config()) {
		setOpacity(pekwm::config()->getMenuFocusOpacity(),
			   pekwm::config()->getMenuUnfocusOpacity());
	}
}

//! @brief Destructor for PMenu class
PMenu::~PMenu(void)
{
	_wo_map.erase(_window);
	woListRemove(this);
	_menu_map.erase(_window); // remove from menu map
	Workspaces::remove(this);

	// Free resources
	if (_menu_wo) {
		_children.erase(std::remove(_children.begin(), _children.end(),
					    _menu_wo),
				_children.end());
		removeChildWindow(_menu_wo->getWindow());
		X11::destroyWindow(_menu_wo->getWindow());
		delete _menu_wo;
	}

	item_it it = _items.begin();
	for (; it != _items.end(); ++it) {
		delete *it;
	}
}

// START - PWinObj interface.

//! @brief Unmapping, deselecting current item and unsticking.
void
PMenu::unmapWindow(void)
{
	_item_curr = _items.size();
	_sticky = false;

	PDecor::unmapWindow();
}

void
PMenu::setFocused(bool focused)
{
	if (_focused != focused) {
		PDecor::setFocused(focused);

		X11::setWindowBackgroundPixmap(
				_menu_wo->getWindow(),
				_focused
				   ? _menu_bg_fo.getDrawable()
				   : _menu_bg_un.getDrawable());
		X11::clearWindow(_menu_wo->getWindow());
		if (_item_curr < _items.size()) {
			item_it item(_items.begin() + _item_curr);
			// Force selectItem(item) to redraw
			_item_curr = _items.size();
			selectItem(item);
		}
	}
}

/**
 * Set focusable, includes the _menu_wo as well as the decor.
 */
void
PMenu::setFocusable(bool focusable)
{
	PDecor::setFocusable(focusable);
	_menu_wo->setFocusable(focusable);
}

ActionEvent*
PMenu::handleButtonPress(XButtonEvent *ev)
{
	if (*_menu_wo == ev->window) {
		handleItemEvent(MOUSE_EVENT_PRESS, ev->x, ev->y);

		// update pointer position
		_pointer_x = ev->x_root;
		_pointer_y = ev->y_root;

		Config* cfg = pekwm::config();
		std::vector<ActionEvent> *malm =
			cfg->getMouseActionList(MOUSE_ACTION_LIST_MENU);
		return ActionHandler::findMouseAction(ev->button, ev->state,
						      MOUSE_EVENT_PRESS, malm);
	} else {
		return PDecor::handleButtonPress(ev);
	}
}

/**
 * Handle button release.
 */
ActionEvent*
PMenu::handleButtonRelease(XButtonEvent *ev)
{
	if (_window == ev->subwindow) {
		ev->window = _menu_wo->getWindow();
		ev->x -= _gm.x;
		ev->y -= _gm.y + titleHeight(this);
	}

	if (*_menu_wo == ev->window) {
		MouseEventType mb = MOUSE_EVENT_RELEASE;

		Config *cfg = pekwm::config();
		// first we check if it's a double click
		if (X11::isDoubleClick(ev->window, ev->button - 1, ev->time,
				       cfg->getDoubleClickTime())) {
			X11::setLastClickID(ev->window);
			X11::setLastClickTime(ev->button - 1, 0);

			mb = MOUSE_EVENT_DOUBLE;

		} else {
			X11::setLastClickID(ev->window);
			X11::setLastClickTime(ev->button - 1, ev->time);
		}

		handleItemEvent(mb, ev->x, ev->y);

		std::vector<ActionEvent> *malm =
			cfg->getMouseActionList(MOUSE_ACTION_LIST_MENU);
		return ActionHandler::findMouseAction(ev->button, ev->state,
						      mb, malm);
	} else {
		return PDecor::handleButtonRelease(ev);
	}
}

/**
 * Handle Expose event, just redraw the currently selected menu item.
 */
ActionEvent*
PMenu::handleExposeEvent(XExposeEvent*)
{
	renderSelectedItem();
	return nullptr;
}

/**
 * Handle motion event, select buttons and execute actions.
 *
 * @param ev Handle motion event.
 */
ActionEvent*
PMenu::handleMotionEvent(XMotionEvent *ev)
{
	if (_window == ev->subwindow) {
		ev->window = _menu_wo->getWindow();
		ev->x -= _gm.x;
		ev->y -= _gm.y + titleHeight(this);
	}

	if (*_menu_wo == ev->window) {
		uint button = X11::getButtonFromState(ev->state);
		handleItemEvent(button
				? MOUSE_EVENT_MOTION_PRESSED
				: MOUSE_EVENT_MOTION,
				ev->x, ev->y);

		ActionEvent *ae;
		X11::stripButtonModifiers(&ev->state);
		Config* cfg = pekwm::config();
		std::vector<ActionEvent> *malm =
			cfg->getMouseActionList(MOUSE_ACTION_LIST_MENU);
		ae = ActionHandler::findMouseAction(button, ev->state,
						    MOUSE_EVENT_MOTION, malm);

		// check motion threshold
		if (ae && (ae->threshold > 0)) {
			if (! ActionHandler::checkAEThreshold(
						ev->x_root, ev->y_root,
						_pointer_x, _pointer_y,
						ae->threshold)) {
				ae = 0;
			}
		}
		return ae;
	} else {
		return PDecor::handleMotionEvent(ev);
	}
}

ActionEvent*
PMenu::handleEnterEvent(XCrossingEvent *ev)
{
	if (*_menu_wo == ev->window) {
		Config* cfg = pekwm::config();
		std::vector<ActionEvent> *malm =
			cfg->getMouseActionList(MOUSE_ACTION_LIST_MENU);
		return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
						      MOUSE_EVENT_ENTER, malm);
	} else {
		return PDecor::handleEnterEvent(ev);
	}
}

ActionEvent*
PMenu::handleLeaveEvent(XCrossingEvent *ev)
{
	if (*_menu_wo == ev->window) {
		Config* cfg = pekwm::config();
		std::vector<ActionEvent> *malm =
			cfg->getMouseActionList(MOUSE_ACTION_LIST_MENU);
		return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
						      MOUSE_EVENT_LEAVE, malm);
	} else {
		return PDecor::handleLeaveEvent(ev);
	}
}

// END - PWinObj interface.

// START - PDecor interface.

void
PMenu::loadTheme(void)
{
	buildMenu();
}

// END - PDecor interface.

/**
 * Handle event on menu item at position, ignores event if no item
 * exists at the position.
 */
void
PMenu::handleItemEvent(MouseEventType type, int x, int y)
{
	PMenu::Item *item = findItem(x, y);
	if (! item) {
		return;
	}

	// Unmap submenu if we enter them on the same event as selecting.
	if ((_item_curr >= _items.size() || item != _items[_item_curr])
	    && pekwm::config()->isMenuSelectOn(type)) {
		select(item, pekwm::config()->isMenuEnterOn(type));
	}

	if (pekwm::config()->isMenuEnterOn(type)) {
		PWinObj* wo_ref = item->getWORef();
		if (wo_ref
		    && (wo_ref->getType() == PWinObj::WO_MENU)) {
			PMenu* ref_menu = static_cast<PMenu*>(wo_ref);
			// Special case for motion to avoid flickering
			if (type != MOUSE_EVENT_MOTION
			    && type != MOUSE_EVENT_MOTION_PRESSED
			    && wo_ref->isMapped()) {
				ref_menu->unmapSubmenus();
				ref_menu->unmapWindow();

			} else if (! wo_ref->isMapped()) {
				// unmap previous opened submenu if any
				unmapSubmenus();
				mapSubmenu(ref_menu);
			}
		}
	}

	// Submenus do not have any actions -> no exec and close
	if (item->getAE().action_list.size()
	    && pekwm::config()->isMenuExecOn(type)) {
		exec(item);
	}
}

//! @brief Sets the position of the items and determine size of the menu
void
PMenu::buildMenu(void)
{
	// calculate geometry, if to enable scrolling etc
	buildMenuCalculate();

	// not necessary to do this if we don't have any visible items
	if (_size > 0) {
		// place menu items
		buildMenuPlace();

		// render items on the menu
		buildMenuRender();
	}
}

/**
 * Calculates how much space and how many rows/cols will be needed
 */
void
PMenu::buildMenuCalculate(void)
{
	// Get how many visible objects we have
	unsigned int sep = 0;
	item_it it(_items.begin());
	for (_size = 0; it != _items.end(); ++it) {
		if ((*it)->getType() == PMenu::Item::MENU_ITEM_NORMAL) {
			++_size;
		} else if ((*it)->getType()
			   == PMenu::Item::MENU_ITEM_SEPARATOR) {
			++sep;
		}
	}

	buildMenuCalculateMaxWidth(_item_width_max,
				   _icon_width, _icon_height);
	uint title_width = titleWidth(this, _title.getReal())
		- titleLeftOffset(this) - titleRightOffset(this);
	if (title_width > _item_width_max) {
		_item_width_max = title_width;
	}

	// Continue add padding etc.
	Theme::PMenuData *md = pekwm::theme()->getMenuData();
	_item_pad_horz = md->getPad(PAD_LEFT) + md->getPad(PAD_RIGHT);
	if (pekwm::config()->isDisplayMenuIcons()) {
		_item_pad_horz += _icon_width;
	}

	// If we have any submenus, increase the maximum width with arrow width
	// + right pad as we are going to pad the arrow from the text too.
	if (_has_submenu) {
		_item_pad_horz += md->getPad(PAD_RIGHT)
			+ md->getTextureArrow(OBJECT_STATE_FOCUSED)->getWidth();
	}

	_item_width_max += _item_pad_horz;

	// Calculate item height
	_item_height =
		std::max(md->getFont(OBJECT_STATE_FOCUSED)->getHeight(),
			 _icon_height)
		+ md->getPad(PAD_UP)
		+ md->getPad(PAD_DOWN);
	_separator_height =
		md->getTextureSeparator(OBJECT_STATE_FOCUSED)->getHeight();

	uint height = (_item_height * _size) + (_separator_height * sep);
	if (_size) {
		_size += sep;
	}

	uint width;
	buildMenuCalculateColumns(width, height);

	// Check if we need to enable scrolling
	_scroll = (width > X11::getWidth());

	resizeChild(std::max(static_cast<uint>(1), width),
		    std::max(static_cast<uint>(1), height));
}

/**
 * Get maximum item width and icon size.
 */
void
PMenu::buildMenuCalculateMaxWidth(uint &max_width,
				  uint &icon_width, uint &icon_height)
{
	// Calculate max item width, to be used if/when splitting a menu
	// up in rows because of limited vertical space.
	max_width = 1;
	icon_width = 0;
	icon_height = 0;

	Theme::PMenuData *md = pekwm::theme()->getMenuData();
	PFont *font = md->getFont(OBJECT_STATE_FOCUSED);
	item_it it = _items.begin();
	for (; it != _items.end(); ++it) {
		// Only include standard items
		if ((*it)->getType() != PMenu::Item::MENU_ITEM_NORMAL) {
			continue;
		}

		// Get icon height if any
		if ((*it)->getIcon()) {
			if ((*it)->getIcon()->getWidth() > icon_width) {
				icon_width = (*it)->getIcon()->getWidth();
			}
			if ((*it)->getIcon()->getHeight() > icon_height) {
				icon_height = (*it)->getIcon()->getHeight();
			}
		}

		uint width = font->getWidth((*it)->getName().c_str());
		if (width > max_width) {
			max_width = width;
		}
	}


	// Make sure icon width and height are not larger than configured.
	if (icon_width) {
		Config *cfg = pekwm::config();
		uint lmin, lmax;

		lmin = cfg->getMenuIconLimit(icon_width, WIDTH_MIN, _name),
		lmax = cfg->getMenuIconLimit(icon_width, WIDTH_MAX, _name);
		icon_width = Util::between<uint>(icon_width, lmin, lmax);

		lmin = cfg->getMenuIconLimit(icon_height, HEIGHT_MIN, _name),
		lmax = cfg->getMenuIconLimit(icon_height, HEIGHT_MAX, _name);
		icon_height = Util::between<uint>(icon_height, lmin, lmax);
	}
}

/**
 * Calculate number of columns, this does not apply to static width
 * menus.
 */
void
PMenu::buildMenuCalculateColumns(unsigned int &width, unsigned int &height)
{
	// Check if the menu fits or is static width
	if (_menu_width
	    || (height + titleHeight(this)) <= X11::getHeight()) {
		_cols = 1;
		width = _menu_width ? _menu_width : _item_width_max;
		_rows = _size;
		return;
	}

	_cols = height / (X11::getHeight() - titleHeight(this));
	if ((height % (X11::getHeight() - titleHeight(this))) != 0) {
		++_cols;
	}
	_rows = _size / _cols;
	if ((_size % _cols) != 0) {
		++_rows;
	}

	width = _cols * _item_width_max;
	// need to calculate max height, the one with most separators if any
	if (_cols > 1) {
		uint i, j;
		height = 0;

		item_it it(_items.begin());
		for (i = 0, it = _items.begin(); i < _cols; ++i) {
			uint row_height = 0;
			for (j = 0;
			     j < _rows && it != _items.end();
			     ++it, ++j) {
				switch ((*it)->getType()) {
				case PMenu::Item::MENU_ITEM_NORMAL:
					row_height += _item_height;
					break;
				case PMenu::Item::MENU_ITEM_SEPARATOR:
					row_height += _separator_height;
					break;
				case PMenu::Item::MENU_ITEM_HIDDEN:
				default:
					break;
				}
			}

			if (row_height > height) {
				height = row_height;
			}
		}
	}
}

//! @brief Places the items in the menu
void
PMenu::buildMenuPlace(void)
{
	uint x;
	std::vector<PMenu::Item*>::const_iterator it;

	x = 0;
	it = _items.begin();
	// cols
	for (uint i = 0; i < _cols; ++i) {
		uint y = 0;
		// rows
		for (uint j = 0; j < _rows && it != _items.end(); ++it) {
			if ((*it)->getType()
			    == PMenu::Item::MENU_ITEM_HIDDEN) {
				continue;
			}
			(*it)->setX(x);
			(*it)->setY(y);
			(*it)->setWidth(_item_width_max);
			if ((*it)->getType()
			    == PMenu::Item::MENU_ITEM_NORMAL) {
				y += _item_height;
				++j; // only count real menu items
				(*it)->setHeight(_item_height);
			} else if ((*it)->getType()
				   == PMenu::Item::MENU_ITEM_SEPARATOR) {
				y += _separator_height;
				(*it)->setHeight(_separator_height);
			}
		}
		x += _item_width_max;
	}
}

//! @brief Renders focused, unfocused and selected pixmaps for menu
void
PMenu::buildMenuRender(void)
{
	_menu_bg_fo.resize(getChildWidth(), getChildHeight());
	buildMenuRenderState(&_menu_bg_fo, OBJECT_STATE_FOCUSED);
	_menu_bg_un.resize(getChildWidth(), getChildHeight());
	buildMenuRenderState(&_menu_bg_un, OBJECT_STATE_UNFOCUSED);
	_menu_bg_se.resize(getChildWidth(), getChildHeight());
	buildMenuRenderState(&_menu_bg_se, OBJECT_STATE_SELECTED);

	X11::setWindowBackgroundPixmap(_menu_wo->getWindow(),
				       _focused ? _menu_bg_fo.getDrawable()
						: _menu_bg_un.getDrawable());
	X11::clearWindow(_menu_wo->getWindow());
}

//! @brief Renders menu content on pix, with state state
void
PMenu::buildMenuRenderState(PSurface *surf, ObjectState state)
{
	Theme::PMenuData *md = pekwm::theme()->getMenuData();
	PTexture *tex = md->getTextureMenu(state);
	tex->render(surf, 0, 0, getChildWidth(), getChildHeight());
	PFont *font = md->getFont(state);
	font->setColor(md->getColor(state));

	item_it it = _items.begin();
	for (; it != _items.end(); ++it) {
		if ((*it)->getType() != PMenu::Item::MENU_ITEM_HIDDEN) {
			buildMenuRenderItem(surf, state, *it);
		}
	}
}

//! @brief Renders item on pix, with state state
void
PMenu::buildMenuRenderItem(PSurface *surf, ObjectState state, PMenu::Item *item)
{
	if (item->getType() == PMenu::Item::MENU_ITEM_NORMAL) {
		buildMenuRenderItemNormal(surf, state, item);
	} else if (item->getType() == PMenu::Item::MENU_ITEM_SEPARATOR
		   && state < OBJECT_STATE_SELECTED) {
		buildMenuRenderItemSeparator(surf, state, item);
	}
}

void
PMenu::buildMenuRenderItemNormal(PSurface *surf, ObjectState state,
				 PMenu::Item* item)
{
	Theme::PMenuData *md = pekwm::theme()->getMenuData();
	Config *cfg = pekwm::config();

	PTexture *tex = md->getTextureItem(state);
	tex->render(surf,
		    item->getX(), item->getY(),
		    item->getWidth(), item->getHeight());

	uint start_x, start_y;
	// If entry has an icon, draw it
	if (item->getIcon() && cfg->isDisplayMenuIcons()) {
		uint lmin, lmax;
		lmin = cfg->getMenuIconLimit(_icon_width, WIDTH_MIN, _name);
		lmax = cfg->getMenuIconLimit(_icon_width, WIDTH_MAX, _name);
		uint icon_width =
			Util::between<uint>(item->getIcon()->getWidth(),
					    lmin, lmax);

		lmin = cfg->getMenuIconLimit(_icon_height, HEIGHT_MIN, _name);
		lmax = cfg->getMenuIconLimit(_icon_height, HEIGHT_MAX, _name);
		uint icon_height =
			Util::between<uint>(item->getIcon()->getHeight(),
					    lmin, lmax);

		start_x = item->getX() + md->getPad(PAD_LEFT)
			+ (_icon_width - icon_width) / 2;
		start_y = item->getY()
			+ (_item_height - icon_height) / 2;
		item->getIcon()->render(surf, start_x, start_y,
					icon_width, icon_height);
	}

	// If entry has a submenu, lets draw our submenu "arrow"
	if (item->getWORef()
	    && (item->getWORef()->getType() == PWinObj::WO_MENU)) {
		tex = md->getTextureArrow(state);
		uint arrow_width = tex->getWidth();
		uint arrow_height = tex->getHeight();
		uint arrow_y =
			static_cast<uint>((_item_height / 2)
					  - (arrow_height / 2));

		start_x = item->getX() + item->getWidth()
			- arrow_width - md->getPad(PAD_RIGHT);
		start_y = item->getY() + arrow_y;
		tex->render(surf, start_x, start_y,
			    arrow_width, arrow_height);
	}

	PFont *font = md->getFont(state);
	start_x = item->getX() + md->getPad(PAD_LEFT);
	// Add icon width to starting x position if frame icons are
	// enabled.
	if (cfg->isDisplayMenuIcons()) {
		start_x += _icon_width;
	}

	start_y = item->getY() + md->getPad(PAD_UP)
		+ (_item_height - font->getHeight()
		   - md->getPad(PAD_UP) - md->getPad(PAD_DOWN)) / 2;

	// Render item text.
	font->draw(surf, start_x, start_y, item->getName().c_str(),
		   0, item->getWidth() - _item_pad_horz);
}

void
PMenu::buildMenuRenderItemSeparator(PSurface *surf, ObjectState state,
				    PMenu::Item* item)
{
	Theme::PMenuData *md = pekwm::theme()->getMenuData();
	PTexture *tex = md->getTextureSeparator(state);
	tex->render(surf,
		    item->getX(), item->getY(),
		    item->getWidth(), item->getHeight());
}

#define COPY_ITEM_AREA(ITEM, PIX)		  \
	X11::copyArea(PIX, _menu_wo->getWindow(), \
		      (ITEM)->getX(), (ITEM)->getY(), \
		      (ITEM)->getWidth(), (ITEM)->getHeight(), \
		      (ITEM)->getX(), (ITEM)->getY());

//! @brief Renders item as selected
//! @param item Item to select
//! @param unmap_submenu Defaults to true
void
PMenu::selectItem(std::vector<PMenu::Item*>::const_iterator item,
		  bool unmap_submenu)
{
	if (_item_curr < _items.size() && _items[_item_curr] == *item) {
		return;
	}

	deselectItem(unmap_submenu);
	_item_curr = item-_items.begin();

	renderSelectedItem();
}


void
PMenu::renderSelectedItem(void)
{
	if (! _mapped || _item_curr >= _items.size()) {
		return;
	}
	PMenu::Item *item = _items[_item_curr];
	if (item->getType() != PMenu::Item::MENU_ITEM_HIDDEN) {
		COPY_ITEM_AREA(item, _menu_bg_se.getDrawable());
	}
}

//! @brief Deselects selected item
//! @param unmap_submenu Defaults to true
void
PMenu::deselectItem(bool unmap_submenu)
{
	// deselect previous item
	if (_item_curr >= _items.size()) {
		return;
	}

	PMenu::Item* item = _items[_item_curr];
	if (item->getType() == PMenu::Item::MENU_ITEM_HIDDEN) {
		return;
	}

	if (_mapped) {
		COPY_ITEM_AREA(_items[_item_curr],
			       (_focused ? _menu_bg_fo.getDrawable()
					 : _menu_bg_un.getDrawable()));
	}

	PWinObj* wo_ref = item->getWORef();
	if (unmap_submenu
	    && wo_ref
	    && (wo_ref->getType() == PWinObj::WO_MENU)) {
		PMenu* ref_menu = static_cast<PMenu*>(wo_ref);
		ref_menu->unmapSubmenus();
		wo_ref->unmapWindow();
	}
}

#undef COPY_ITEM_AREA

//! @brief Selects next item ( wraps ). First item if none is selected.
void
PMenu::selectNextItem(void)
{
	if (_size == 0) {
		return;
	}

	item_it item(_items.begin() + (_item_curr<_items.size()?_item_curr:0));

	// no item selected, select the first item
	if (item == _items.end()) {
		item = _items.begin();
	} else {
		// select next item, wrap if needed
		++item;
		if (item == _items.end()) {
			item = _items.begin();
		}
	}

	// skip to next if separator/hidden
	if ((*item)->getType() != PMenu::Item::MENU_ITEM_NORMAL) {
		deselectItem(); // otherwise, last selected won't get deselcted
		_item_curr = item-_items.begin();
		selectNextItem();
	} else {
		selectItem(item);
	}
}

//! @brief Selects previous item ( wraps ). Last item if none is selected.
void
PMenu::selectPrevItem(void)
{
	if (_size == 0) {
		return;
	}

	item_it item(_items.begin() + (_item_curr<_items.size()?_item_curr:0));

	// no item selected, select the last item OR
	// we're at the beginning and need to wrap to the end
	if (item == _items.end() || item == _items.begin()) {
		item = _items.end();
	}
	--item;

	// skip to prev if separator/hidden
	if ((*item)->getType() != PMenu::Item::MENU_ITEM_NORMAL) {
		deselectItem(); // otherwise, last selected won't get deselcted
		_item_curr = item-_items.begin();
		selectPrevItem();
	} else {
		selectItem(item);
	}
}

//! @brief Sets title of the menu/decor
void
PMenu::setTitle(const std::string &title)
{
	_title.setReal(title);

	// Apply title rules to allow title rewriting
	applyTitleRules(title);
}

/**
 * Applies title rules to menu.
 */
void
PMenu::applyTitleRules(const std::string &title)
{
	_class_hint.title = title;
	AutoProperties *auto_properties = pekwm::autoProperties();
	TitleProperty *data = auto_properties
		? auto_properties->findTitleProperty(_class_hint) : nullptr;
	if (data) {
		std::string new_title(title);
		if (data->getTitleRule().ed_s(new_title)) {
			_title.setCustom(new_title);
		}
	}
}

/**
 * Insert menu at the end (without rebuilding)
 */
void
PMenu::insert(PMenu::Item *item)
{
	insert(_items.end(), item);
}

/**
 * Inserts item into the menu at the given position (without rebuilding)
 */
void
PMenu::insert(std::vector<PMenu::Item*>::iterator at, PMenu::Item *item)
{
	checkItemWORef(item);

	// Check if we have a submenu item
	if (item->getWORef()
	    && (item->getWORef()->getType() == PWinObj::WO_MENU)) {
		_has_submenu++;
	}

	_items.insert(at, item);
}

//! @brief Creates and inserts Item
//! @param name Name of objet to create and insert
//! @param wo_ref PWinObj to refer to, defaults to 0
void
PMenu::insert(const std::string &name, PWinObj *wo_ref, PTexture *icon)
{
	insert(new PMenu::Item(name, wo_ref, icon));
}

//! @brief Creates and inserts Item
//! @param name Name of object to create and insert
//! @param ae ActionEvent for the object
//! @param wo_ref PWinObj to refer to, defaults to 0
void
PMenu::insert(const std::string &name, const ActionEvent &ae,
	      PWinObj *wo_ref, PTexture *icon)
{
	PMenu::Item *item = new PMenu::Item(name, wo_ref, icon);
	item->setAE(ae);
	insert(item);
}

//! @brief Removes an item from the menu, without rebuilding.
void
PMenu::remove(PMenu::Item *item)
{
	if (item == nullptr) {
		P_ERR("trying to remove null item");
		return;
	}

	if (_item_curr < _items.size() && item == _items[_item_curr]) {
		_item_curr = _items.size();
	}

	if (item->getWORef()
	    && (item->getWORef()->getType() == PWinObj::WO_MENU)) {
		_has_submenu--;
	}

	_items.erase(std::remove(_items.begin(), _items.end(), item),
		     _items.end());
	delete item;
}

//! @brief Removes all items from the menu, without rebuilding.
void
PMenu::removeAll(void)
{
	item_it it = _items.begin();
	for (; it != _items.end(); ++it) {
		delete *it;
	}
	_items.clear();
	_item_curr = 0;
	_has_submenu = 0;
}

//! @brief Places the menu under the mouse and maps it.
void
PMenu::mapUnderMouse(void)
{
	int x, y;

	X11::getMousePosition(x, y);

	// this might seem a bit silly but the menu won't get updated before
	// it has been mapped (if dynamic) so we're doing it twice to reduce
	// the "flickering" risk but it's not 100% so it's done twice.
	makeInsideScreen(x, y);
	mapWindowRaised();
	makeInsideScreen(x, y);
}

//! @brief Maps menu relative to the this menu
//! @param menu Submenu to map
//! @param focus Give input focus and select first item. Defaults to false.
void
PMenu::mapSubmenu(PMenu *menu, bool focus)
{
	int x, y;

	x = getRX();
	if (_item_curr < _items.size()) {
		y = _gm.y + _items[_item_curr]->getY();
	} else {
		y = _gm.y;
	}

	// this might seem a bit silly but the menu won't get updated before
	// it has been mapped (if dynamic) so we're doing it twice to reduce
	// the "flickering" risk but it's not 100% so it's done twice.
	menu->makeInsideScreen(x, y);
	menu->mapWindowRaised();
	menu->makeInsideScreen(x, y);

	if (focus) {
		menu->giveInputFocus();
		menu->selectItemNum(0);
	}
}

//! @brief Unmaps all ( recursive ) submenus open under this menu
void
PMenu::unmapSubmenus(void)
{
	item_it it = _items.begin();
	for (; it != _items.end(); ++it) {
		if ((*it)->getWORef()
		    && (*it)->getWORef()->getType() == PWinObj::WO_MENU) {
			// Sub-menus will be deleted when unmapping this, so no
			// need to continue.
			static_cast<PMenu*>((*it)->getWORef())->unmapSubmenus();
			(*it)->getWORef()->unmapWindow();
		}
	}
}

//! @brief Unmaps all menus belonging to this menu
void
PMenu::unmapAll(void)
{
	if (_menu_parent) {
		_menu_parent->unmapAll();
	} else {
		unmapSubmenus();
		unmapWindow();
	}
}

//! @brief Gives input focus to parent and unmaps submenus
void
PMenu::gotoParentMenu(void)
{
	if (! _menu_parent) {
		return;
	}

	_menu_parent->unmapSubmenus();
	_menu_parent->giveInputFocus();
}

//! @brief Selects item, if 0/not in list current item is deselected
//! @param item Item to select
//! @param unmap_submenu Defaults to true
void
PMenu::select(PMenu::Item *item, bool unmap_submenu)
{
	selectItem(std::find(_items.begin(), _items.end(), item),
		   unmap_submenu);
}

static bool
_filter_reg(PMenu::Item *item)
{
	return item->getType() == PMenu::Item::MENU_ITEM_NORMAL;
}

/**
 * @brief Selects item number num in menu skipping separators and other
 * special items.
 */
bool
PMenu::selectItemNum(uint num)
{
	FilterIt<item_it, PMenu::Item*> it(_items.begin(), _items.end(),
					   _filter_reg);
	for (uint i = 0; it.isValid() && i < num; i++) {
		it.next();
	}

	if (it.isValid()) {
		selectItem(*it);
		return true;
	}
	return false;
}

//! @brief Selects item relative to the selected
void
PMenu::selectItemRel(int off)
{
	if (off == 0) {
		P_WARN("trying to select non existing relative (current) "
		       "item number");
		return;
	}

	// if no selected item, use first
	item_it it(_items.begin() + (_item_curr < _items.size()?_item_curr:0));

	int dir = (off > 0) ? 1 : -1;
	off = abs(off);

	for (int i = 0; i < off; ++i) {
		if (dir == 1) { // forward
			if (++it == _items.end()) {
				it = _items.begin();
			}

		} else { // backward
			if (it == _items.begin()) {
				it = _items.end();
			}
			--it;
		}
	}

	selectItem(it);
}

//! @brief Executes items action, sending it to the parent menu if availible
void
PMenu::exec(PMenu::Item *item)
{
	if (_menu_parent) {
		_menu_parent->exec(item);
	} else {
		handleItemExec(item);
		if (! _sticky) {
			unmapAll();
		}
	}
}

//! @brief Sets up children _menu_parent field, if item's _wo_ref is a menu
void
PMenu::checkItemWORef(PMenu::Item *item)
{
	if (item->getWORef() &&
	    (item->getWORef()->getType() == PWinObj::WO_MENU)) {
		PMenu *child = static_cast<PMenu*>(item->getWORef());
		child->_menu_parent = this;
	}
}

//! @brief Searches for item at x, y
PMenu::Item*
PMenu::findItem(int x, int y)
{
	item_it it = _items.begin();
	for (; it != _items.end(); ++it) {
		if (((*it)->getType() == PMenu::Item::MENU_ITEM_NORMAL) &&
		    (x >= (*it)->getX())
		    && (x <= signed((*it)->getX() + (*it)->getWidth())) &&
		    (y >= (*it)->getY())
		    && (y <= signed((*it)->getY() + (*it)->getHeight()))) {
			return *it;
		}
	}
	return nullptr;
}

//! @brief Moves the menu relative to it's parent to make it fit on screen
//! @param x Use x instead of _gm.x ( optional )
//! @param y Use y instead of _gm.y ( optional )
void
PMenu::makeInsideScreen(int x, int y)
{
	x = (x == -1) ? _gm.x : x;
	y = (y == -1) ? _gm.y : y;

	Geometry head;
	X11::getHeadInfo(x, y, head);

	// we map on submenus on the right side so this only happens on the
	// top-level menu
	if (x < head.x) {
		x = head.x;
	} else if ((x + _gm.width) > (head.x + head.width)) {
		if (_menu_parent) {
			// not using getX(), refers to child
			x = _menu_parent->_gm.x - _gm.width;
		} else {
			x = head.x + head.width - _gm.width;
		}
	}

	if (y < head.y) {
		y = head.y;
	} else if ((y + _gm.height) > (head.y + head.height)) {
		y = head.y + head.height - _gm.height;
	}

	move(x, y);
}
