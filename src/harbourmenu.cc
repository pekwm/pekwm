//
// harbourmenu.cc for pekwm
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

#ifdef HARBOUR
#ifdef MENUS

#include "harbourmenu.hh"
#include "pekwm.hh"

HarbourMenu::HarbourMenu(ScreenInfo *s, Theme *t, Harbour *h) :
GenericMenu(s, t, "Harbour"),
scr(s), theme(t),
harbour(h),
m_dockapp(NULL)
{
	addToMenuList(this);

	insert("Hide", "", ICONIFY);
	insert("Kill", "", CLOSE);

	updateMenu();
}

HarbourMenu::~HarbourMenu()
{
}

void
HarbourMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (!curr || !m_dockapp)
		return;

	switch(curr->getAction()->action) {
	case ICONIFY:
		//		m_dockapp->hide();
		break;
	case CLOSE:
		m_dockapp->kill();
		m_dockapp = NULL;
		break;
	default:
		// do nothing
		break;
	}
}

#endif // MENUS
#endif // HARBOUR
