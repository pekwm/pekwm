//
// MenuHandler.hh for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <map>
#include <string>
#include <vector>
using std::vector;

#include "PMenu.hh"
class Theme;

/**
 * Menu manager, creates, reloads and delete menus.
 */
class MenuHandler {
public:
    static PMenu *getMenu(const std::string &name) {
        std::map<std::string, PMenu*>::iterator it = _menu_map.find(name);
        return (it != _menu_map.end()) ? it->second : 0;
    }

    /**
     * Return list with names of loaded menus.
     */
    static vector<std::string> getMenuNames(void) {
        vector<std::string> menu_names;
        std::map<std::string, PMenu*>::iterator it(_menu_map.begin());
        for (; it != _menu_map.end(); ++it) {
            menu_names.push_back(it->second->getName());
        }
        return menu_names;
    }

    static void createMenus(Theme *);
    static void hideAllMenus(void) {
        std::map<std::string, PMenu*>::iterator it(_menu_map.begin());
        for (; it != _menu_map.end(); ++it) {
            it->second->unmapAll();
        }
    }
    static void reloadMenus(void);
    static void deleteMenus(void);

private:
    static bool loadMenuConfig(const std::string &menu_file, CfgParser &menu_cfg);

    static void createMenusLoadConfiguration(void);

    static void reloadStandaloneMenus(CfgParser::Entry *section);

    static bool isReservedName(const std::string &name);

    static std::map<std::string, time_t> _menu_state; /**< Map of file mtime for all files touched by a configuration. */
    static std::map<std::string, PMenu*> _menu_map; /**< Map from menu name to menu */
    static const char *MENU_NAMES_RESERVED[];
    static const unsigned int MENU_NAMES_RESERVED_COUNT;
};
