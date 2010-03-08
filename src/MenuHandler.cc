
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef MENUS

#include <cassert>
#include <map>
#include <string>

#include "PWinObj.hh"
#include "PMenu.hh"
#include "PScreen.hh"
#include "MenuHandler.hh"
#include "ActionHandler.hh"

#include "WORefMenu.hh"
#include "ActionMenu.hh"
#include "FrameListMenu.hh"
#include "DecorMenu.hh"

using std::map;
using std::string;

MenuHandler *MenuHandler::_instance = 0;

/**
 * List of reserved names of built-in menus.
 */
const char *MenuHandler::MENU_NAMES_RESERVED[] = {
    "AttachClientInFrame",
    "AttachClient",
    "AttachFrameInFrame",
    "AttachFrame",
    "DecorMenu",
    "GotoClient",
    "Goto",
    "Icon",
    "RootMenu",
    "Root", // To avoid name conflict, ROOTMENU -> ROOT
    "WindowMenu",
    "Window" // To avoid name conflict, WINDOWMENU -> WINDOW
};

const unsigned int MenuHandler::MENU_NAMES_RESERVED_COUNT =
    sizeof(MenuHandler::MENU_NAMES_RESERVED)
    / sizeof(MenuHandler::MENU_NAMES_RESERVED[0]);

/**
 * Comparsion for binary search
 */
bool
str_comparator(const string &lhs, const string &rhs) {
    return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

/**
 * Initialize menu handler instance.
 */
void
MenuHandler::init(Theme *theme)
{
    if (_instance) {
        delete _instance;
    }
    _instance = new MenuHandler(theme);
}

/**
 * Destroy menu handler instance.
 */
void
MenuHandler::destroy(void)
{
    delete _instance;
    _instance = 0;
}

/**
 * Menu handler constructor, create menus.
 */
MenuHandler::MenuHandler(Theme *theme)
    : _theme(theme)
{
    createMenus();
}

/**
 * Menu handler destructor, delete menus.
 */
MenuHandler::~MenuHandler(void)
{
    deleteMenus();
}

/**
 * Hides all menus
 */
void
MenuHandler::hideAllMenus(void)
{
    // bool do_focus = PWinObj::isFocusedPWinObj(PWinObj::WO_MENU);

    map<std::string, PMenu*>::iterator it(_menu_map.begin());
    for (; it != _menu_map.end(); ++it) {
        it->second->unmapAll();
    }

    /**
     * FIXME: Handled in window manager?
     * if (do_focus) {
     * findWOAndFocus(0);
     * }
     */
}

/**
 * Creates reserved menus and populates _menu_map
 */
void
MenuHandler::createMenus(void)
{
    Config *config = Config::instance();
    PScreen *screen = PScreen::instance();
    ActionHandler *action_handler = ActionHandler::instance();
    PMenu *menu = 0;

    menu = new FrameListMenu(screen, _theme, ATTACH_CLIENT_IN_FRAME_TYPE,
                             L"Attach Client In Frame",
                             "ATTACHCLIENTINFRAME");
    _menu_map["ATTACHCLIENTINFRAME"] = menu;
     menu = new FrameListMenu(screen, _theme, ATTACH_CLIENT_TYPE,
                              L"Attach Client", "ATTACHCLIENT");
    _menu_map["ATTACHCLIENT"] = menu;
     menu = new FrameListMenu(screen, _theme, ATTACH_FRAME_IN_FRAME_TYPE,
                              L"Attach Frame In Frame",
                              "ATTACHFRAMEINFRAME");
    _menu_map["ATTACHFRAMEINFRAME"] = menu;
    menu = new FrameListMenu(screen, _theme, ATTACH_FRAME_TYPE,
                             L"Attach Frame", "ATTACHFRAME");
    _menu_map["ATTACHFRAME"] = menu;
    menu = new FrameListMenu(screen, _theme, GOTOCLIENTMENU_TYPE,
                             L"Focus Client", "GOTOCLIENT");
    _menu_map["GOTOCLIENT"] = menu;
    menu = new FrameListMenu(screen, _theme, GOTOMENU_TYPE,
                             L"Focus Frame", "GOTO");
    _menu_map["GOTO"] = menu;
    menu = new FrameListMenu(screen, _theme, ICONMENU_TYPE,
                             L"Focus Iconified Frame", "ICON");
    _menu_map["ICON"] = menu;
    menu = new DecorMenu(screen, _theme, action_handler, "DECORMENU");
    _menu_map["DECORMENU"] = menu;
    menu =  new ActionMenu(ROOTMENU_TYPE, L"", "ROOTMENU");
    _menu_map["ROOT"] = menu;
    menu = new ActionMenu(WINDOWMENU_TYPE, L"", "WINDOWMENU");
    _menu_map["WINDOW"] = menu;

    // As the previous step is done manually, make sure it's done correct.
    assert(_menu_map.size() == (MENU_NAMES_RESERVED_COUNT - 2));

    // Load configuration, pass specific section to loading
    CfgParser menu_cfg;
    if (menu_cfg.parse(config->getMenuFile())
        || menu_cfg.parse (string(SYSCONFDIR "/menu"))) {
        _menu_state = menu_cfg.get_file_list();

        // Load standard menus
        map<string, PMenu*>::iterator it = _menu_map.begin();
        for (; it != _menu_map.end(); ++it) {
            it->second->reload(menu_cfg.get_entry_root()->find_section(it->second->getName()));
        }

        // Load standalone menus
        reloadStandaloneMenus(menu_cfg.get_entry_root());
    }
}

/**
 * (re)loads the menus in the menu configuration if the file has been
 * updated since last load.
 */
void
MenuHandler::reloadMenus(void)
{
    string menu_file(Config::instance()->getMenuFile());
    if (! Util::requireReload(_menu_state, menu_file)) {
        return;
    }

    // Load configuration, pass specific section to loading
    bool cfg_ok = true;
    CfgParser menu_cfg;
    if (! menu_cfg.parse(menu_file)) {
        if (! menu_cfg.parse(string(SYSCONFDIR "/menu"))) {
            cfg_ok = false;
        }
    }

    // Make sure menu is reloaded next time as content is dynamically
    // generated from the configuration file.
    if (! cfg_ok || menu_cfg.is_dynamic_content()) {
        _menu_state.clear();
    } else {
        _menu_state = menu_cfg.get_file_list();
    }

    // Update, delete standalone root menus, load decors on others
    map<string, PMenu*>::iterator it(_menu_map.begin());
    for (; it != _menu_map.end(); ++it) {
        if (it->second->getMenuType() == ROOTMENU_STANDALONE_TYPE) {
            delete it->second;
            _menu_map.erase(it);
        } else {
            // Only reload the menu if we got a ok configuration
            if (cfg_ok) {
                it->second->reload(menu_cfg.get_entry_root()->find_section(it->second->getName()));
            }
        }
    }

    // Update standalone root menus (name != ROOTMENU)
    reloadStandaloneMenus(menu_cfg.get_entry_root());
}

/**
 * Updates standalone root menus
 */
void
MenuHandler::reloadStandaloneMenus(CfgParser::Entry *section)
{
    // Temporary name, as names are stored uppercase
    string menu_name;

    // Go through all but reserved section names and create menus
    CfgParser::iterator it(section->begin());
    for (; it != section->end(); ++it) {
        // Uppercase name
        menu_name = (*it)->get_name();
        Util::to_upper(menu_name);

        // Create new menus, if the name is not reserved and not used
        if (! binary_search(MENU_NAMES_RESERVED,
                            MENU_NAMES_RESERVED + MENU_NAMES_RESERVED_COUNT,
                            menu_name, str_comparator)
                && ! getMenu(menu_name)) {
            // Create, parse and add to map
            PMenu *menu = new ActionMenu(ROOTMENU_STANDALONE_TYPE, L"", menu_name);
            menu->reload((*it)->get_section());
            _menu_map[menu->getName()] = menu;
        }
    }
}

/**
 * Clears the menu map and frees up resources used by menus
 */
void
MenuHandler::deleteMenus(void)
{
    map<std::string, PMenu*>::iterator it(_menu_map.begin());
    for (; it != _menu_map.end(); ++it) {
        delete it->second;
    }
    _menu_map.clear();
}

#endif // MENUS
