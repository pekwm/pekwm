//
// ActionMenu.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef MENUS

#include "WindowObject.hh"
#include "BaseMenu.hh"
#include "ActionMenu.hh"

#include "Config.hh"
#include "ActionHandler.hh"
#include "WindowManager.hh"

#include <cstdio>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::list;
using std::vector;

ActionMenu::ActionMenu(WindowManager *w, MenuType type, const string &name) :
BaseMenu(w->getScreen(), w->getTheme(), w->getWorkspaces()),
_wm(w),
_client(NULL), _insert_at(NULL),
_has_dynamic(false), _insert_dynamic(false)
{
	_cfg = _wm->getConfig();
	_act = _wm->getActionHandler();

	_name = name;
	_menu_type = type;

	if (_menu_type == WINDOWMENU_TYPE) {
		_action_ok = WINDOWMENU_OK;
	} else if (_menu_type == ROOTMENU_TYPE) {
		_action_ok = ROOTMENU_OK;
	}
}

ActionMenu::~ActionMenu()
{
	removeAll();
}

// START - WindowObject interface.

//! @fn    void mapWindow(void)
//! @brief Rebuilds the vmenu and if it has any items after it shows it.
void
ActionMenu::mapWindow(void)
{
	// find and rebuild the dynamic entries
	if (!isMapped() && _has_dynamic) {
		removeDynamic();
		rebuildDynamic();
		updateMenu();
	}

	if (size())
		BaseMenu::mapWindow();
}

//! @fn    void unmapWindow(void)
//! @brief Hides and removes all items created by dynamic entries.
void
ActionMenu::unmapWindow(void)
{
	if (!isMapped())
		return;

	// causes segfault as the entries get removed before they are executed
	// it seems, is my brain b0rked?
	//	if (_has_dynamic)
	//		removeDynamic();

	BaseMenu::unmapWindow();
}

// END - WindowObject interface.

//! @fn    void handleButton1Release(BaseMenuItem *curr)
//! @brief
void
ActionMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (!curr)
		return;

	ActionPerformed ap(&curr->ae, _client);
	_act->handleAction(ap);
}


//! @fn    void reload(void)
//! @brief
void
ActionMenu::reload(void)
{
	// clear the menu before loading
	removeAll();

	string name;
	if (_menu_type == WINDOWMENU_TYPE) {
		name = "WINDOWMENU";
	} else if (_menu_type == ROOTMENU_TYPE) {
		name = "ROOTMENU";
	}

	BaseConfig menu_cfg;
	if (menu_cfg.load(_cfg->getMenuFile())) { // try parsing the file
		parse(menu_cfg.getSection(name), this);
	} else if (menu_cfg.load(string(SYSCONFDIR "/menu"))) {
		parse(menu_cfg.getSection(name), this);
	}

	updateMenu();
}

//! @fn    void insert(BaseMenuItem *item)
//! @brief Checks if we have a position set for where to insert.
void
ActionMenu::insert(BaseMenuItem *item)
{
	if (!item)
		return;

	if (_insert_dynamic) {
		_insert_at = _item_list.insert(++_insert_at, item);
		item->dynamic = true;
	} else
		_item_list.push_back(item);
}

//! @fn    void remove(BaseMenuItem *item)
//! @brief Removes a BaseMenuItem from the menu.
void
ActionMenu::remove(BaseMenuItem *item)
{
	if (!item)
		return;

	vector<BaseMenuItem*>::iterator it =
		find(_item_list.begin(), _item_list.end(), item);

	if (it != _item_list.end()) {
		if (*it == _curr)
			_curr = NULL; // make sure we don't point anywhere dangerous

		if ((*it)->submenu)
			delete (*it)->submenu;
		delete *it;

		_item_list.erase(it);
	}
}

//! @fn    void removeAll(void)
//! @brief
void
ActionMenu::removeAll(void)
{
	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for (; it != _item_list.end(); ++it) {
		if ((*it)->submenu)
			delete (*it)->submenu;
		delete *it;
	}

	_item_list.clear();
	_curr = NULL; // make sure we don't point anywhere we shouldn't

	updateMenu();
}

//! @fn    void parse(BaseConfig::CfgSection *cs, bool dynamic)
//! @brief Parse config and push items into menu
//! @param cs CfgSection object to read config from
//! @param menu BaseMenu object to push object in
//! @param dynamic Defaults to false
void
ActionMenu::parse(BaseConfig::CfgSection *cs, bool dynamic)
{
	if (!cs)
		return;

	_has_dynamic = dynamic; // reset this

	ActionEvent ae;
	string name, value;
	ActionMenu *submenu = NULL;

	if (cs->getValue("NAME", name))
		setName(name);

	BaseConfig::CfgSection *sect;
	while ((sect = cs->getNextSection())) {
		sect->getValue("NAME", name);

		if (*sect == "SUBMENU") {
			// create a new submenu
			submenu = new ActionMenu(_wm, _menu_type, name);

			// parse and add to the newly created menu
			submenu->parse(sect);
			submenu->updateMenu();
			BaseMenu::insert(name, submenu);

			submenu = NULL; // make sure we don't do any icky

		} else if (sect->getValue("ACTIONS", value)) {
			if (_cfg->parseActions(value, ae, _action_ok)) {

				if (ae.isOnlyAction(DYNAMIC_MENU))
					_has_dynamic = true;

				BaseMenu::insert(name, ae);
			}
		}
	}
}

//! @fn    void rebuildDynamic(void)
//! @brief Executes all Dynamic entries in the menu.
void
ActionMenu::rebuildDynamic(void)
{
	BaseMenuItem* item = NULL;
	vector<BaseMenuItem*>::iterator it;
	for (it = _item_list.begin(); it != _item_list.end(); ++it) {
		if ((*it)->ae.isOnlyAction(DYNAMIC_MENU)) {
			_insert_dynamic = true;
			_insert_at = it;

			item = *it;

			BaseConfig dynamic;
			if (dynamic.loadCommand((*it)->ae.action_list.front().param_s))
				parse(dynamic.getSection("DYNAMIC"), true);

			// TO-DO: This shouldn't be needed.
			it = find(_item_list.begin(), _item_list.end(), item);
			_insert_dynamic = false;
		}
	}
}

//! @fn    void removeDynamic(void)
//! @brief Remove all entries from the menu created by dynamic entries.
void
ActionMenu::removeDynamic(void)
{
	vector<BaseMenuItem*>::iterator it = _item_list.begin();
	for (; it != _item_list.end(); ++it) {
		if ((*it)->dynamic) {
			if ((*it)->submenu)
				delete (*it)->submenu;
			delete (*it);

			it = _item_list.erase(it);
			--it; // compensate for the ++ in the loop
		}
	}
}

#endif // MENUS
