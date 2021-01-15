//
// MenuHandler.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _MENUHANDLER_HH_
#define _MENUHANDLER_HH_

#include "config.h"

#include <map>
#include <string>

#include "PMenu.hh"

class Theme;
class ActionHandler;

/**
 * Menu manager, creates, reloads and delete menus.
 */
class MenuHandler {
public:
    static PMenu *getMenu(const std::string &name) {
        auto it = _menu_map.find(name);
        return (it != _menu_map.end()) ? it->second : 0;
    }

    /**
     * Return list with names of loaded menus.
     */
    static std::vector<std::string> getMenuNames(void) {
        std::vector<std::string> menu_names;
        for (auto it : _menu_map) {
            menu_names.push_back(it.second->getName());
        }
        return menu_names;
    }

    static void createMenus(ActionHandler *act);
    static void hideAllMenus(void) {
        for (auto it : _menu_map) {
            it.second->unmapAll();
        }
    }
    static void reloadMenus(ActionHandler *act);
    static void deleteMenus(void);

private:
    static bool loadMenuConfig(const std::string &menu_file, CfgParser &menu_cfg);
    static void createMenusLoadConfiguration(ActionHandler *act);
    static void reloadStandaloneMenus(ActionHandler *act, CfgParser::Entry *section);

    static TimeFiles _cfg_files;
    static std::map<std::string, PMenu*> _menu_map; /**< Map from menu name to menu */
};

#endif // _MENUHANDLER_HH_
