//
// ActionHandler.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _ACTIONHANDLER_HH_
#define _ACTIONHANDLER_HH_

#include "pekwm.hh"
#include "Action.hh"

#include <string>

class WindowManager;

class ActionHandler
{
public:
	ActionHandler(WindowManager* wm);
	~ActionHandler();

	void handleAction(const ActionPerformed& ap);

private:
	WindowManager *_wm;
};

#endif // _ACTIONHANDLER_HH_
