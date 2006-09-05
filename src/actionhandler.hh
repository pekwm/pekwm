//
// actionhandler.hh for pekwm
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

#ifndef _ACTIONHANDLER_HH_
#define _ACTIONHANDLER_HH_

#include "pekwm.hh"
#include <string>

class WindowManager;
class Client;


class ActionHandler
{
public:
	ActionHandler(WindowManager *w);
	~ActionHandler();

	void handleAction(Action *action, Client *client);

private:
	WindowManager *wm;
};

#endif // _ACTIONHANDLER_HH_
