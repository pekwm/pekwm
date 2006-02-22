//
// ActionHandler.hh for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
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
#include <list>

class Client;
class Frame;
class PWinObj;
class PDecor;
class PMenu;
class WindowManager;

class ActionHandler
{
public:
	ActionHandler(WindowManager *wm);
	~ActionHandler(void);

	static inline ActionHandler *instance(void) { return _instance; }

	void handleAction(const ActionPerformed &ap);

	static bool checkAEThreshold(int x, int y, int x_t, int y_t, uint t);
	static ActionEvent *findMouseAction(uint button, uint mod,
																			MouseEventType type,
																			std::list<ActionEvent> *actions);
private:
	void handleStateAction(const Action &action, PWinObj *wo,
												 Client *client, Frame *frame);

	void actionFindClient(const std::string &title);
	void actionWarpToWorkspace(PDecor *decor, uint direction);
	void actionWarpToViewport(PDecor *decor, uint direction);
	void actionFocusToggle(uint button, uint raise, int off, bool mru);
	void actionFocusDirectional(PWinObj *wo, DirectionType dir, bool raise);
#ifdef MENUS
	void actionShowMenu(MenuType m_type, bool stick, uint e_type, PWinObj *wo_ref);
#endif // MENUS

	// action helpers
	Client *findClientFromTitle(const std::string &title);

	PMenu *createNextPrevMenu(void);
	PMenu *createMRUMenu(void);

private:
	static ActionHandler *_instance;

	WindowManager *_wm;
};

#endif // _ACTIONHANDLER_HH_
