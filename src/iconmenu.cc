//
// iconmenu.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// iconmenu.cc for aewm++
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
#include "frame.hh"

#include <cstdio>

using std::string;
using std::vector;

IconMenu::IconMenu(WindowManager *w, ScreenInfo *s, Theme *t) :
GenericMenu(s, t, "IconMenu"),
wm(w)
{
	addToMenuList(this);
}

IconMenu::~IconMenu()
{
}

void
IconMenu::updateIconMenu(void)
{
	updateMenu();
}

void
IconMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (curr && (curr->getAction()->action == UNICONIFY)) {
		if (curr->getClient()->onWorkspace() != (signed) wm->getActiveWorkspace())
			curr->getClient()->setWorkspace(wm->getActiveWorkspace());

		curr->getClient()->getFrame()->unhideClient(curr->getClient());
	}
	hideAll();
}

void
IconMenu::addThisClient(Client *c)
{
	if (! c)
		return;

	char buf[16];

	string name = c->getClientIconName();
	snprintf(buf, sizeof(buf), " <%d>", c->onWorkspace() + 1);
	name.append(buf);

	BaseMenuItem *item = new BaseMenuItem();

	item->setClient(c);
	item->setName(name);
	item->getAction()->action = UNICONIFY;

	insert(item);
}

void
IconMenu::removeClientFromIconMenu(Client *c)
{
	if (!c)
		return;

	vector<BaseMenuItem*>::iterator it =  m_item_list.begin();
	for(; it != m_item_list.end(); ++it) {
		if((*it)->getClient() == c) {
			if (*it == m_curr)
				m_curr = NULL; // make sure we don't point anywhere dangerous

			delete *it;
			m_item_list.erase(it);
			break;
		}
	}
}

#endif // MENUS
