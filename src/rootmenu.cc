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
///

#ifdef MENUS
 
#include "pekwm.hh"
#include "windowmanager.hh"
#include "util.hh"

#include <iostream>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

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
	vector<BaseMenu*>::iterator it = m_submenu_list.begin();
	for (; it != m_submenu_list.end(); ++it) {
		removeFromMenuList(*it);
		delete *it;
	}
	m_submenu_list.clear();
}

void
RootMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (!curr)
		return;

	wm->getActionHandler()->handleAction(curr->getAction(), NULL);
}

void
RootMenu::updateRootMenu(void)
{
	// clear the menu before loading
	if (m_submenu_list.size()) {
		vector<BaseMenu*>::iterator it = m_submenu_list.begin();
		for (; it < m_submenu_list.end(); it++) {
			removeFromMenuList(*it);
			delete *it;
		}
		m_submenu_list.clear();
	}

	removeAll();

	BaseConfig cfg(wm->getConfig()->getMenuFile(), "*", ";\n");

	if (! cfg.loadConfig()) {
		string cfg_file = DATADIR "/menu";
		cfg.setFile(cfg_file);
		cfg.loadConfig();
	}

	if (cfg.isLoaded()) {
		parse(&cfg, this);
	} else { // couldn't open any menu config file
		cerr << "Couldn't open rootmenu config file: " <<
			wm->getConfig()->getMenuFile() << endl;

		insert("XTerm", "xterm", EXEC);
		insert("Restart", "", RESTART);
		insert("Exit", "", EXIT);
	}

	updateMenu();
}

//! @fn    void parse(BaseConfig *cfg, BaseMenu *menu)
//! @brief Parse config and push items into menu
//! @param cfg BaseConfig object to read config from
//! @param menu BaseMenu object to push object in
void
RootMenu::parse(BaseConfig *cfg, BaseMenu *menu)
{
	BaseMenu *sub = NULL;

	Actions action;

	string name, value;
	vector<string> values;

	while (cfg->getNextValue(name,value)) {
		action = wm->getConfig()->getAction(name, ROOTMENU_OK);

		switch (action) {
		case EXEC:
		case RESTART_OTHER:
			values.clear();
			if ((Util::splitString(value, values, ":", 2)) == 2) {
				menu->insert(values[0], values[1], action);
			}
			break;
		case RESTART:
		case RELOAD:
		case EXIT:
			menu->insert(value, "", action);
			break;
		case SUBMENU:
			// create a new submenu
			sub = new BaseMenu(wm->getScreen(), wm->getTheme());

			// add until we don't have anymore tokens or we find an end
			parse(cfg, sub);

			// add to menu search
			m_submenu_list.push_back(sub);
			addToMenuList(sub);

			// update the menu
			sub->updateMenu();

			// insert it
			menu->insert(value, sub);

			// make sure we don't do anything funny :)
			sub = NULL;
			break;
		case SUBMENU_END:
			return;
			break;
		default:
			// do nothing, invalid action
			break;
		}
	}
}

#endif // MENUS
