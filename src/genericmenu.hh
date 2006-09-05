//
// genericmenu.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// genericmenu.hh
// Copyright (C) 2000 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
//
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

#include "basemenu.hh"

class GenericMenu : public BaseMenu
{
public:
	GenericMenu(ScreenInfo *s, Theme *t);
	virtual ~GenericMenu();

	BaseMenu* findMenu(Window w);

	inline void addToMenuList(BaseMenu *m) { m_menu_list.push_back(m); }
	void removeFromMenuList(BaseMenu *m);
public:
	std::vector<BaseMenu*> m_menu_list;	
};

#endif // MENUS
