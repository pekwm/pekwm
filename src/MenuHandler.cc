//
// MenuHandler.cc for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <cassert>
#include <map>
#include <string>

#include "PWinObj.hh"
#include "PMenu.hh"
#include "MenuHandler.hh"
#include "ActionHandler.hh"

#include "WORefMenu.hh"
#include "ActionMenu.hh"
#include "FrameListMenu.hh"

TimeFiles MenuHandler::_cfg_files;
std::map<std::string, PMenu*> MenuHandler::_menu_map;

/**
 * Creates reserved menus and populates _menu_map
 */
void
MenuHandler::createMenus(ActionHandler *act)
{
    PMenu *menu = 0;

    menu = new FrameListMenu(ATTACH_CLIENT_IN_FRAME_TYPE,
                             L"Attach Client In Frame",
                             "AttachClientInFrame");
    _menu_map["ATTACHCLIENTINFRAME"] = menu;
     menu = new FrameListMenu(ATTACH_CLIENT_TYPE,
                              L"Attach Client", "AttachClient");
    _menu_map["ATTACHCLIENT"] = menu;
     menu = new FrameListMenu(ATTACH_FRAME_IN_FRAME_TYPE,
                              L"Attach Frame In Frame",
                              "AttachFrameInFrame");
    _menu_map["ATTACHFRAMEINFRAME"] = menu;
    menu = new FrameListMenu(ATTACH_FRAME_TYPE,
                             L"Attach Frame", "AttachFrame");
    _menu_map["ATTACHFRAME"] = menu;
    menu = new FrameListMenu(GOTOCLIENTMENU_TYPE,
                             L"Focus Client", "GotoClient");
    _menu_map["GOTOCLIENT"] = menu;
    menu = new FrameListMenu(GOTOMENU_TYPE,
                             L"Focus Frame", "Goto");
    _menu_map["GOTO"] = menu;
    menu = new FrameListMenu(ICONMENU_TYPE,
                             L"Focus Iconified Frame", "Icon");
    _menu_map["ICON"] = menu;
    menu =  new ActionMenu(ROOTMENU_TYPE, act, L"", "RootMenu");
    _menu_map["ROOT"] = menu;
    menu = new ActionMenu(WINDOWMENU_TYPE, act, L"", "WindowMenu");
    _menu_map["WINDOW"] = menu;

    createMenusLoadConfiguration(act);
}

/**
 * Initial load of menu configuration.
 */
void
MenuHandler::createMenusLoadConfiguration(ActionHandler *act)
{
    // Load configuration, pass specific section to loading
    CfgParser menu_cfg;
    if (menu_cfg.parse(Config::instance()->getMenuFile())
        || menu_cfg.parse(std::string(SYSCONFDIR "/menu"))) {
        _cfg_files = menu_cfg.getCfgFiles();
        auto root_entry = menu_cfg.getEntryRoot();

        // Load standard menus
        for (auto it : _menu_map) {
            it.second->reload(root_entry->findSection(it.second->getName()));
        }

        // Load standalone menus
        reloadStandaloneMenus(act, menu_cfg.getEntryRoot());
    }
}

/**
 * (re)loads the menus in the menu configuration if the file has been
 * updated since last load.
 */
void
MenuHandler::reloadMenus(ActionHandler *act)
{
    std::string menu_file(Config::instance()->getMenuFile());
    if (! _cfg_files.requireReload(menu_file)) {
        return;
    }

    CfgParser cfg;
    bool cfg_ok = loadMenuConfig(menu_file, cfg);
    CfgParser::Entry *root = cfg.getEntryRoot();

    // Update, delete standalone root menus, load decors on others
    auto it(_menu_map.begin());
    while (it != _menu_map.end()) {
        if (it->second->getMenuType() == ROOTMENU_STANDALONE_TYPE) {
            delete it->second;
            _menu_map.erase(it++);
            continue;
        } else if (cfg_ok) {
            // Only reload the menu if we got a ok configuration
            it->second->reload(root->findSection(it->second->getName()));
        }
        ++it;
    }

    // Update standalone root menus (name != ROOTMENU)
    reloadStandaloneMenus(act, root);
}

/**
 * Load menu configuration from menu_file resetting menu state.
 */
bool
MenuHandler::loadMenuConfig(const std::string &menu_file, CfgParser &menu_cfg)
{
    bool cfg_ok = true;

    if (! menu_cfg.parse(menu_file)) {
        if (! menu_cfg.parse(std::string(SYSCONFDIR "/menu"))) {
            cfg_ok = false;
        }
    }

    // Make sure menu is reloaded next time as content is dynamically
    // generated from the configuration file.
    if (! cfg_ok || menu_cfg.isDynamicContent()) {
        _cfg_files.clear();
    } else {
        _cfg_files = menu_cfg.getCfgFiles();
    }

    return cfg_ok;
}

/**
 * Updates standalone root menus
 */
void
MenuHandler::reloadStandaloneMenus(ActionHandler *act,
                                   CfgParser::Entry *section)
{
    // Temporary name, as names are stored uppercase
    std::string menu_name_upper;

    // Go through all but reserved section names and create menus
    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        // Uppercase name
        menu_name_upper = (*it)->getName();
        Util::to_upper(menu_name_upper);

        // Create new menu if the name is not used
        if (! getMenu(menu_name_upper)) {
            // Create, parse and add to map
            auto menu = new ActionMenu(ROOTMENU_STANDALONE_TYPE, act,
                                       L"", (*it)->getName());
            menu->reload((*it)->getSection());
            _menu_map[menu_name_upper] = menu;
        }
    }
}

/**
 * Clears the menu map and frees up resources used by menus
 */
void
MenuHandler::deleteMenus(void)
{
    for (auto it : _menu_map) {
        delete it.second;
    }
    _menu_map.clear();
}
