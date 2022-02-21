//
// ActionHandler.cc for pekwm
// Copyright (C) 2002-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "ActionHandler.hh"

#include "Charset.hh"
#include "Debug.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "Frame.hh"
#include "Client.hh"
#include "ClientMgr.hh"
#include "Config.hh"
#include "CmdDialog.hh"
#include "SearchDialog.hh"
#include "Workspaces.hh"
#include "Util.hh"
#include "RegexString.hh"
#include "WorkspaceIndicator.hh"
#include "Harbour.hh"
#include "MenuHandler.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"
#include "ActionMenu.hh"
#include "FrameListMenu.hh"
#include "ManagerWindows.hh"
#include "X11Util.hh"
#include "X11.hh"

#include "FocusToggleEventHandler.hh"
#include "GroupingDragEventHandler.hh"
#include "MoveEventHandler.hh"
#include "KeyboardMoveResizeEventHandler.hh"

#include <memory>

ActionHandler::ActionHandler(AppCtrl* app_ctrl, EventLoop* event_loop)
	: _app_ctrl(app_ctrl),
	  _event_loop(event_loop)
{
	// Initialize state_to_keycode map
	for (uint i = 0; i < X11::MODIFIER_TO_MASK_NUM; ++i) {
		uint modifier = X11::MODIFIER_TO_MASK[i];
		_state_to_keycode[modifier] = X11::getKeycodeFromMask(modifier);
	}
}

//! @brief ActionHandler destructor
ActionHandler::~ActionHandler(void)
{
}

//! @brief Executes an ActionPerformed event.
void
ActionHandler::handleAction(const ActionPerformed &ap)
{
	PWinObj *wo = ap.wo;
	Client *client = nullptr;
	Frame *frame = nullptr;
	PMenu *menu = nullptr;
	PDecor *decor = nullptr;
	bool matched = false;

	// go through the list of actions and execute them
	ActionEvent::it it = ap.ae.action_list.begin();
	for (; it != ap.ae.action_list.end(); ++it, matched = false) {
		// Determine what type if any of the window object that is focused
		// and check if it is still alive.
		lookupWindowObjects(&wo, &client, &frame, &menu, &decor);
		P_TRACE("start action " << it->getAction() << " wo " << wo);

		// actions valid for all PWinObjs
		if (! matched && wo) {
			matched = true;
			switch (it->getAction()) {
			case ACTION_FOCUS:
				if (wo->isFocusable()) {
					wo->giveInputFocus();
				}
				break;
			case ACTION_UNFOCUS:
				PWinObj::getRootPWinObj()->giveInputFocus();
				break;
			case ACTION_FOCUS_DIRECTIONAL:
				actionFocusDirectional(wo, DirectionType(it->getParamI(0)),
						       it->getParamI(1));
				break;
			case ACTION_SEND_KEY:
				actionSendKey(wo, it->getParamS());
				break;
			default:
				matched = false;
				break;
			};
		}

		// actions valid for Clients and Frames
		if (! matched && frame) {
			matched = true;
			switch (it->getAction()) {
			case ACTION_GROUPING_DRAG:
				actionGroupingDrag(ap, frame, client,
						   it->getParamI(0));
				break;
			case ACTION_ACTIVATE_CLIENT:
				if ((ap.type == ButtonPress) || (ap.type == ButtonRelease))
					frame->activateChild(frame->getChildFromPos(ap.event.button->x));
				break;
			case ACTION_GOTO_CLIENT:
				gotoClient(client);
				break;
			case ACTION_MAXFILL:
				frame->setStateMaximized(STATE_SET,
							 it->getParamI(0), it->getParamI(1), true);
				break;
			case ACTION_GROW_DIRECTION:
				frame->growDirection(it->getParamI(0));
				break;
			case ACTION_RESIZE:
				if (ap.type == MotionNotify && ! it->getParamI(0)) {
					frame->doResize(ap.event.motion);
				} else {
					frame->doResize(BorderPosition(it->getParamI(0)-1));
				}
				break;
			case ACTION_MOVE_RESIZE: {
				EventHandler *event_handler =
					new KeyboardMoveResizeEventHandler(pekwm::config(),
									   pekwm::keyGrabber(),
									   decor);
				setEventHandler(event_handler);
				break;
			}
			case ACTION_CLOSE:
				client->close();
				break;
			case ACTION_CLOSE_FRAME:
				frame->close();
				break;
			case ACTION_KILL:
				client->kill();
				break;
			case ACTION_SET_GEOMETRY:
				frame->setGeometry(it->getParamS(),
						   it->getParamI(0), it->getParamI(1) == 1);
				break;
			case ACTION_RAISE:
				if (it->getParamI(0)) {
					ClientMgr::familyRaiseLower(client, true);
				} else {
					frame->raise();
				}
				break;
			case ACTION_LOWER:
				if (it->getParamI(0)) {
					ClientMgr::familyRaiseLower(client, false);
				} else {
					frame->lower();
				}
				break;
			case ACTION_ACTIVATE_OR_RAISE:
				if ((ap.type == ButtonPress) || (ap.type == ButtonRelease)) {
					if (ap.event.button->window == frame->getTitleWindow()) {
						frame->activateChild(frame->getChildFromPos(ap.event.button->x));
					}
				}

				if (frame->isFocused()) {
					frame->raise();
				} else {
					wo->giveInputFocus();
				}
				break;
			case ACTION_MOVE_TO_HEAD:
				frame->moveToHead(it->getParamS(0));
				break;
			case ACTION_MOVE_TO_EDGE:
				frame->moveToEdge(OrientationType(it->getParamI(0)));
				break;
			case ACTION_ACTIVATE_CLIENT_REL:
				frame->activateChildRel(it->getParamI(0));
				break;
			case ACTION_MOVE_CLIENT_REL:
				frame->moveChildRel(it->getParamI(0));
				break;
			case ACTION_ACTIVATE_CLIENT_NUM:
				frame->activateChildNum(it->getParamI(0));
				break;
			case ACTION_SEND_TO_WORKSPACE:
				actionSendToWorkspace(decor, it->getParamI(0), calcWorkspaceNum(*it, 1));
				break;
			case ACTION_DETACH:
				frame->detachClient(client,
						    frame->getX(),
						    frame->getY());
				break;
			case ACTION_ATTACH_MARKED:
				attachMarked(frame);
				break;
			case ACTION_ATTACH_CLIENT_IN_NEXT_FRAME:
				attachInNextPrevFrame(client, false, true);
				break;
			case ACTION_ATTACH_CLIENT_IN_PREV_FRAME:
				attachInNextPrevFrame(client, false, false);
				break;
			case ACTION_ATTACH_FRAME_IN_NEXT_FRAME:
				attachInNextPrevFrame(client, true, true);
				break;
			case ACTION_ATTACH_FRAME_IN_PREV_FRAME:
				attachInNextPrevFrame(client, true, false);
				break;
			case ACTION_SET_OPACITY:
				actionSetOpacity(client, frame,
						 it->getParamI(0), it->getParamI(1));
				break;
			default:
				matched = false;
				break;
			}
		}

		// Actions valid for Menus
		if (! matched && menu) {
			matched = true;
			switch (it->getAction()) {
				// menu navigation
			case ACTION_MENU_NEXT:
				menu->selectNextItem();
				break;
			case ACTION_MENU_PREV:
				menu->selectPrevItem();
				break;
			case ACTION_MENU_GOTO:
				menu->selectItemNum(it->getParamI(0));
				break;
			case ACTION_MENU_SELECT:
			case ACTION_MENU_ENTER_SUBMENU:
				if (menu->getItemCurr() &&
				    menu->getItemCurr()->getWORef() &&
				    (menu->getItemCurr()->getWORef()->getType() == PWinObj::WO_MENU)) {
					menu->mapSubmenu(static_cast<PMenu*>(menu->getItemCurr()->getWORef()), true);
				} else if (it->getAction() == ACTION_MENU_SELECT) {
					menu->exec(menu->getItemCurr());

					// special case: execItem can cause an reload to be issued, if that's
					// the case it causes the list (ae) to change and therefore
					// it can't be used anymore
					return;
				}
				break;
			case ACTION_MENU_LEAVE_SUBMENU:
				menu->gotoParentMenu();
				break;
			case ACTION_CLOSE:
				menu->unmapAll();
				Workspaces::findWOAndFocus(nullptr);
				break;
			default:
				matched = false;
				break;
			}
		}
		// actions valid for pdecor
		if (! matched && decor) {
			matched = true;
			switch (it->getAction()) {
			case ACTION_MOVE: {
				int x_root, y_root;

				if (it->numParamI() == 2) {
					x_root = it->getParamI(0);
					y_root = it->getParamI(1);
				} else {
					// Get root position, previously used event
					// position but was error prone on Xinerama setups
					X11::getMousePosition(x_root, y_root);
				}
				EventHandler *event_handler =
					new MoveEventHandler(pekwm::config(), decor,
							     x_root, y_root);
				setEventHandler(event_handler);
				break;
			}
			case ACTION_CLOSE:
				decor->unmapWindow();
				if (decor->getType() == PWinObj::WO_CMD_DIALOG
				    || decor->getType() == PWinObj::WO_SEARCH_DIALOG) {
					Workspaces::findWOAndFocus(nullptr);
				}
				break;
			case ACTION_WARP_TO_WORKSPACE:
				actionWarpToWorkspace(decor, calcWorkspaceNum(*it));
				break;
			default:
				matched = false;
				break;
			}
		}

		// Actions valid from everywhere
		if (! matched) {
			matched = true;
			switch (it->getAction()) {
			case ACTION_FOCUS_WITH_SELECTOR:
				actionFocusWithSelector(*it);
				break;
			case ACTION_SET:
			case ACTION_UNSET:
			case ACTION_TOGGLE:
				handleStateAction(*it, wo, client, frame);
				break;
			case ACTION_NEXT_FRAME:
			case ACTION_NEXT_FRAME_MRU:
			case ACTION_PREV_FRAME:
			case ACTION_PREV_FRAME_MRU: {
				bool mru = it->getAction() == ACTION_NEXT_FRAME_MRU
					|| it->getAction() == ACTION_PREV_FRAME_MRU;
				int dir = (it->getAction() == ACTION_NEXT_FRAME
					   || it->getAction() == ACTION_NEXT_FRAME_MRU) ? 1 : -1;
				EventHandler *event_handler =
					new FocusToggleEventHandler(pekwm::config(),
								    ap.ae.sym, it->getParamI(0),
								    dir, it->getParamI(1), mru);
				setEventHandler(event_handler);
				break;
			}
			case ACTION_GOTO_WORKSPACE:
				actionGotoWorkspace(*it, ap.type);
				break;
			case ACTION_FIND_CLIENT:
				actionFindClient(it->getParamS());
				break;
			case ACTION_GOTO_CLIENT_ID:
				actionGotoClientID(it->getParamI(0));
				break;
			case ACTION_EXEC:
				actionExec(client, it->getParamS(), false);
				break;
			case ACTION_SHELL_EXEC:
				actionExec(client, it->getParamS(), true);
				break;
			case ACTION_SHOW_MENU:
				actionShowMenu(it->getParamS(), it->getParamI(0),
					       ap.type, client ? client : wo);
				break;
			case ACTION_HIDE_ALL_MENUS:
				MenuHandler::hideAllMenus();
				break;
			case ACTION_RELOAD:
				_app_ctrl->reload();
				break;
			case ACTION_RESTART:
				_app_ctrl->restart();
				break;
			case ACTION_RESTART_OTHER:
				if (it->getParamS().size())
					_app_ctrl->restart(it->getParamS());
				break;
			case ACTION_EXIT:
				_app_ctrl->shutdown();
				break;
			case ACTION_SHOW_CMD_DIALOG:
				actionShowInputDialog(&_cmd_dialog, it->getParamS(),
						      frame, wo);
				break;
			case ACTION_SHOW_SEARCH_DIALOG:
				actionShowInputDialog(&_search_dialog, it->getParamS(),
						      frame, wo);
				break;
			case ACTION_DEBUG:
				Debug::doAction(it->getParamS());
				break;
			case ACTION_WARP_POINTER:
				actionWarpPointer(it->getParamI(0), it->getParamI(1));
				break;
			default:
				matched = false;
				break;
			}
		}

		P_TRACE("end action " << it->getAction() << " wo " << wo);
	}
}

void
ActionHandler::lookupWindowObjects(PWinObj **wo, Client **client, Frame **frame,
                                   PMenu **menu, PDecor **decor)
{
	*client = 0;
	*frame = 0;
	*menu = 0;
	*decor = 0;

	if (PWinObj::windowObjectExists(*wo)) {
		if ((*wo)->getType() == PWinObj::WO_CLIENT) {
			*client = static_cast<Client*>(*wo);
			*frame = static_cast<Frame*>((*client)->getParent());
			*decor = static_cast<PDecor*>(*frame);
		} else if ((*wo)->getType() == PWinObj::WO_FRAME) {
			*frame = static_cast<Frame*>(*wo);
			*client = static_cast<Client*>((*frame)->getActiveChild());
			*decor = static_cast<PDecor*>(*wo);
		} else if ((*wo)->getType() == PWinObj::WO_MENU) {
			*menu = static_cast<PMenu*>(*wo);
			*decor = static_cast<PDecor*>(*wo);
		} else {
			*decor = dynamic_cast<PDecor*>(*wo);
		}
	} else {
		*wo = 0;
	}
}

//! @brief Handles state actions
void
ActionHandler::handleStateAction(const Action &action, PWinObj *wo,
                                 Client *client, Frame *frame)
{
	StateAction sa = static_cast<StateAction>(action.getAction()); // convenience

	bool matched = false;

	// check for frame actions
	if (! matched && frame) {
		matched = true;
		switch (action.getParamI(0)) {
		case ACTION_STATE_MAXIMIZED:
			frame->setStateMaximized(sa, action.getParamI(1),
						 action.getParamI(2), false);
			break;
		case ACTION_STATE_FULLSCREEN:
			frame->setStateFullscreen(sa);
			break;
		case ACTION_STATE_SHADED:
			frame->setShaded(sa);
			break;
		case ACTION_STATE_STICKY:
			frame->setStateSticky(sa);
			break;
		case ACTION_STATE_ALWAYS_ONTOP:
			frame->setStateAlwaysOnTop(sa);
			break;
		case ACTION_STATE_ALWAYS_BELOW:
			frame->setStateAlwaysBelow(sa);
			break;
		case ACTION_STATE_DECOR_BORDER:
			frame->setStateDecorBorder(sa);
			break;
		case ACTION_STATE_DECOR_TITLEBAR:
			frame->setStateDecorTitlebar(sa);
			break;
		case ACTION_STATE_ICONIFIED:
			frame->setStateIconified(sa);
			break;
		case ACTION_STATE_TAGGED:
			frame->setStateTagged(sa, action.getParamI(1));
			break;
		case ACTION_STATE_MARKED:
			frame->setStateMarked(sa, client);
			break;
		case ACTION_STATE_SKIP:
			frame->setStateSkip(sa, action.getParamI(1));
			break;
		case ACTION_STATE_CFG_DENY:
			client->setStateCfgDeny(sa, action.getParamI(1));
			break;
		case ACTION_STATE_DECOR:
			frame->setDecorOverride(sa, action.getParamS());
			break;
		case ACTION_STATE_TITLE:
			frame->setStateTitle(sa, client, action.getParamS());
			break;
		case ACTION_STATE_OPAQUE:
			frame->setStateOpaque(sa);
			break;
		default:
			matched = false;
			break;
		}
	}

	// check for menu actions
	if (! matched && wo &&
	    (wo->getType() == PWinObj::WO_MENU)) {
		matched = true;
		switch (action.getParamI(0)) {
		case ACTION_STATE_STICKY:
			wo->stick();
			break;
		default:
			matched = false;
			break;
		}
	}

	if (! matched) {
		matched = true;
		switch (action.getParamI(0)) {
		case ACTION_STATE_HARBOUR_HIDDEN:
			pekwm::harbour()->setStateHidden(sa);
			break;
		case ACTION_STATE_GLOBAL_GROUPING:
			ClientMgr::setStateGlobalGrouping(sa);
			break;
		default:
			matched = false;
			break;
		}
	}
}

//! @brief Checks if motion threshold is within bounds.
bool
ActionHandler::checkAEThreshold(int x, int y, int x_t, int y_t, uint t)
{
	if (((x > x_t) ? (x > (x_t + signed(t))) : (x < (x_t - signed(t)))) ||
	    ((y > y_t) ? (y > (y_t + signed(t))) : (y < (y_t - signed(t))))) {
		return true;
	}
	return false;
}

//! @brief Searches the actions list for an matching event
ActionEvent*
ActionHandler::findMouseAction(uint button, uint state, MouseEventType type,
                               std::vector<ActionEvent> *actions)
{
	if (! actions) {
		return 0;
	}

	X11::stripStateModifiers(&state);
	X11::stripButtonModifiers(&state);

	std::vector<ActionEvent>::iterator it = actions->begin();
	for (; it != actions->end(); ++it) {
		if ((it->type == unsigned(type))
		    && ((it->mod == MOD_ANY) || (it->mod == state))
		    && ((it->sym == BUTTON_ANY) || (it->sym == button))) {
			return &*it;
		}
	}

	return 0;
}

/**
 * Execute action, setting client environment before (if any).
 */
void
ActionHandler::actionExec(Client *client, const std::string &command,
                          bool use_shell)
{
	if (! command.size()) {
		USER_WARN("empty Exec/ShellExec command");
		return;
	}

	Client::setClientEnvironment(client);
	if (use_shell) {
		Util::forkExec(command);
	} else {
		std::vector<std::string> args = StringUtil::shell_split(command);
		Util::forkExec(args);
	}
}

//! @brief Searches for a client matching titles and makes it visible
void
ActionHandler::actionFindClient(const std::string &title)
{
	Client *client = findClientFromTitle(title);
	if (client) {
		gotoClient(client);
	}
}

//! @brief Goto workspace
//! @param workspace Workspace to got to
//! @param warp If true, warp pointer as well
void
ActionHandler::actionGotoWorkspace(const Action &action, int type)
{

	uint workspace = calcWorkspaceNum(action);
	bool focus = action.getParamI(action.numParamI() - 1);
	// events caused by a motion event ( dragging frame to the
	// edge ) or enter event ( moving the pointer to the edge )
	// should warp the pointer.
	bool warp = type == MotionNotify || type == EnterNotify;
	Workspaces::gotoWorkspace(workspace, focus, warp);
}

//! @brief Focus client with id.
//! @param id Client id.
void
ActionHandler::actionGotoClientID(uint id)
{
	Client *client = Client::findClientFromID(id);
	if (client) {
		gotoClient(client);
	}
}

/**
 * Initiate grouping drag.
 */
void
ActionHandler::actionGroupingDrag(const ActionPerformed &ap,
				  Frame *frame, Client *client, bool behind)
{
	if (ap.type != MotionNotify) {
		return;
	}

	int x = ap.event.motion->x_root;
	int y = ap.event.motion->y_root;
	EventHandler *event_handler =
		new GroupingDragEventHandler(frame, client, x, y, behind);
	setEventHandler(event_handler);
}

//! @brief Sends client to specified workspace
//! @param focus make the window "last focused" in the new workspace
//! @param direction Workspace to send to, or special meaning relative space.
void
ActionHandler::actionSendToWorkspace(PDecor *decor, bool focus, int direction)
{
	// Convenience
	const uint per_row = Workspaces::getPerRow();
	const uint cur_row = Workspaces::getRow();
	const uint cur_act = Workspaces::getActive();
	const uint row_min = Workspaces::getRowMin();
	const uint row_max = Workspaces::getRowMax();
	const uint udirection = static_cast<uint>(direction);

	// Initialized to silence compiler warning
	uint new_workspace = cur_act;

	switch (static_cast<WorkspaceChangeType>(direction)) {
	case WORKSPACE_LEFT:
	case WORKSPACE_PREV:
		if (cur_act > row_min) {
			new_workspace = cur_act - 1;
		} else if (udirection == WORKSPACE_PREV) {
			new_workspace = row_max;
		}
		break;
	case WORKSPACE_NEXT:
	case WORKSPACE_RIGHT:
		if (cur_act < row_max) {
			new_workspace = cur_act + 1;
		} else if (udirection == WORKSPACE_NEXT) {
			new_workspace = row_min;
		}
		break;
	case WORKSPACE_PREV_V:
	case WORKSPACE_UP:
		if (cur_act >= per_row) {
			new_workspace = cur_act - per_row;
		} else if (udirection == WORKSPACE_PREV_V) {
			// Bottom left + column
			new_workspace = Workspaces::size() - per_row
				+ cur_act - cur_row * per_row;
		}
		break;
	case WORKSPACE_NEXT_V:
	case WORKSPACE_DOWN:
		if ((cur_act + per_row) < Workspaces::size()) {
			new_workspace = cur_act + per_row;
		} else if (udirection == WORKSPACE_NEXT_V) {
			new_workspace = cur_act - cur_row * per_row;
		}
		break;
	case WORKSPACE_LAST:
		new_workspace = Workspaces::getPrevious();
		break;
	default:
		new_workspace = direction;
		break;
	}

	decor->setWorkspace(new_workspace);
	if (focus) {
		Workspaces::setLastFocused(new_workspace, decor);
	}
}

void
ActionHandler::actionWarpToWorkspace(PDecor *decor, uint direction)
{
	// Removing the already accumulated motion events can help
	// to avoid skipping workspaces (see task #77).
	X11::removeMotionEvents();

	// actually did move
	if (Workspaces::gotoWorkspace(DirectionType(direction), false, true)) {
		int x, y;
		X11::getMousePosition(x, y);

		decor->move(decor->getClickX() + x - decor->getPointerX(),
			    decor->getClickY() + y - decor->getPointerY());
		decor->setWorkspace(Workspaces::getActive());
	}
}

/**
 * Find window object under the pointer and give it focus, if none is
 * found use fallback.
 */
void
ActionHandler::actionFocusWithSelector(const Action& action)
{
	PWinObj *wo_focus = nullptr;
	for (size_t i = 0; i < action.numParamI() && ! wo_focus; i++) {
		switch (action.getParamI(i)) {
		case FOCUS_SELECTOR_POINTER:
			wo_focus = Workspaces::findUnderPointer();
			break;
		 case FOCUS_SELECTOR_WORKSPACE_LAST_FOCUSED: {
			int active = Workspaces::getActive();
			wo_focus = Workspaces::getLastFocused(active);
			break;
		}
		case FOCUS_SELECTOR_TOP: {
			int mask = PWinObj::WO_FRAME
			  |PWinObj::WO_MENU
			  |PWinObj::WO_CMD_DIALOG
			  |PWinObj::WO_SEARCH_DIALOG;
			wo_focus = Workspaces::getTopFocusableWO(mask);
			break;
		}
		case FOCUS_SELECTOR_ROOT:
			wo_focus = pekwm::rootWo();
			break;
		default:
			// should not happen, validated during parse
			break;
		}
	}

	if (wo_focus != nullptr) {
		wo_focus->giveInputFocus();
	}
}

//! @brief Finds a window in direction and gives it focus
void
ActionHandler::actionFocusDirectional(PWinObj *wo, DirectionType dir, bool raise)
{
	PWinObj *wo_focus =
		Workspaces::findDirectional(wo, dir, SKIP_FOCUS_TOGGLE);

	if (wo_focus) {
		if (raise) {
			wo_focus->raise();
		}
		wo_focus->giveInputFocus();
	}
}

/**
 * Parse key information and send to wo.
 *
 * To handle this smoothly first all state modifiers are pressed one
 * by one adding to the state, then the real key is pressed and
 * released, then the state modifiers are released.
 *
 * @param win Window to send key to.
 * @param key_str String definition of key to send.
 * @return true if key was parsed ok, no validation of sending is done.
 */
bool
ActionHandler::actionSendKey(PWinObj *wo, const std::string &key_str)
{
	uint mod, key;
	if (! ActionConfig::parseKey(key_str, mod, key)) {
		return false;
	}

	XEvent ev;
	initSendKeyEvent(ev, wo);

	// Press state modifiers
	std::map<uint, uint>::iterator it;
	for (it  = _state_to_keycode.begin(); it != _state_to_keycode.end(); ++it) {
		if (mod & it->first) {
			ev.xkey.keycode = it->second;
			XSendEvent(X11::getDpy(), wo->getWindow(), True, KeyPressMask, &ev);
			ev.xkey.state |= it->first;
		}
	}

	// Send press and release of main key
	ev.xkey.keycode = key;
	XSendEvent(X11::getDpy(), wo->getWindow(), True, KeyPressMask, &ev);
	ev.type = KeyRelease;
	XSendEvent(X11::getDpy(), wo->getWindow(), True, KeyPressMask, &ev);

	// Release state modifiers
	for (it  = _state_to_keycode.begin(); it != _state_to_keycode.end(); ++it) {
		if (mod & it->first) {
			ev.xkey.keycode = it->second;
			XSendEvent(X11::getDpy(), wo->getWindow(), True, KeyPressMask, &ev);
			ev.xkey.state &= ~it->first;
		}
	}

	return true;
}

void
ActionHandler::actionSetOpacity(PWinObj *client, PWinObj *frame,
                                uint focus, uint unfocus)
{
	if (! unfocus) {
		unfocus = focus;
	}
	CONV_OPACITY(focus);
	CONV_OPACITY(unfocus);
	client->setOpacity(focus, unfocus);
	frame->setOpacity(client);
}

//! @brief Toggles visibility of menu.
//! @param name Name of menu to toggle visibilty of
//! @param stick Stick menu when showing
//! @param e_type Event type triggered this action
//! @param wo_ref Reference to active window object
void
ActionHandler::actionShowMenu(const std::string &name, bool stick,
                              uint e_type, PWinObj *wo_ref)
{
	PMenu *menu = MenuHandler::getMenu(name);
	if (! menu) {
		return;
	}

	if (menu->isMapped()) {
		menu->unmapAll();

	} else {
		// if it's a WORefMenu, set referencing client
		WORefMenu *wo_ref_menu = dynamic_cast<WORefMenu*>(menu);
		if (wo_ref_menu
		    // Don't set reference on these, we don't want a funky title
		    && (wo_ref_menu->getMenuType() != ROOTMENU_TYPE)
		    && (wo_ref_menu->getMenuType() != ROOTMENU_STANDALONE_TYPE)) {
			wo_ref_menu->setWORef(wo_ref);
		}

		// mapping can fail because of empty menu, like iconmenu, so we check
		menu->mapUnderMouse();
		if (menu->isMapped()) {
			menu->giveInputFocus();
			if (stick) {
				menu->setSticky(true);
			}

			// if we opened the menu with the keyboard, select item 0
			if (e_type == KeyPress) {
				menu->selectItemNum(0);
			}
		}
	}
}

void
ActionHandler::actionShowInputDialog(InputDialog *dialog,
                                     const std::string &initial,
                                     Frame *frame, PWinObj *wo)
{
	if (dialog->isMapped()) {
		dialog->unmapWindow();
		return;
	}


	Geometry gm;
	if (frame) {
		frame->getGeometry(gm);
	} else {
		CurrHeadSelector chs = pekwm::config()->getCurrHeadSelector();
		X11::getHeadInfo(X11Util::getCurrHead(chs), gm);
	}

	// skip leave events for the focused window object displaying
	// the input dialog, it should get the focus
	PWinObj *focused_wo = PWinObj::getFocusedPWinObj();
	if (focused_wo && focused_wo->getType() == PWinObj::WO_MENU) {
		PWinObj::setSkipEnterAfter(focused_wo);
	}

	dialog->mapCentered(initial, gm, frame ? frame : wo);
}

bool
ActionHandler::actionWarpPointer(int x, int y)
{
	if (x < 0 || y < 0
	    || static_cast<uint>(x) > X11::getScreenGeometry().width
	    || static_cast<uint>(y) > X11::getScreenGeometry().height) {
		return false;
	}
	X11::warpPointer(x, y);
	return true;
}

//! @brief Searches the client list for a client with a title matching title
Client*
ActionHandler::findClientFromTitle(const std::string &or_title)
{
	RegexString o_rs;

	if (o_rs.parse_match(or_title, true)) {
		Client::client_cit it = Client::client_begin();
		for (; it != Client::client_end(); ++it) {
			if (o_rs == (*it)->getTitle()->getVisible()) {
				return (*it);
			}
		}
	}

	return 0;
}

//! @brief Makes sure Client gets visible and focus it.
//! @param client Client to activate.
void
ActionHandler::gotoClient(Client *client)
{
	Frame *frame = dynamic_cast<Frame*>(client->getParent());
	if (! frame) {
		P_WARN("parent is not a Frame");
		return;
	}

	// Make sure it's visible
	if (! frame->isSticky()
	    && (frame->getWorkspace() != Workspaces::getActive())) {
		Workspaces::setWorkspace(frame->getWorkspace(), false);
		Workspaces::addToMRUFront(frame);
	}

	if (! frame->isMapped()) {
		frame->mapWindow();
	}

	// Activate it within the Frame.
	frame->activateChild(client);
	frame->raise();
	frame->giveInputFocus();
}

/**
 * Initialize XEvent for sending KeyPress/KeyRelease events.
 *
 * @param ev Reference to event.
 * @param wo PWinObj key event is aimed for.
 */
void
ActionHandler::initSendKeyEvent(XEvent &ev, PWinObj *wo)
{
	ev.type = KeyPress;
	ev.xkey.display = X11::getDpy();
	ev.xkey.root = X11::getRoot();
	ev.xkey.window = wo->getWindow();
	ev.xkey.time = CurrentTime;
	ev.xkey.x = 1;
	ev.xkey.y = 1;
	X11::getMousePosition(ev.xkey.x_root, ev.xkey.y_root);
	ev.xkey.same_screen = True;
	ev.xkey.type = KeyPress;
	ev.xkey.state = 0;
}

/**
 * Attaches all marked clients to frame
 */
void
ActionHandler::attachMarked(Frame *frame)
{
	Client::client_cit it = Client::client_begin();
	for (; it != Client::client_end(); ++it) {
		if ((*it)->isMarked()) {
			if ((*it)->getParent() != frame) {
				Frame *parent = static_cast<Frame*>((*it)->getParent());
				parent->removeChild(*it);
				(*it)->setWorkspace(frame->getWorkspace());
				frame->addChild(*it);
			}
			(*it)->setStateMarked(STATE_UNSET);
		}
	}
}

/**
 * Attaches the Client/Frame into the Next/Prev Frame
 */
void
ActionHandler::attachInNextPrevFrame(Client *client, bool frame, bool next)
{
	if (client == nullptr) {
		return;
	}

	Frame *new_frame;
	Frame *parent = static_cast<Frame*>(client->getParent());
	if (next) {
		new_frame = Workspaces::getNextFrame(parent, true, SKIP_FOCUS_TOGGLE);
	} else {
		new_frame = Workspaces::getPrevFrame(parent, true, SKIP_FOCUS_TOGGLE);
	}

	if (new_frame) {
		if (frame) {
			new_frame->addDecor(parent);
		} else {
			parent->setFocused(false);
			parent->removeChild(client);
			new_frame->addChild(client);
			new_frame->activateChild(client);
			new_frame->giveInputFocus();
		}
	}
}

/**
 * Get worksapce number for Warp/Send/Goto Workspace actions, either
 * in form of 3 integer arguments (-1, row, col) or 1 integer where
 * the number is already set.
 */
int
ActionHandler::calcWorkspaceNum(const Action& action, int index)
{
	if (action.getParamI(index) == -1) {
		return pekwm::config()->getWorkspacesPerRow() * action.getParamI(index + 1)
			+ action.getParamI(index + 2);
	}
	return action.getParamI(index);
}

void
ActionHandler::setEventHandler(EventHandler *event_handler)
{
	if (event_handler->initEventHandler()) {
		_event_loop->setEventHandler(event_handler);
	} else {
		delete event_handler;
	}
}
