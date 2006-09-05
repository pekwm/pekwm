//
// actionhandler.cc for pekwm
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

#include "actionhandler.hh"
#include "windowmanager.hh"
#include "frame.hh"
#include "util.hh"

using std::string;

ActionHandler::ActionHandler(WindowManager *w) :
wm(w)
{
}

ActionHandler::~ActionHandler()
{
}

void ActionHandler::handleAction(Action *action, Client *client)
{
	// if we have a frame, try to find an action that needs an frame/client
	if (client) {
		switch(action->action) {
		case MAXIMIZE:
			client->getFrame()->maximize();
			break;
		case MAXIMIZE_VERTICAL:
			client->getFrame()->maximizeVertical();
			break;
		case MAXIMIZE_HORIZONTAL:
			client->getFrame()->maximizeHorizontal();
			break;
		case RESIZE:
			client->getFrame()->doResize(false, true, false, true);
			break;
		case SHADE:
			client->getFrame()->shade();
			break;
		case ICONIFY:
			client->getFrame()->getActiveClient()->iconify();
			break;
		case ICONIFY_GROUP:
			client->getFrame()->iconifyAll();
			break;
		case STICK:
			client->getFrame()->getActiveClient()->stick();
			break;
		case CLOSE:
			client->getFrame()->getActiveClient()->close();
			break;
		case KILL:
			client->getFrame()->getActiveClient()->kill();
			break;
		case RAISE:
			client->getFrame()->raise();
			break;
		case LOWER:
			client->getFrame()->lower();
			break;
		case ALWAYS_ON_TOP:
			client->getFrame()->getActiveClient()->alwaysOnTop();
			break;
		case ALWAYS_BELOW:
			client->getFrame()->getActiveClient()->alwaysBelow();
			break;
		case TOGGLE_BORDER:
			client->toggleBorder();
			break;
		case TOGGLE_TITLEBAR:
			client->toggleTitlebar();
			break;
		case TOGGLE_DECOR:
			client->toggleDecor();
			break;

		case NUDGE_HORIZONTAL:
			client->getFrame()->move(client->getFrame()->getX() + action->i_param,
															 client->getFrame()->getY());
				break;
		case NUDGE_VERTICAL:
			client->getFrame()->move(client->getFrame()->getX(),
															 client->getFrame()->getY() + action->i_param);
			break;
		case RESIZE_HORIZONTAL:
			client->getFrame()->resizeHorizontal(action->i_param);
			break;
		case RESIZE_VERTICAL:
			client->getFrame()->resizeVertical(action->i_param);
			break;
		case MOVE_TO_CORNER:
			client->getFrame()->moveToCorner((Corner) action->i_param);
			break;
		case NEXT_IN_FRAME:
			client->getFrame()->activateNextClient();
			break;
		case PREV_IN_FRAME:
			client->getFrame()->activatePrevClient();
			break;
		case MOVE_CLIENT_NEXT:
			client->getFrame()->moveClientNext();
			break;
		case MOVE_CLIENT_PREV:
			client->getFrame()->moveClientPrev();
			break;
		case ACTIVATE_CLIENT_NUM:
			client->getFrame()->activateClientFromNum(action->i_param);
			break;
		case SEND_TO_WORKSPACE:
			client->getFrame()->setWorkspace(action->i_param);
			break;
#ifdef MENUS
		case SHOW_WINDOWMENU:
			client->getFrame()->showWindowMenu();
			break;
#endif // MENUS
		default:
			// do nothing
			break;
		}
	}

	switch (action->action) {
	case NEXT_FRAME:
		wm->focusNextFrame();
		break;
	case NEXT_WORKSPACE:
	case RIGHT_WORKSPACE:
		if ((wm->getActiveWorkspace() + 1) <
				wm->getWorkspaces()->getNumWorkspaces()) {
			wm->setWorkspace(wm->getActiveWorkspace() + 1, true);
		} else if (action->action == NEXT_WORKSPACE) {
			wm->setWorkspace(0, true);
		}
		break;

	case PREV_WORKSPACE:
	case LEFT_WORKSPACE:
		if (wm->getActiveWorkspace() > 0) {
			wm->setWorkspace(wm->getActiveWorkspace() - 1, true);
		} else if (action->action == PREV_WORKSPACE) {
			wm->setWorkspace(wm->getWorkspaces()->getNumWorkspaces() - 1, true);
		}
		break;
	case GO_TO_WORKSPACE:
		wm->setWorkspace(action->i_param, true);
		break;
	case EXEC:
		if (action->s_param.size())
			Util::forkExec(action->s_param);
		break;
#ifdef MENUS
	case SHOW_ROOTMENU:
		if(wm->getRootMenu()->isVisible())
			wm->getRootMenu()->hideAll();
		else if (wm->getRootMenu()->getItemCount())
			wm->getRootMenu()->showUnderMouse();

		wm->getIconMenu()->hideAll();
		wm->getWindowMenu()->hideAll();
		break;
	case SHOW_ICONMENU:
		if(wm->getIconMenu()->isVisible())
			wm->getIconMenu()->hideAll();
		else if (wm->getIconMenu()->getItemCount())
			wm->getIconMenu()->showUnderMouse();

		wm->getRootMenu()->hideAll();
		wm->getWindowMenu()->hideAll();
		break;
	case HIDE_ALL_MENUS:
		wm->getRootMenu()->hideAll();
		wm->getWindowMenu()->hideAll();
		wm->getIconMenu()->hideAll();
#ifdef HARBOUR
		wm->getHarbour()->getHarbourMenu()->hideAll();
#endif // HARBOUR
		break;
#endif //MENUS
	case RELOAD:
		wm->reload();
		break;
	case RESTART:
		wm->restart();
		break;
	case RESTART_OTHER:
		if (action->s_param.size())
			wm->restart(action->s_param);
		break;
	case EXIT:
		wm->quitNicely();
		break;
	default:
		// do nothing
		break;
	}
}
