//
// PMenu.cc for pekwm
// Copyright © 2004-2007 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PFont.hh"
#include "PMenu.hh"
#include "PTexture.hh"
#include "PTexturePlain.hh"
#include "PScreen.hh"
#include "ActionHandler.hh"
#include "Config.hh"
#include "ScreenResources.hh"
#include "Theme.hh"
#include "PixmapHandler.hh"
#include "Workspaces.hh"

#include <algorithm>
#include <cstdlib>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::list;
using std::map;
using std::string;
using std::find;

map<Window,PMenu*> PMenu::_menu_map = map<Window,PMenu*>();

//! @brief Constructor for PMenu class
PMenu::PMenu(Display *dpy, Theme *theme, const std::wstring &title,
             const std::string &name, const std::string decor_name)
    : PDecor(dpy, theme, decor_name),
      _name(name),
      _menu_wo(NULL), _menu_parent(NULL),
      _menu_bg_fo(None), _menu_bg_un(None), _menu_bg_se(None),
      _item_height(0), _item_width_max(0), _item_width_max_avail(0),
      _icon_width(0), _icon_height(0),
      _separator_height(0),
      _rows(0), _cols(0), _scroll(false), _has_submenu(false)
{
    // initiate items
    _item_curr = _item_list.end();

    // PWinObj attributes
    _type = PWinObj::WO_MENU;
    _layer = LAYER_MENU;
    _hidden = true; // don't care about it when changing worskpace etc

    titleAdd(&_title);
    titleSetActive(0);
    setTitle(title);

    // create menu content child
    _menu_wo = new PWinObj(_dpy);
    XSetWindowAttributes attr;
    attr.override_redirect = True;
    attr.event_mask = ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|
                      FocusChangeMask|KeyPressMask|KeyReleaseMask|PointerMotionMask;
    _menu_wo->setWindow(XCreateWindow(_dpy, _window,
                                      0, 0, 1, 1, 0,
                                      CopyFromParent, InputOutput, CopyFromParent,
                                      CWOverrideRedirect|CWEventMask, &attr));
    addChild(_menu_wo);
    addChildWindow(_menu_wo->getWindow());
    activateChild(_menu_wo);
    _menu_wo->mapWindow();

    Workspaces::instance()->insert(this);
    _menu_map[_window] = this; // add to menu map
    woListAdd(this);
    _wo_map[_window] = this;
}

//! @brief Destructor for PMenu class
PMenu::~PMenu(void)
{
    _wo_map.erase(_window);
    woListRemove(this);
    _menu_map.erase(_window); // remove from menu map
    Workspaces::instance()->remove(this);

    // Free resources
    if (_menu_wo != NULL) {
        _child_list.remove(_menu_wo);
        removeChildWindow(_menu_wo->getWindow());
        XDestroyWindow(_dpy, _menu_wo->getWindow());
        delete _menu_wo;
    }

    list<PMenu::Item*>::iterator it(_item_list.begin());
    for (; it != _item_list.end(); ++it) {
        delete *it;
    }

    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_menu_bg_fo);
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_menu_bg_un);
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_menu_bg_se);
}

// START - PWinObj interface.

//! @brief Unmapping, deselecting current item and unsticking.
void
PMenu::unmapWindow(void)
{
    _item_curr = _item_list.end();
    _sticky = false;

    PDecor::unmapWindow();
}

//! @brief
void
PMenu::setFocused(bool focused)
{
    if (_focused != focused) {
        PDecor::setFocused(focused);

        _menu_wo->setBackgroundPixmap(_focused ? _menu_bg_fo : _menu_bg_un);
        _menu_wo->clear();
        if (_item_curr != _item_list.end()) {
            list<PMenu::Item*>::iterator item(_item_curr);
            _item_curr = _item_list.end();
            selectItem(item);
        }
    }
}

//! @brief
ActionEvent*
PMenu::handleButtonPress(XButtonEvent *ev)
{
    if (*_menu_wo == ev->window) {
        handleItemEvent(MOUSE_EVENT_PRESS, ev->x, ev->y);

        // update pointer position
        _pointer_x = ev->x_root;
        _pointer_y = ev->y_root;

        return ActionHandler::findMouseAction(ev->button, ev->state,
                                              MOUSE_EVENT_PRESS,
                                              Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_MENU));
    } else {
        return PDecor::handleButtonPress(ev);
    }
}

//! @brief
ActionEvent*
PMenu::handleButtonRelease(XButtonEvent *ev)
{
    if (*_menu_wo == ev->window) {
        MouseEventType mb = MOUSE_EVENT_RELEASE;

        // first we check if it's a double click
        if (PScreen::instance()->isDoubleClick(ev->window, ev->button - 1, ev->time,
                                               Config::instance()->getDoubleClickTime())) {
            PScreen::instance()->setLastClickID(ev->window);
            PScreen::instance()->setLastClickTime(ev->button - 1, 0);

            mb = MOUSE_EVENT_DOUBLE;

        } else {
            PScreen::instance()->setLastClickID(ev->window);
            PScreen::instance()->setLastClickTime(ev->button - 1, ev->time);
        }

        handleItemEvent(mb, ev->x, ev->y);

        return ActionHandler::findMouseAction(ev->button, ev->state, mb,
                                              Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_MENU));
    } else {
        return PDecor::handleButtonRelease(ev);
    }
}

//! @brief
ActionEvent*
PMenu::handleMotionEvent(XMotionEvent *ev)
{
    if (*_menu_wo == ev->window) {
        handleItemEvent(MOUSE_EVENT_MOTION, ev->x, ev->y);

        ActionEvent *ae;
        uint button = PScreen::instance()->getButtonFromState(ev->state);
        ae = ActionHandler::findMouseAction(button, ev->state,
                                            MOUSE_EVENT_MOTION,
                                            Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_MENU));

        // check motion threshold
        if ((ae != NULL) && (ae->threshold > 0)) {
            if (!ActionHandler::checkAEThreshold(ev->x_root, ev->y_root,
                                                 _pointer_x, _pointer_y, ae->threshold)) {
                ae = NULL;
            }
        }
        return ae;
    } else {
        return PDecor::handleMotionEvent(ev);
    }
}

//! @brief
ActionEvent*
PMenu::handleEnterEvent(XCrossingEvent *ev)
{
    if (*_menu_wo == ev->window) {
        return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
                                              MOUSE_EVENT_ENTER,
                                              Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_MENU));
    } else {
        return PDecor::handleEnterEvent(ev);
    }
}

//! @brief
ActionEvent*
PMenu::handleLeaveEvent(XCrossingEvent *ev)
{
    if (*_menu_wo == ev->window) {
        return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
                                              MOUSE_EVENT_LEAVE,
                                              Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_MENU));
    } else {
        return PDecor::handleLeaveEvent(ev);
    }
}

// END - PWinObj interface.

//! @brief
void
PMenu::handleItemEvent(MouseEventType type, int x, int y)
{
    PMenu::Item *item = findItem(x, y);
    if (!item)
        return;

    // Unmap submenu if we enter them on the same event as selecting.
    if (((_item_curr == _item_list.end()) || (item != *_item_curr))
            && Config::instance()->isMenuSelectOn(type))
        select(item, Config::instance()->isMenuEnterOn(type));

    if (Config::instance()->isMenuEnterOn(type)) {
        if ((item->getWORef() != NULL) &&
                (item->getWORef()->getType() == PWinObj::WO_MENU)) {
            // special case for motion, would flicker like crazy if we didn't check
            if ((type != MOUSE_EVENT_MOTION) &&
                    (item->getWORef()->isMapped() == true)) {
                static_cast<PMenu*>(item->getWORef())->unmapSubmenus();
                item->getWORef()->unmapWindow();

            } else if (item->getWORef()->isMapped() == false) {
                // unmap previous opened submenu if any
                unmapSubmenus();
                mapSubmenu(static_cast<PMenu*>(item->getWORef()));
            }
        }
    }

    // Submenus don't have any actions, so we don't exec ( and close them )
    if (item->getAE().action_list.size()
            && Config::instance()->isMenuExecOn(type)) {
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

//! @brief Calculates how much space and how many rows/cols will be needed
void
PMenu::buildMenuCalculate(void)
{
    uint width = 1, height = 1, sep = 0;
    list<PMenu::Item*>::iterator it;
    _has_submenu = false;

    // Get how many visible objects we have
    _size = 0;
    for (it = _item_list.begin(); it != _item_list.end(); ++it) {
        if ((*it)->getType() == PMenu::Item::MENU_ITEM_NORMAL) {
            ++_size;
        } else if ((*it)->getType() == PMenu::Item::MENU_ITEM_SEPARATOR) {
            ++sep;
        }
    }

    if (_size == 0) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PMenu(" << this << ")::buildMenuCalculate()" << endl
             << " *** _size == 0" << endl;
#endif // DEBUG
        return;
    }

    // Calculate max item width, to be used if/when splitting a menu
    // up in rows because of limited vertical space.
    _item_width_max = 1;
    _icon_width = _icon_height = 0;
    for (it = _item_list.begin(); it != _item_list.end(); ++it) {
        // Only include standard items
        if ((*it)->getType() != PMenu::Item::MENU_ITEM_NORMAL) {
            continue;
        }

        // Check if we have a submenu item
        if (!_has_submenu && ((*it)->getWORef() != NULL) &&
            ((*it)->getWORef()->getType() == PWinObj::WO_MENU)) {
          _has_submenu = true;
        }

        // Get icon height if any
        if ((*it)->getIcon()) {
          if ((*it)->getIcon()->getWidth() > _icon_width) {
            _icon_width = (*it)->getIcon()->getWidth();
          }
          if ((*it)->getIcon()->getHeight() > _icon_height) {
            _icon_height = (*it)->getIcon()->getHeight();
          }
        }

        width = _theme->getMenuData()->getFont(OBJECT_STATE_FOCUSED)->getWidth((*it)->getName().c_str());
        if (width > _item_width_max) {
            _item_width_max = width;
        }
    }

    // Make sure icon width and height are not larger than configured.
    if (Config::instance()->getMenuIconWidth()) {
      _icon_width = std::min(Config::instance()->getMenuIconWidth(), _icon_width);
    }
    if (Config::instance()->getMenuIconHeight()) {
      _icon_height = std::min(Config::instance()->getMenuIconHeight(), _icon_height);
    }

    // This is the available width for drawing text on, the rest is reserved
    // for submenu indicator, padding etc.
    _item_width_max_avail = _item_width_max;

    // Continue add padding etc.
    _item_width_max += _theme->getMenuData()->getPad(PAD_LEFT)
      + _theme->getMenuData()->getPad(PAD_RIGHT) + _icon_width;

    // If we have any submenus, increase the maximum width with arrow width +
    // right pad as we are going to pad the arrow from the text too.
    if (_has_submenu) {
        _item_width_max += _theme->getMenuData()->getPad(PAD_RIGHT)
                           + _theme->getMenuData()->getTextureArrow(OBJECT_STATE_FOCUSED)->getWidth();
    }

    // calculate item height
    _item_height = std::max(_theme->getMenuData()->getFont(OBJECT_STATE_FOCUSED)->getHeight(), _icon_height)
      + _theme->getMenuData()->getPad(PAD_UP)
      + _theme->getMenuData()->getPad(PAD_DOWN);
    _separator_height = _theme->getMenuData()->getTextureSeparator(OBJECT_STATE_FOCUSED)->getHeight();

    height = (_item_height * _size) + (_separator_height * sep);

    // check if menu should have more than one row
    if ((height + getTitleHeight()) > PScreen::instance()->getHeight()) {
        _cols = height / (PScreen::instance()->getHeight() - getTitleHeight());
        if ((height % (PScreen::instance()->getHeight() - getTitleHeight())) != 0) {
            ++_cols;
        }
        _rows = _size / _cols;
        if ((_size % _cols) != 0) {
            ++_rows;
        }
    } else {
        _cols = 1;
        _rows = _size;
    }

    width = _cols * _item_width_max;
    // need to calculate max height, the one with most separators if any
    if (_cols > 1) {
        uint i, j, row_height;
        height = 0;

        for (i = 0, it = _item_list.begin(); i < _cols; ++i) {
            row_height = 0;
            for (j = 0; (j < _rows) && (it != _item_list.end()); ++it) {
                if ((*it)->getType() == PMenu::Item::MENU_ITEM_NORMAL) {
                    row_height += _item_height;
                    ++j; // only count real menu items
                } else if ((*it)->getType() == PMenu::Item::MENU_ITEM_SEPARATOR) {
                    row_height += _separator_height;
                }
            }

            if (row_height > height) {
                height = row_height;
            }
        }
    }

    // check if we need to enable scrolling
    if (width > PScreen::instance()->getWidth()) {
        _scroll = true;
    } else {
        _scroll = false;
    }

    resizeChild(width, height);
}

//! @brief Places the items in the menu
void
PMenu::buildMenuPlace(void)
{
    uint x, y;
    list<PMenu::Item*>::iterator it;

    x = 0;
    it = _item_list.begin();
    // cols
    for (uint i = 0; i < _cols; ++i) {
        y = 0;
        // rows
        for (uint j = 0; (j < _rows) && (it != _item_list.end()); ++it) {
            if ((*it)->getType() != PMenu::Item::MENU_ITEM_HIDDEN) {
                (*it)->setX(x);
                (*it)->setY(y);
                if ((*it)->getType() == PMenu::Item::MENU_ITEM_NORMAL) {
                    y += _item_height;
                    ++j; // only count real menu items
                } else if ((*it)->getType() == PMenu::Item::MENU_ITEM_SEPARATOR) {
                    y += _separator_height;
                }
            }
        }
        x += _item_width_max;
    }
}

//! @brief Renders focused, unfocused and selected pixmaps for menu
void
PMenu::buildMenuRender(void)
{
    buildMenuRenderState(_menu_bg_fo, OBJECT_STATE_FOCUSED);
    buildMenuRenderState(_menu_bg_un, OBJECT_STATE_UNFOCUSED);
    buildMenuRenderState(_menu_bg_se, OBJECT_STATE_SELECTED);

    _menu_wo->setBackgroundPixmap(_focused ? _menu_bg_fo : _menu_bg_un);
    _menu_wo->clear();
}

//! @brief Renders menu content on pix, with state state
void
PMenu::buildMenuRenderState(Pixmap &pix, ObjectState state)
{
    PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();

    // get a fresh pixmap for the menu
    pm->returnPixmap(pix);
    pix = pm->getPixmap(getChildWidth(), getChildHeight(),
                        PScreen::instance()->getDepth());

    PTexture *tex;
    PFont *font;

    tex = _theme->getMenuData()->getTextureMenu(state);
    tex->render(pix, 0, 0, getChildWidth(), getChildHeight());

    font = _theme->getMenuData()->getFont(state);
    font->setColor(_theme->getMenuData()->getColor(state));

    list<PMenu::Item*>::iterator it(_item_list.begin());
    for (; it != _item_list.end(); ++it) {
        if ((*it)->getType() != PMenu::Item::MENU_ITEM_HIDDEN) {
            buildMenuRenderItem(pix, state, *it);
        }
    }
}

//! @brief Renders item on pix, with state state
void
PMenu::buildMenuRenderItem(Pixmap pix, ObjectState state, PMenu::Item *item)
{
    PTexture *tex;
    Theme::PMenuData *md = _theme->getMenuData();

    if (item->getType() == PMenu::Item::MENU_ITEM_NORMAL) {
        tex = md->getTextureItem(state);
        tex->render(pix,
                    item->getX(), item->getY(), _item_width_max, _item_height);

        // If entry has an icon, draw it
        if (item->getIcon()) {
          uint i_w = std::min(Config::instance()->getMenuIconWidth(),
                              item->getIcon()->getWidth());
          uint i_h = std::min(Config::instance()->getMenuIconHeight(),
                              item->getIcon()->getHeight());

          item->getIcon()->render(pix,
                                  item->getX() + md->getPad(PAD_LEFT) + (_icon_width - i_w) / 2,
                                  item->getY() + (_item_height - i_h) / 2,
                                  i_w, i_h);
        }

        // If entry has a submenu, lets draw our submenu "arrow"
        if (item->getWORef()
                && (item->getWORef()->getType() == PWinObj::WO_MENU)) {
            tex = md->getTextureArrow(state);
            uint a_w = tex->getWidth();
            uint a_h = tex->getHeight();
            uint a_y = static_cast<uint>((_item_height / 2) - (a_h / 2));

            tex->render(pix,
                        item->getX() + _item_width_max - a_w -
                        md->getPad(PAD_RIGHT),
                        item->getY() + a_y,
                        a_w, a_h);
        }

        PFont *font = md->getFont(state);
        font->draw(pix,
                   item->getX() + md->getPad(PAD_LEFT) + _icon_width,
                   item->getY() + md->getPad(PAD_UP),
                   item->getName().c_str(), 0, // Text and max chars
                   _item_width_max_avail);

    } else if ((item->getType() == PMenu::Item::MENU_ITEM_SEPARATOR) &&
               (state < OBJECT_STATE_SELECTED)) {
        tex = md->getTextureSeparator(state);
        tex->render(pix,
                    item->getX(), item->getY(),
                    _item_width_max, _separator_height);
    }
}

#define COPY_ITEM_AREA(ITEM, PIX) \
		XCopyArea(_dpy, PIX, _menu_wo->getWindow(), PScreen::instance()->getGC(), \
							(ITEM)->getX(), (ITEM)->getY(), _item_width_max, _item_height, \
							(ITEM)->getX(), (ITEM)->getY());

//! @brief Renders item as selected
//! @param item Item to select
//! @param unmap_submenu Defaults to true
void
PMenu::selectItem(std::list<PMenu::Item*>::iterator item, bool unmap_submenu)
{
    if (_item_curr == item)
        return;

    deselectItem(unmap_submenu);

    _item_curr = item;

    if (_mapped)
        COPY_ITEM_AREA((*item), _menu_bg_se);
}

//! @brief Deselects selected item
//! @þaram unmap_submenu Defaults to true
void
PMenu::deselectItem(bool unmap_submenu)
{
    // deselect previous item
    if ((_item_curr != _item_list.end())
            && ((*_item_curr)->getType() != PMenu::Item::MENU_ITEM_HIDDEN)) {
        if (_mapped)
            COPY_ITEM_AREA((*_item_curr), (_focused ? _menu_bg_fo : _menu_bg_un));

        if (unmap_submenu && (*_item_curr)->getWORef()
                && ((*_item_curr)->getWORef()->getType() == PWinObj::WO_MENU)) {
            static_cast<PMenu*>((*_item_curr)->getWORef())->unmapSubmenus();
            (*_item_curr)->getWORef()->unmapWindow();
        }
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

    list<PMenu::Item*>::iterator item(_item_curr);

    // no item selected, select the first item
    if (item == _item_list.end()) {
        item = _item_list.begin();

        // select next item, wrap if needed
    } else {
        ++item;
        if (item == _item_list.end()) {
            item = _item_list.begin();
        }
    }

    // skip to next if separator/hidden
    if ((*item)->getType() != PMenu::Item::MENU_ITEM_NORMAL) {
        deselectItem(); // otherwise, last selected won't get deselcted
        _item_curr = item;
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

    list<PMenu::Item*>::iterator item( _item_curr);

    // no item selected, select the last item OR
    // we're at the beginning and need to wrap to the end
    if ((item == _item_list.end()) || (item == _item_list.begin())) {
        item = _item_list.end();
    }
    --item;

    // skip to prev if separator/hidden
    if ((*item)->getType() != PMenu::Item::MENU_ITEM_NORMAL) {
        deselectItem(); // otherwise, last selected won't get deselcted
        _item_curr = item;
        selectPrevItem();
    } else {
        selectItem(item);
    }
}

//! @brief Sets title of the menu/decor
void
PMenu::setTitle(const std::wstring &title)
{
    _title.setReal(title);
}

//! @brief Inserts item into the menu ( without rebuilding )
void
PMenu::insert(PMenu::Item *item)
{
    checkItemWORef(item);
    _item_list.push_back(item);
}

//! @brief Creates and inserts Item
//! @param name Name of objet to create and insert
//! @param wo_ref PWinObj to refer to, defaults to NULL
void
PMenu::insert(const std::wstring &name, PWinObj *wo_ref, PTexture *icon)
{
    PMenu::Item *item;

    item = new PMenu::Item(name, wo_ref, icon);

    insert(item);
}

//! @brief Creates and inserts Item
//! @param name Name of object to create and insert
//! @param ae ActionEvent for the object
//! @param wo_ref PWinObj to refer to, defaults to NULL
void
PMenu::insert(const std::wstring &name, const ActionEvent &ae, PWinObj *wo_ref, PTexture *icon)
{
    PMenu::Item *item;

    item = new PMenu::Item(name, wo_ref, icon);
    item->setAE(ae);

    insert(item);
}

//! @brief Removes an item from the menu, without rebuilding.
void
PMenu::remove(PMenu::Item *item)
{
    if (item == NULL) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PMenu(" << this << ")::remove(" << item << ")" << endl
             << " *** item == NULL" << endl;
#endif // DEBUG
        return;
    }

    if ((_item_curr != _item_list.end()) && (item == *_item_curr))
        _item_curr = _item_list.end();

    delete item;
    _item_list.remove(item);
}

//! @brief Removes all items from the menu, without rebuilding.
void
PMenu::removeAll(void)
{
    list<PMenu::Item*>::iterator it(_item_list.begin());
    for (; it != _item_list.end(); ++it) {
        delete *it;
    }
    _item_list.clear();
}

//! @brief Places the menu under the mouse and maps it.
void
PMenu::mapUnderMouse(void)
{
    int x, y;

    PScreen::instance()->getMousePosition(x, y);

    // this might seem a bit silly but the menu won't get updated before
    // it has been mapped (if dynamic) so we're doing it twice to reduce the
    // "flickering" risk but it's not 100% so it's done twice.
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
    if (_item_curr != _item_list.end()) {
        y = _gm.y + (*_item_curr)->getY();
    } else {
        y = _gm.y;
    }

    // this might seem a bit silly but the menu won't get updated before
    // it has been mapped (if dynamic) so we're doing it twice to reduce the
    // "flickering" risk but it's not 100% so it's done twice.
    menu->makeInsideScreen(x, y);
    menu->mapWindowRaised();
    menu->makeInsideScreen(x, y);

    if (focus == true) {
        menu->giveInputFocus();
        menu->selectItemNum(0);
    }
}

//! @brief Unmaps all ( recursive ) submenus open under this menu
void
PMenu::unmapSubmenus(void)
{
    list<PMenu::Item*>::iterator it(_item_list.begin());
    for (; it != _item_list.end(); ++it) {
        if ((*it)->getWORef() && (*it)->getWORef()->getType() == PWinObj::WO_MENU) {
            if (! (*it)->isDynamic()) {
                // Sub-menus will be deleted when unmapping this, so no need
                // to continue.
                static_cast<PMenu*>((*it)->getWORef())->unmapSubmenus();
            }
            (*it)->getWORef()->unmapWindow();
        }
    }
}

//! @brief Unmaps all menus belonging to this menu
void
PMenu::unmapAll(void)
{
    if (_menu_parent != NULL) {
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
    if (_menu_parent == NULL) {
        return;
    }

    _menu_parent->unmapSubmenus();
    _menu_parent->giveInputFocus();
}

//! @brief Selects item, if NULL/not in list current item is deselected
//! @param item Item to select
//! @þaram unmap_submenu Defaults to true
void
PMenu::select(PMenu::Item *item, bool unmap_submenu)
{
    selectItem(find(_item_list.begin(), _item_list.end(), item), unmap_submenu);
}

//! @brief Selects item number num in menu
void
PMenu::selectItemNum(uint num)
{
    if (num > _item_list.size()) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PMenu(" << this << ")::selectItem(" << num << ")"
             << " *** num > _item_list_size()[" << _item_list.size()
             << "]" << endl;
#endif // DEBUG
        return;
    }

    list<PMenu::Item*>::iterator it(_item_list.begin());
    for (uint i = 0; i < num; ++i, ++it)
        ;

    selectItem(it);
}

//! @brief Selects item relative to the selected
void
PMenu::selectItemRel(int off)
{
    if (off == 0) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PMenu(" << this << ")::selectItemRel(" << off << ")"
             << " *** off == 0" << endl;
#endif // DEBUG
        return;
    }

    // if no selected item, use first
    list<PMenu::Item*>::iterator it((_item_curr == _item_list.end()) ? _item_list.begin() : _item_curr);

    int dir = (off > 0) ? 1 : -1;
    off = abs(off);

    for (int i = 0; i < off; ++i) {
        if (dir == 1) { // forward
            if (++it == _item_list.end()) {
                it = _item_list.begin();
            }

        } else { // backward
            if (it == _item_list.begin()) {
                it = _item_list.end();
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
    if (_menu_parent != NULL) {
        _menu_parent->exec(item);
    } else {
        handleItemExec(item);
        if (_sticky == false) {
            unmapAll();
        }
    }
}

//! @brief Sets up children _menu_parent field, if item's _wo_ref is a menu
void
PMenu::checkItemWORef(PMenu::Item *item)
{
    if ((item->getWORef() != NULL) &&
            (item->getWORef()->getType() == PWinObj::WO_MENU)) {
        PMenu *child = static_cast<PMenu*>(item->getWORef());
        child->_menu_parent = this;
    }
}

//! @brief Searches for item at x, y
PMenu::Item*
PMenu::findItem(int x, int y)
{
    list<PMenu::Item*>::iterator it(_item_list.begin());
    for (; it != _item_list.end(); ++it) {
        if (((*it)->getType() == PMenu::Item::MENU_ITEM_NORMAL) &&
                (x >= (*it)->getX()) && (x <= signed((*it)->getX() + _item_width_max)) &&
                (y >= (*it)->getY()) && (y <= signed((*it)->getY() + _item_height))) {
            return *it;
        }
    }
    return NULL;
}

//! @brief Moves the menu relative to it's parent to make it fit on screen
//! @param x Use x instead of _gm.x ( optional )
//! @param y Use y instead of _gm.y ( optional )
void
PMenu::makeInsideScreen(int x, int y)
{
    Geometry head;
    PScreen::instance()->getHeadInfo(PScreen::instance()->getCurrHead(), head);

    x = (x == -1) ? _gm.x : x;
    y = (y == -1) ? _gm.y : y;

    // we map on submenus on the right side so this only happens on the
    // top-level menu
    if (x < head.x) {
        x = head.x;
    } else if ((x + _gm.width) > (head.x + head.width)) {
        if (_menu_parent != NULL) {
            x = _menu_parent->_gm.x - _gm.width; // not using getX(), refers to child
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
