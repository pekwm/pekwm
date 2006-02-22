//
// ActionMenu.cc for pekwm
// Copyright (C) 2002-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef MENUS

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"
#include "ActionMenu.hh"

#include "Config.hh"
#include "ActionHandler.hh"
#include "Client.hh"
#include "Frame.hh"
#include "WindowManager.hh"

#include <cstdio>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::list;
using std::find;

//! @brief ActionMenu constructor
//! @param wm Pointer to WindowManager
//! @param type Type of menu
//! @param name Title of menu, defaults to beeing empty
ActionMenu::ActionMenu(WindowManager *wm, MenuType type, const std::string &name) :
WORefMenu(wm->getScreen(), wm->getTheme(), name),
_wm(wm), _act(wm->getActionHandler()),
_has_dynamic(false)
{
	// when creating dynamic submenus, this needs to be initialized as
	// dynamic inserting will be done
	_insert_at = _item_list.begin();
	_menu_type = type;

	if (_menu_type == WINDOWMENU_TYPE) {
		_action_ok = WINDOWMENU_OK;
	} else if (_menu_type == ROOTMENU_TYPE) {
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
	if ((isMapped() == false) && (_has_dynamic == true)) {
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
	if (isMapped() == false) {
		return;
	}

	// causes segfault as the entries get removed before they are executed
	// it seems, is my brain b0rked?
	if (_has_dynamic == true) {
		removeDynamic();
	}

	PMenu::unmapWindow();
}

// END - PWinObj interface.

//! @brief Handle item exec.
void
ActionMenu::handleItemExec(PMenu::Item *item)
{
	if (item == NULL) {
		return;
	}

	ActionPerformed ap(_wo_ref, item->getAE());
	_act->handleAction(ap);
}


//! @brief Re-reads the configuration
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

	CfgParser menu_cfg;
	if (menu_cfg.parse (Config::instance()->getMenuFile())) {
		parse (menu_cfg.get_entry_root ()->find_section (name));
	} else if (menu_cfg.parse (string(SYSCONFDIR "/menu"))) {
		parse (menu_cfg.get_entry_root ()->find_section (name));
	}

	buildMenu();
}

//! @brief Checks if we have a position set for where to insert.
void
ActionMenu::insert(PMenu::Item *item)
{
	if (item == NULL) {
		return;
	}

	checkItemWORef(item);

	ActionMenu::DItem *d_item = static_cast<ActionMenu::DItem*>(item);
	if (d_item->isDynamic()) {
		_insert_at = _item_list.insert(++_insert_at, item);
	} else {
		_item_list.push_back(item);
	}
}

//! @brief Non-shadowing PMenu::insert
void
ActionMenu::insert(const std::string &or_name, PWinObj *op_wo_ref)
{
  PMenu::insert (or_name, op_wo_ref);
}

//! @brief Non-shadowing PMenu::insert
void
ActionMenu::insert(const std::string &or_name, const ActionEvent &or_ae,
                   PWinObj *op_wo_ref)
{
  PMenu::insert (or_name, or_ae, op_wo_ref);
}

//! @brief Removes a BaseMenuItem from the menu
void
ActionMenu::remove(PMenu::Item *item)
{
	if (item == NULL) {
		return;
	}

	if ((item->getWORef() != NULL) && (item->getWORef()->getType() == WO_MENU)) {
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
//! @param dynamic Defaults to false
void
ActionMenu::parse(CfgParser::Entry *op_section, bool dynamic)
{
  if (!op_section)
      return;

  _has_dynamic = dynamic; // reset this

  CfgParser::Entry *op_sub, *op_value;
  ActionEvent ae;

  ActionMenu *submenu = NULL;
  ActionMenu::DItem *item = NULL;

  if (op_section->get_value ().size ())
    {
      setTitle (op_section->get_value ());
      _title_base = op_section->get_value ();
    }
  op_section = op_section->get_section ();

  while ((op_section = op_section->get_section_next ()) != NULL)
    {
      item = NULL;
      op_sub = op_section->get_section ();

      if (*op_section == "SUBMENU")
        {
          submenu = new ActionMenu (_wm, _menu_type, op_section->get_value ());
          submenu->parse (op_section, dynamic);
          submenu->buildMenu ();

          item = new ActionMenu::DItem (dynamic, op_sub->get_value (), submenu);
        }
      else if (*op_section == "SEPARATOR")
        {
          item = new ActionMenu::DItem (dynamic, "");
          item->setType (PMenu::Item::MENU_ITEM_SEPARATOR);
        }
      else
        {
          op_value = op_section->get_section ()->find_entry ("ACTIONS");
          if (op_value
              && Config::instance ()->parseActions (op_value->get_value (),
                                                    ae, _action_ok))
            {
              item = new ActionMenu::DItem (dynamic, op_sub->get_value ());
              item->setAE (ae);

              if (ae.isOnlyAction (ACTION_MENU_DYN))
                {
                  _has_dynamic = true;
                  item->setType (PMenu::Item::MENU_ITEM_HIDDEN);
                }
            }
        }

      // If an item was successfully created, insert it to the menu.
      if (item != NULL)
        {
          ActionMenu::insert (item);
        }
    }
}

//! @brief Executes all Dynamic entries in the menu.
void
ActionMenu::rebuildDynamic(void)
{
	PMenu::Item* item = NULL;
	list<PMenu::Item*>::iterator it;
	for (it = _item_list.begin(); it != _item_list.end(); ++it) {
		if ((*it)->getAE().isOnlyAction(ACTION_MENU_DYN)) {
			_insert_at = it;

			item = *it;

			CfgParser dynamic;
			if (dynamic.parse ((*it)->getAE().action_list.front().getParamS(),
                         CfgParserSource::SOURCE_COMMAND))
        {
          parse (dynamic.get_entry_root ()->find_section ("DYNAMIC"), true);
        }

			it = find(_item_list.begin(), _item_list.end(), item);
		}
	}
}

//! @brief Remove all entries from the menu created by dynamic entries.
void
ActionMenu::removeDynamic(void)
{
	ActionMenu::DItem* item;
	list<PMenu::Item*>::iterator it(_item_list.begin());
	for (; it != _item_list.end(); ++it) {
		item = static_cast<ActionMenu::DItem*>(*it);
		if (item->isDynamic()) {
			if (((*it)->getWORef() != NULL) &&
			    ((*it)->getWORef()->getType() == WO_MENU)) {
				delete (*it)->getWORef();
			}
			delete (*it);

			it = _item_list.erase(it);
			--it; // compensate for the ++ in the loop
		}
	}
}

#endif // MENUS
