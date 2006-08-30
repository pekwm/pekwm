//
// ActionHandler.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "ActionHandler.hh"

#include "WindowObject.hh"
#include "Frame.hh"
#include "FrameWidget.hh"
#include "Client.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"
#include "Util.hh"

#ifdef HARBOUR
#include "Harbour.hh"
#endif // HARBOUR

#ifdef MENUS
#include "BaseMenu.hh"
#include "ActionMenu.hh"
#include "FrameListMenu.hh"
#ifdef HARBOUR
#include "HarbourMenu.hh"
#endif // HARBOUR
#endif // MENUS

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::list;

ActionHandler::ActionHandler(WindowManager* wm) :
_wm(wm)
{
}

ActionHandler::~ActionHandler()
{
}

//! @fn    void handleAction(const ActionPerformed &ap)
//! @brief Executes an ActionPerformed event.
void
ActionHandler::handleAction(const ActionPerformed& ap)
{
	Frame *f = ap.client ? ap.client->getFrame() : NULL; // convenience
#ifdef MENUS
	BaseMenu *menu = NULL;
#endif // MENUS

	list<Action>::iterator it = ap.ae->action_list.begin();
	for (; it != ap.ae->action_list.end(); ++it) {
		if (ap.client) {
			switch (it->action) {
			case MOVE:
				if (ap.type == MotionNotify)
					f->doMove(ap.event.motion);
				break;
			case GROUPING_DRAG:
				if (ap.type == MotionNotify)
					f->doGroupingDrag(ap.event.motion, ap.client, it->param_i);
				break;
			case ACTIVATE_CLIENT:
				if ((ap.type == ButtonPress) || (ap.type == ButtonRelease))
					f->activateClientFromPos(ap.event.button->x);
				break;
			case MAXIMIZE:
				f->maximize(true, true);
				break;
			case MAXIMIZE_HORIZONTAL:
				f->maximize(true, false);
				break;
			case MAXIMIZE_VERTICAL:
				f->maximize(false, true);
				break;
			case RESIZE:
				if (ap.type == MotionNotify)
					f->doResize(ap.event.motion);
				else
					f->doResize(false, true, false, true);
				break;
#ifdef KEYS
			case MOVE_RESIZE:
				f->doKeyboardMoveResize();
				break;
#endif // KEYS
			case SHADE:
				f->shade();
				break;
			case ICONIFY:
				f->iconify();
				break;
			case STICK:
				f->stick();
				break;
			case CLOSE:
				ap.client->close();
				break;
			case KILL:
				ap.client->kill();
				break;
			case RAISE:
				f->raise();
				break;
			case ACTIVATE_OR_RAISE:
				if (f->isFocused())
					f->raise();
				break;
			case LOWER:
				f->lower();
				break;
			case ALWAYS_ON_TOP:
				f->alwaysOnTop();
				break;
			case ALWAYS_BELOW:
				f->alwaysBelow();
				break;
			case TOGGLE_BORDER:
				f->toggleBorder();
				break;
			case TOGGLE_TITLEBAR:
				f->toggleTitlebar();
				break;
			case TOGGLE_DECOR:
				f->toggleDecor();
				break;
			case MOVE_TO_EDGE:
				f->moveToEdge((Edge) it->param_i);
				break;
			case NEXT_IN_FRAME:
				f->activateNextClient();
				break;
			case PREV_IN_FRAME:
				f->activatePrevClient();
				break;
			case MOVE_CLIENT_NEXT:
				f->moveClientNext();
				break;
			case MOVE_CLIENT_PREV:
				f->moveClientPrev();
				break;
			case ACTIVATE_CLIENT_NUM:
				f->activateClientFromNum(it->param_i);
				break;
			case SEND_TO_WORKSPACE:
				f->setWorkspace(it->param_i);
				break;
			case DETACH:
				f->detachClient(ap.client);
				break;
			case TOGGLE_TAG:
				if (f == _wm->getTaggedFrame())
					_wm->setTaggedFrame(NULL, true);
				else
					_wm->setTaggedFrame(f, true);
				break;
			case TOGGLE_TAG_BEHIND:
				if (f == _wm->getTaggedFrame())
					_wm->setTaggedFrame(NULL, true);
				else
					_wm->setTaggedFrame(f, false);
				break;
			case UNTAG:
				_wm->setTaggedFrame(NULL, true);
				break;
			case MARK_CLIENT:
				ap.client->mark();
				break;
			case ATTACH_MARKED:
				_wm->attachMarked(f);
				break;
			case ATTACH_CLIENT_IN_NEXT_FRAME:
				_wm->attachInNextPrevFrame(ap.client, false, true);
				break;
			case ATTACH_CLIENT_IN_PREV_FRAME:
				_wm->attachInNextPrevFrame(ap.client, false, false);
				break;
			case ATTACH_FRAME_IN_NEXT_FRAME:
				_wm->attachInNextPrevFrame(ap.client, true, true);
				break;
			case ATTACH_FRAME_IN_PREV_FRAME:
				_wm->attachInNextPrevFrame(ap.client, true, false);
				break;

			default:
				// do nothing
				break;
			}
		}

		switch (it->action) {
		case NEXT_FRAME:
			_wm->focusNextPrevFrame(ap.ae->sym, it->param_i, true);
			break;
		case PREV_FRAME:
			_wm->focusNextPrevFrame(ap.ae->sym, it->param_i, false);
			break;
		case NEXT_WORKSPACE:
		case RIGHT_WORKSPACE:
			if ((_wm->getWorkspaces()->getActive() + 1) <
					_wm->getWorkspaces()->getNumber()) {
				_wm->setWorkspace(_wm->getWorkspaces()->getActive() + 1, true);
			} else if (it->action == NEXT_WORKSPACE) {
				_wm->setWorkspace(0, true);
			}
			break;
		case PREV_WORKSPACE:
		case LEFT_WORKSPACE:
			if (_wm->getWorkspaces()->getActive() > 0) {
				_wm->setWorkspace(_wm->getWorkspaces()->getActive() - 1, true);
			} else if (it->action == PREV_WORKSPACE) {
				_wm->setWorkspace(_wm->getWorkspaces()->getNumber() - 1, true);
			}
			break;
		case GO_TO_WORKSPACE:
			_wm->setWorkspace(it->param_i, true);
			break;
		case EXEC:
			if (it->param_s.size())
				Util::forkExec(it->param_s);
			break;
#ifdef MENUS
		case SHOW_MENU:
			menu = _wm->getMenu(MenuType(it->param_i));
			if (menu) {
				if (menu->isMapped()) {
					menu->unmapAll();
				} else {
					_wm->hideAllMenus();
					if (menu->getMenuType() == WINDOWMENU_TYPE) {
						if (ap.client)
							ap.client->getFrame()->showWindowMenu();
					} else {
						menu->mapUnderMouse();
						menu->giveInputFocus();
					}
				}
			}
			break;
		case HIDE_ALL_MENUS:
			_wm->hideAllMenus();
			break;
#endif // MENUS
		case RELOAD:
			_wm->reload();

			// here's a bit of a special case, as I reload the list (ae)
			// changes and therefore I can no longer use it. 
			return;

			break;
		case RESTART:
			_wm->restart();
			break;
		case RESTART_OTHER:
			if (it->param_s.size())
				_wm->restart(it->param_s);
			break;
		case EXIT:
			_wm->quitNicely();
			break;
		default:
			// do nothing
			break;
		}
	}
}

