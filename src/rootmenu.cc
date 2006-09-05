//
// rootmenu.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifdef MENUS

#include "pekwm.hh"
#include "windowmanager.hh"
#include "util.hh"

#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::list;

RootMenu::RootMenu(WindowManager *w) :
GenericMenu(w->getScreen(), w->getTheme()),
wm(w)
{
	addToMenuList(this);

	// should do an initial update
	updateRootMenu();
}

RootMenu::~RootMenu()
{
	list<BaseMenu*>::iterator it = m_submenu_list.begin();
	for (; it != m_submenu_list.end(); ++it) {
		removeFromMenuList(*it);
		delete *it;
	}
	m_submenu_list.clear();
}

//! @fn    void handleButton1Release(BaseMenuItem *curr)
//! @brief Executes the action of the current item.
void
RootMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (!curr)
		return;

	wm->getActionHandler()->handleAction(curr->getAction(), NULL);
}

//! @fn    void updateRootMenu(void)
//! @brief Rebuilds the RootMenu.
void
RootMenu::updateRootMenu(void)
{
	// clear the menu before loading
	if (m_submenu_list.size()) {
		list<BaseMenu*>::iterator it = m_submenu_list.begin();
		for (; it != m_submenu_list.end(); ++it) {
			removeFromMenuList(*it);
			delete *it;
		}
		m_submenu_list.clear();
	}

	removeAll();

	BaseConfig cfg;

	if (!cfg.load(wm->getConfig()->getMenuFile())) {
		if (!cfg.load(string(DATADIR "/menu"))) {
			cerr << "Couldn't open rootmenu config file: " <<
				wm->getConfig()->getMenuFile() << endl;

			setupEmergencyMenu();
		} else {
			parse(cfg.getSection("ROOTMENU"), this);
		}
	} else {
		parse(cfg.getSection("ROOTMENU"), this);
	}

	updateMenu();
}

//! @fn    void parse(BaseConfig::CfgSection *cs, BaseMenu *menu)
//! @brief Parse config and push items into menu
//! @param cs CfgSection object to read config from
//! @param menu BaseMenu object to push object in
void
RootMenu::parse(BaseConfig::CfgSection *cs, BaseMenu *menu)
{
	if (!cs || !menu)
		return;

	Config *cfg = wm->getConfig(); // convinience
	BaseConfig::CfgSection *sect;

	BaseMenu *sub = NULL; // temporary stuff used in parsing
	Actions action;
	string name, param;

	if (cs->getValue("NAME", name))
		menu->setName(name);

	while ((sect = cs->getNextSection())) {
		action = cfg->getAction(sect->getName(), ROOTMENU_OK);
		if (action == NO_ACTION)
			continue; // invalid action

		if (!sect->getValue("NAME", name))
			continue; // we need a name

		switch (action) {
		case EXEC:
		case RESTART_OTHER:
			if (sect->getValue("PARAM", param))
				menu->insert(name, param, action);
			break;
		case RESTART:
		case RELOAD:
		case EXIT:
			menu->insert(name, "", action);
			break;
		case SUBMENU:
			// create a new submenu
			sub = new BaseMenu(wm->getScreen(), wm->getTheme(), name);

			// add until we don't have anymore tokens or we find an end
			parse(sect, sub);

			// add to menu search
			m_submenu_list.push_back(sub);
			addToMenuList(sub);

			// update the menu
			sub->updateMenu();

			// insert it
			menu->insert(name, sub);

			// make sure we don't do anything funny :)
			sub = NULL;
			break;
		default:
			// do nothing, we shouldn't be able to get here
			break;
		}
	}
}

//! @fn    void setupEmergencyMenu(void)
//! @brief Builds a basic menu if we can't load the rootmenu config.
void
RootMenu::setupEmergencyMenu(void)
{
	setName("RootMenu");
	insert("XTerm", "xterm", EXEC);
	insert("Restart", "", RESTART);
	insert("Exit", "", EXIT);
}

#endif // MENUS
