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

#ifdef MENUS

#include <map>
#include <list>
#include <string>

#include "CfgParser.hh"
#include "PMenu.hh"
#include "Theme.hh"

/**
 * Menu manager, creates, reloads and delete menus.
 */
class MenuHandler {
public:
    static void init(Theme *theme);
    static void destroy(void);
    static MenuHandler *instance(void) { return _instance; }

    PMenu *getMenu(const std::string &name) {
        std::map<std::string, PMenu*>::iterator it = _menu_map.find(name);
        return (it != _menu_map.end()) ? it->second : 0;
    }

    /**
     * Return list with names of loaded menus.
     */
    std::list<std::string> getMenuNames(void) {
        std::list<std::string> menu_names;
        std::map<std::string, PMenu*>::iterator it(_menu_map.begin());
        for (; it != _menu_map.end(); ++it) {
            menu_names.push_back(it->second->getName());
        }
        return menu_names;
    }

    void hideAllMenus(void);
    void reloadMenus(void);

private:
    MenuHandler(Theme *theme);
    MenuHandler(MenuHandler &menu_handler);
    ~MenuHandler(void);

    bool loadMenuConfig(const std::string &menu_file, CfgParser &menu_cfg);

    void createMenus(void);
    void createMenusLoadConfiguration(void);
    void deleteMenus(void);

    void reloadStandaloneMenus(CfgParser::Entry *section);

    static bool isReservedName(const std::string &name);

private:
    Theme *_theme; /**< Theme in use. */

    std::map <std::string, time_t> _menu_state; /**< Map of file mtime for all files touched by a configuration. */
    std::map<std::string, PMenu*> _menu_map; /**< Map from menu name to menu */

    static MenuHandler *_instance; /**< Instance pointer for MenuHandler. */

    static const char *MENU_NAMES_RESERVED[];
    static const unsigned int MENU_NAMES_RESERVED_COUNT;
};

#endif // MENUS
