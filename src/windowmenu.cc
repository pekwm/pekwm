//
// windowmenu.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// windowmenu.cc for aewm++
// Copyright (C) 2000 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
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

#include "windowmanager.hh"

#include <stdio.h>

WindowMenu::WindowMenu(WindowManager *w) :
GenericMenu(w->getScreen(), w->getTheme(), "WindowMenu"),
wm(w)
{
	addToMenuList(this);

	m_send_to_menu = new BaseMenu(wm->getScreen(), wm->getTheme(), "Send To");
	addToMenuList(m_send_to_menu);

	// should do an initial update
	updateWindowMenu();
}

WindowMenu::~WindowMenu()
{
	removeFromMenuList(m_send_to_menu);
	delete m_send_to_menu;
}

void
WindowMenu::updateWindowMenu(void)
{
	removeAll();
	m_send_to_menu->removeAll();

	insert("(Un)Maximize", "", MAXIMIZE);
	insert("Iconify", "", ICONIFY);
	insert("(Un)Shade", "", SHADE);
	insert("(Un)Stick", "", STICK);
	insert("Raise", "", RAISE);
	insert("Lower", "", LOWER);
	insert("Always On Top", "", ALWAYS_ON_TOP);
	insert("Always Below", "", ALWAYS_BELOW);
	insert("Close", "", CLOSE);
	insert("Kill", "", KILL);

	char send_to[16]; // this should fit Desk and numbers up to 99999
	for(unsigned int i = 0; i < wm->getWorkspaces()->getNumWorkspaces(); ++i) {
		snprintf(send_to, 16, "Workspace %d", i + 1);
		m_send_to_menu->insert(send_to, i, SEND_TO_WORKSPACE);
	}
	m_send_to_menu->updateMenu();

	insert("Send To", m_send_to_menu);

	updateMenu();
}

void
WindowMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (!curr || !m_client)
		return;

	wm->getActionHandler()->handleAction(curr->getAction(), m_client);
}

#endif // MENUS
