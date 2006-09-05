//
// rootmenu.hh for pekwm
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

#ifndef _ROOTMENU_HH_
#define _ROOTMENU_HH_

#include "pekwm.hh"
#include "baseconfig.hh"

class RootMenu : public GenericMenu
{
public:
	RootMenu(WindowManager *w);
	virtual ~RootMenu();

	virtual void handleButton1Release(BaseMenuItem *curr);

	void updateRootMenu(void);
private:
	void handleAction(BaseMenuItem *item);
	void parse(BaseConfig *cfg, BaseMenu *menu);

	WindowManager *wm;

	std::vector<BaseMenu*> m_submenu_list;
};

#endif // _ROOTMENU_HH_

#endif // MENUS
