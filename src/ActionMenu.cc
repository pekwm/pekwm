//
// ActionMenu.cc for pekwm
// Copyright © 2002-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef MENUS

#include <cstdio>
#include <iostream>

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"
#include "ActionMenu.hh"
#include "TextureHandler.hh"
#include "ImageHandler.hh"

#include "Config.hh"
#include "ActionHandler.hh"
#include "Client.hh"
#include "Frame.hh"
#include "WindowManager.hh"
#include "Util.hh"

using std::cerr;
using std::endl;
using std::find;
using std::list;
using std::string;
using std::wstring;

//! @brief ActionMenu constructor
//! @param type Type of menu
//! @param title Title of menu
//! @param name Name of the menu, empty for dynamic else should be unique
//! @param decor_name Name of decor to use, defaults to MENU.
ActionMenu::ActionMenu(MenuType type,
                       const std::wstring &title, const std::string &name,
                       const std::string &decor_name) :
    WORefMenu(WindowManager::instance()->getScreen(),
              WindowManager::instance()->getTheme(), title, name, decor_name),
    _act(WindowManager::instance()->getActionHandler()),
    _is_dynamic(false), _has_dynamic(false)
{
    // when creating dynamic submenus, this needs to be initialized as
    // dynamic inserting will be done
    _insert_at = _item_list.begin();
    _menu_type = type;

    if (_menu_type == WINDOWMENU_TYPE) {
        _action_ok = WINDOWMENU_OK;
    } else if ((_menu_type == ROOTMENU_TYPE) || (_menu_type == ROOTMENU_STANDALONE_TYPE)) {
        _action_ok = ROOTMENU_OK;
    }
}

//! @brief ActionMenu destructor
ActionMenu::~ActionMenu(void)
{
    removeAll();
}

// START - PWinObj interface.

//! @brief Rebuilds the vmenu and if it has any items after it shows it.
void
ActionMenu::mapWindow(void)
{
    // find and rebuild the dynamic entries
    if (! isMapped() && _has_dynamic) {
        uint size_before = _item_list.size();
        rebuildDynamic();
        if (size_before != _item_list.size()) {
            buildMenu();
        }
    }

    PMenu::mapWindow();
}

//! @brief Hides and removes all items created by dynamic entries.
void
ActionMenu::unmapWindow(void)
{
    if (! isMapped()) {
        return;
    }

    if (_has_dynamic) {
        removeDynamic();
    }

    PMenu::unmapWindow();
}

// END - PWinObj interface.

//! @brief Handle item exec.
void
ActionMenu::handleItemExec(PMenu::Item *item)
{
    if (! item) {
        return;
    }

    ActionPerformed ap(_wo_ref, item->getAE());
    _act->handleAction(ap);
}


/**
 * Re-reads the configuration clearing old entries from the menu. This
 * adds the ICON_PATH to the ImageHandler before loading to support
 * loading icons properly.
 *
 * @param section CfgParser::Entry with menu configuration
 */
void
ActionMenu::reload(CfgParser::Entry *section)
{
    // Clear menu
    removeAll();

    // Parse section (if any)
    ImageHandler::instance()->path_push_back(Config::instance()->getSystemIconPath());
    ImageHandler::instance()->path_push_back(Config::instance()->getIconPath());
    parse(section);
    ImageHandler::instance()->path_pop_back();
    ImageHandler::instance()->path_pop_back();

    // Build menu from parsed content
    buildMenu();
}

//! @brief Checks if we have a position set for where to insert.
void
ActionMenu::insert(PMenu::Item *item)
{
    if (! item) {
        return;
    }

    checkItemWORef(item);

    if (item->isDynamic()) {
        _insert_at = _item_list.insert(++_insert_at, item);
    } else {
        _item_list.push_back(item);
    }
}

//! @brief Non-shadowing PMenu::insert
void
ActionMenu::insert(const std::wstring &name, PWinObj *wo_ref, PTexture *icon)
{
    PMenu::insert(name, wo_ref, icon);
}

//! @brief Non-shadowing PMenu::insert
void
ActionMenu::insert(const std::wstring &name, const ActionEvent &ae, PWinObj *wo_ref, PTexture *icon)
{
    PMenu::insert(name, ae, wo_ref);
}

//! @brief Removes a BaseMenuItem from the menu
void
ActionMenu::remove(PMenu::Item *item)
{
    if (! item) {
        return;
    }

    if (item->getIcon()) {
        TextureHandler::instance()->returnTexture(item->getIcon());
    }

    if (item->getWORef() && (item->getWORef()->getType() == WO_MENU)) {
        delete item->getWORef();
    }

    PMenu::remove(item);
}

//! @brief Removes all items from the menu
void
ActionMenu::removeAll(void)
{
    while (_item_list.size() > 0) {
        remove(_item_list.back());
    }
}

//! @brief Parse config and push items into menu
//! @param cs Section object to read config from
//! @param menu BaseMenu object to push object in
//! @param has_dynamic If true the menu being parsed is dynamic, defaults to false.
void
ActionMenu::parse(CfgParser::Entry *section, bool has_dynamic)
{
    if (! section) {
        return;
    }

    if (section->get_value().size()) {
        wstring title(Util::to_wide_str(section->get_value()));
        setTitle(title);
        _title_base = title;
    }

    CfgParser::Entry *value;
    ActionEvent ae;

    ActionMenu *submenu = 0;
    PMenu::Item *item = 0;
    PTexture *icon = 0;

    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        item = 0;

        if (*(*it) == "SUBMENU") {
            CfgParser::Entry *sub_section = (*it)->get_section();
            if (sub_section) {
                icon = getIcon(sub_section->find_entry("ICON"));

                submenu = new ActionMenu(_menu_type, Util::to_wide_str((*it)->get_value()), "");
                submenu->_is_dynamic = has_dynamic;
                submenu->_menu_parent = this;
                submenu->parse(sub_section, has_dynamic);
                submenu->buildMenu();

                item = new PMenu::Item(Util::to_wide_str(sub_section->get_value()), submenu, icon);
                item->setDynamic(has_dynamic);
            } else {
                cerr << " *** WARNING: submenu entry does not contain any section." << endl;
            }
        } else if (*(*it) == "SEPARATOR") {
            // No icon support on separators.
            item = new PMenu::Item(L"", 0, 0);
            item->setDynamic(has_dynamic);
            item->setType(PMenu::Item::MENU_ITEM_SEPARATOR);


        } else {
            CfgParser::Entry *sub_section = (*it)->get_section();
            if (sub_section) {
                // Inside of the Entry = "foo" { ... } section, here
                // Actions and Icon are the valid options.
                value = sub_section->find_entry("ACTIONS");
                if (value && Config::instance()->parseActions(value->get_value(), ae, _action_ok)) {
                    icon = getIcon(sub_section->find_entry("ICON"));

                    item = new PMenu::Item(Util::to_wide_str(sub_section->get_value()), 0, icon);
                    item->setDynamic(has_dynamic);
                    item->setAE(ae);

                    if (ae.isOnlyAction(ACTION_MENU_DYN)) {
                        _has_dynamic = true;
                        item->setType(PMenu::Item::MENU_ITEM_HIDDEN);
                    }
                }
            }
        }

        // If an item was successfully created, insert it to the menu.
        if (item) {
            ActionMenu::insert (item);
        }
    }
}

/**
 * Get icon texture from parser value.
 *
 * @param value Entry to get icon name from.
 * @return PTexture if icon was loaded, else 0.
 */
PTexture*
ActionMenu::getIcon(CfgParser::Entry *value)
{
    PTexture *icon = 0;

    if (value) {
        icon = TextureHandler::instance()->getTexture("IMAGE " + value->get_value() + "#SCALED");
    }

    return icon;
}

/**
 * Executes all Dynamic entries in the menu.
 */
void
ActionMenu::rebuildDynamic(void)
{
    // Setup icon path before parsing.
    ImageHandler::instance()->path_push_back(Config::instance()->getSystemIconPath());
    ImageHandler::instance()->path_push_back(Config::instance()->getIconPath());

    PMenu::Item* item = 0;
    list<PMenu::Item*>::iterator it;
    for (it = _item_list.begin(); it != _item_list.end(); ++it) {
        if ((*it)->getAE().isOnlyAction(ACTION_MENU_DYN)) {
            _insert_at = it;

            item = *it;

            CfgParser dynamic;
            if (dynamic.parse((*it)->getAE().action_list.front().getParamS(),
                              CfgParserSource::SOURCE_COMMAND)) {
                _has_dynamic = true;
                parse(dynamic.get_entry_root()->find_section("DYNAMIC"), true);
            }

            it = find(_item_list.begin(), _item_list.end(), item);
        }
    }

    // Cleanup icon path
    ImageHandler::instance()->path_pop_back();
    ImageHandler::instance()->path_pop_back();
}

//! @brief Remove all entries from the menu created by dynamic entries.
void
ActionMenu::removeDynamic(void)
{
    // FIXME: This is a kludge.
    // Problem: If a submenu is created by an "dynamic"-action, we cannot delete all the items
    // as we cannot recreate them (we would need the original output of the 
    // dynamic-command). Therefore we just unmap them and hope for the best.
    // Second shortfall: We don't know for sure if this menu actually _was_ created dynamically, 
    // hence the test with _parent. 

    bool do_unmap = false;
    if (_menu_parent) {
        ActionMenu *parent = dynamic_cast<ActionMenu*>(_menu_parent);
        do_unmap = (parent && parent->_is_dynamic);
    }

    list<PMenu::Item*>::iterator it(_item_list.begin());
    for (; it != _item_list.end(); ++it) {
        if ((*it)->isDynamic()) {
            if (do_unmap) {
                if ((*it)->getWORef() && ((*it)->getWORef()->getType() == WO_MENU)) {
                    (*it)->getWORef()->unmapWindow();
                }
            } else {
                if ((*it)->getWORef() && ((*it)->getWORef()->getType() == WO_MENU)) {
                    delete (*it)->getWORef();
                }
                delete (*it);

                it = _item_list.erase(it);
                --it; // compensate for the ++ in the loop
            }
        }
    }
}

#endif // MENUS
