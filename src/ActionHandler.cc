//
// ActionHandler.cc for pekwm
// Copyright (C) 2002-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "ActionHandler.hh"

#include "Debug.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "x11.hh"
#include "Frame.hh"
#include "Client.hh"
#include "Config.hh"
#include "CmdDialog.hh"
#include "SearchDialog.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"
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

#include <memory>

using std::string;
using std::find;
using std::map;

ActionHandler *ActionHandler::_instance = 0;

//! @brief ActionHandler constructor
ActionHandler::ActionHandler(void)
{
    _instance = this;

    // Initialize state_to_keycode map
    for (uint i = 0; i < X11::MODIFIER_TO_MASK_NUM; ++i) {
        uint modifier = X11::MODIFIER_TO_MASK[i];
        _state_to_keycode[modifier] = X11::getKeycodeFromMask(modifier);
    }
}

//! @brief ActionHandler destructor
ActionHandler::~ActionHandler(void)
{
    _instance = 0;
}

//! @brief Executes an ActionPerformed event.
void
ActionHandler::handleAction(const ActionPerformed &ap)
{
    PWinObj *wo = ap.wo;
    Client *client = 0;
    Frame *frame = 0;
    PMenu *menu = 0;
    PDecor *decor = 0;
    bool matched = false;

    // go through the list of actions and execute them
    vector<Action>::const_iterator it = ap.ae.action_list.begin();
    for (; it != ap.ae.action_list.end(); ++it, matched = false) {
        // Determine what type if any of the window object that is focused
        // and check if it is still alive.
        lookupWindowObjects(&wo, &client, &frame, &menu, &decor);

        // actions valid for all PWinObjs
        if (! matched && wo) {
            matched = true;
            switch (it->getAction()) {
            case ACTION_FOCUS:
                wo->giveInputFocus();
                break;
            case ACTION_UNFOCUS:
                PWinObj::getRootPWinObj()->giveInputFocus();
                break;
            case ACTION_FOCUS_DIRECTIONAL:
                actionFocusDirectional(wo, DirectionType(it->getParamI(0)), it->getParamI(1));
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
                if (ap.type == MotionNotify) {
                    frame->doGroupingDrag(ap.event.motion, client, it->getParamI(0));
                }
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
            case ACTION_MOVE_RESIZE:
                frame->doKeyboardMoveResize();
                break;
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
                frame->setGeometry(it->getParamS(), it->getParamI(0), it->getParamI(1) == 1);
                break;
            case ACTION_RAISE:
                if (it->getParamI(0)) {
                    WindowManager::instance()->familyRaiseLower(client, true);
                } else {
                    frame->raise();
                }
                break;
            case ACTION_LOWER:
                if (it->getParamI(0)) {
                    WindowManager::instance()->familyRaiseLower(client, false);
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
                frame->moveToHead(it->getParamI(0));
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
                actionSendToWorkspace(decor, it->getParamI(0));
                break;
            case ACTION_DETACH:
                frame->detachClient(client);
                break;
            case ACTION_ATTACH_MARKED:
                WindowManager::instance()->attachMarked(frame);
                break;
            case ACTION_ATTACH_CLIENT_IN_NEXT_FRAME:
                WindowManager::instance()->attachInNextPrevFrame(client, false, true);
                break;
            case ACTION_ATTACH_CLIENT_IN_PREV_FRAME:
                WindowManager::instance()->attachInNextPrevFrame(client, false, false);
                break;
            case ACTION_ATTACH_FRAME_IN_NEXT_FRAME:
                WindowManager::instance()->attachInNextPrevFrame(client, true, true);
                break;
            case ACTION_ATTACH_FRAME_IN_PREV_FRAME:
                WindowManager::instance()->attachInNextPrevFrame(client, true, false);
                break;
            case ACTION_SET_OPACITY:
                actionSetOpacity(client, frame, it->getParamI(0), it->getParamI(1));
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
                menu->exec(menu->getItemCurr());

                // special case: execItem can cause an reload to be issued, if that's
                // the case it causes the list (ae) to change and therefore
                // it can't be used anymore
                return;
            case ACTION_MENU_ENTER_SUBMENU:
                if (menu->getItemCurr() &&
                        menu->getItemCurr()->getWORef() &&
                        (menu->getItemCurr()->getWORef()->getType() == PWinObj::WO_MENU)) {
                    menu->mapSubmenu(static_cast<PMenu*>(menu->getItemCurr()->getWORef()), true);
                }
                break;
            case ACTION_MENU_LEAVE_SUBMENU:
                menu->gotoParentMenu();
                break;
            case ACTION_CLOSE:
                menu->unmapAll();
                WindowManager::instance()->findWOAndFocus(0);
                break;
            default:
                matched = false;
                break;
            }
        }
        // actions valid for pdecor
        if (! matched && decor) {
            int x_root, y_root;

            matched = true;
            switch (it->getAction()) {
            case ACTION_MOVE:
                // Get root position, previously event positions was
                // used however this seems to be error prone on
                // Xinerama setups
                X11::getMousePosition(x_root, y_root);

                decor->doMove(x_root, y_root);
                break;
            case ACTION_CLOSE:
                decor->unmapWindow();
                if (decor->getType() == PWinObj::WO_CMD_DIALOG
                    || decor->getType() == PWinObj::WO_SEARCH_DIALOG) {
                    WindowManager::instance()->findWOAndFocus(0);
                }
                break;
            case ACTION_WARP_TO_WORKSPACE:
                actionWarpToWorkspace(decor, it->getParamI(0));
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
            case ACTION_SET:
            case ACTION_UNSET:
            case ACTION_TOGGLE:
                handleStateAction(*it, wo, client, frame);
                break;
            case ACTION_NEXT_FRAME:
                actionFocusToggle(ap.ae.sym, it->getParamI(0), 1 /* Direction */,
                                  it->getParamI(1), false);
                break;
            case ACTION_NEXT_FRAME_MRU:
                actionFocusToggle(ap.ae.sym, it->getParamI(0), 1 /* Direction */,
                                  it->getParamI(1), true);
                break;
            case ACTION_PREV_FRAME:
                actionFocusToggle(ap.ae.sym, it->getParamI(0), -1 /* Direction */,
                                  it->getParamI(1), false);
                break;
            case ACTION_PREV_FRAME_MRU:
                actionFocusToggle(ap.ae.sym, it->getParamI(0), -1 /* Direction */,
                                  it->getParamI(1), true);
                break;
            case ACTION_GOTO_WORKSPACE:
                // Events caused by a motion event ( dragging frame to
                // the edge ) or enter event ( moving the pointer to
                // the edge ) should warp the pointer.
                actionGotoWorkspace(it->getParamI(0), (ap.type == MotionNotify) || (ap.type == EnterNotify));
                break;
            case ACTION_FIND_CLIENT:
                actionFindClient(Util::to_wide_str(it->getParamS()));
                break;
            case ACTION_GOTO_CLIENT_ID:
                actionGotoClientID(it->getParamI(0));
                break;
            case ACTION_EXEC:
                actionExec(client, it->getParamS());
                break;
            case ACTION_SHOW_MENU:
                actionShowMenu(it->getParamS(), it->getParamI(0),
                               ap.type, client ? client : wo);
                break;
            case ACTION_HIDE_ALL_MENUS:
                MenuHandler::hideAllMenus();
                break;
            case ACTION_RELOAD:
                WindowManager::instance()->reload();
                break;
            case ACTION_RESTART:
                WindowManager::instance()->restart();
                break;
            case ACTION_RESTART_OTHER:
                if (it->getParamS().size())
                    WindowManager::instance()->restart(it->getParamS());
                break;
            case ACTION_EXIT:
                WindowManager::instance()->shutdown();
                break;
            case ACTION_SHOW_CMD_DIALOG:
                actionShowInputDialog(WindowManager::instance()->getCmdDialog(), it->getParamS(), frame, wo);
                break;
            case ACTION_SHOW_SEARCH_DIALOG:
                actionShowInputDialog(WindowManager::instance()->getSearchDialog(), it->getParamS(), frame, wo);
                break;
            case ACTION_DEBUG:
#ifdef DEBUG
                Debug::doAction(it->getParamS());
#endif
                break;
            default:
                matched = false;
                break;
            }
        }
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
            frame->setStateTitle(sa, client, Util::to_wide_str(action.getParamS()));
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
            WindowManager::instance()->getHarbour()->setStateHidden(sa);
            break;
        case ACTION_STATE_GLOBAL_GROUPING:
            WindowManager::instance()->setStateGlobalGrouping(sa);
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
ActionHandler::findMouseAction(uint button, uint state, MouseEventType type, vector<ActionEvent> *actions)
{
    if (! actions) {
        return 0;
    }

    X11::stripStateModifiers(&state);
    X11::stripButtonModifiers(&state);

    vector<ActionEvent>::iterator it(actions->begin());
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
ActionHandler::actionExec(Client *client, const std::string &command)
{
    if (command.size()) {
        Client::setClientEnvironment(client);
        Util::forkExec(command);
    }
}

//! @brief Searches for a client matching titles and makes it visible
void
ActionHandler::actionFindClient(const std::wstring &title)
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
ActionHandler::actionGotoWorkspace(uint workspace, bool warp)
{
    Workspaces::gotoWorkspace(workspace, warp);
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

//! @brief Sends client to specified workspace
//! @param direction Workspace to send to, or special meaning relative space.
void
ActionHandler::actionSendToWorkspace(PDecor *decor, int direction)
{
    // Convenience
    const uint per_row = Workspaces::getPerRow(),
        cur_row = Workspaces::getRow(),
        cur_act = Workspaces::getActive(),
        row_min = Workspaces::getRowMin(),
        row_max = Workspaces::getRowMax();

    switch (static_cast<WorkspaceChangeType>(direction)) {
    case WORKSPACE_LEFT:
    case WORKSPACE_PREV:
        if (cur_act > row_min) {
            decor->setWorkspace(cur_act - 1);
        } else if (static_cast<uint>(direction) == WORKSPACE_PREV) {
            decor->setWorkspace(row_max);
        }
        break;
    case WORKSPACE_NEXT:
    case WORKSPACE_RIGHT:
        if (cur_act < row_max) {
            decor->setWorkspace(cur_act + 1);
        } else if (static_cast<uint>(direction) == WORKSPACE_NEXT) {
            decor->setWorkspace(row_min);
        }
        break;
    case WORKSPACE_PREV_V:
    case WORKSPACE_UP:
        if (cur_act >= per_row) {
            decor->setWorkspace(cur_act - per_row);
        } else if (static_cast<uint>(direction) == WORKSPACE_PREV_V) {
            // Bottom left + column
            decor->setWorkspace(Workspaces::size() - per_row
                                + cur_act - cur_row * per_row);
        }
        break;
    case WORKSPACE_NEXT_V:
    case WORKSPACE_DOWN:
        if ((cur_act + per_row) < Workspaces::size()) {
            decor->setWorkspace(cur_act + per_row);
        } else if (static_cast<uint>(direction) == WORKSPACE_NEXT_V) {
            decor->setWorkspace(cur_act - cur_row * per_row);
        }
        break;
    case WORKSPACE_LAST:
        decor->setWorkspace(Workspaces::getPrevious());
        break;
    default:
        decor->setWorkspace(direction);
        break;
    }
}

void
ActionHandler::actionWarpToWorkspace(PDecor *decor, uint direction)
{
    // Removing the already accumulated motion events can help
    // to avoid skipping workspaces (see task #77).
    X11::removeMotionEvents();

    // actually did move
    if (Workspaces::gotoWorkspace(DirectionType(direction), true)) {
        int x, y;
        X11::getMousePosition(x, y);

        decor->move(decor->getClickX() + x - decor->getPointerX(),
                    decor->getClickY() + y - decor->getPointerY());
        decor->setWorkspace(Workspaces::getActive());
    }
}

//! @brief Tries to find the next/prev frame relative to the focused client
void
ActionHandler::actionFocusToggle(uint button, uint raise, int off,
                                 bool show_iconified, bool mru)
{
    std::unique_ptr<PMenu> menu{createNextPrevMenu(show_iconified, mru)};

    // no clients in the list
    if (menu->size() == 0) {
        return;
    }

    // unable to grab keyboard
    if (! X11::grabKeyboard(X11::getRoot())) {
        return;
    }

    // find the focused window object
    PWinObj *fo_wo = 0;
    if (PWinObj::isFocusedPWinObj(PWinObj::WO_CLIENT)) {
        fo_wo = PWinObj::getFocusedPWinObj()->getParent();

        vector<PMenu::Item*>::const_iterator it(menu->m_begin());
        for (; it != menu->m_end(); ++it) {
            if ((*it)->getWORef() == fo_wo) {
                menu->selectItem(it);
                break;
            }
        }
        fo_wo->setFocused(false);
    }

    if (Config::instance()->getShowFrameList()) {
        menu->buildMenu();

        Geometry head;
        X11::getHeadInfo(X11::getCurrHead(), head);
        menu->move(head.x + ((head.width - menu->getWidth()) / 2),
                   head.y + ((head.height - menu->getHeight()) / 2));
        menu->setFocused(true);
        menu->mapWindowRaised();
    }

    menu->selectItemRel(off);
    fo_wo = menu->getItemCurr()->getWORef();

    XEvent ev;
    bool cycling = true;
    bool was_iconified = false;

    while (cycling) {
        if (fo_wo) {
            fo_wo->setFocused(true);
            if (raise == ALWAYS_RAISE) {
                // Make sure it's not iconified if raise is on.
                if (fo_wo->isIconified()) {
                    was_iconified = true;
                    fo_wo->mapWindow();
                }
                fo_wo->raise();
            } else if (raise == TEMP_RAISE) {
                XRaiseWindow(X11::getDpy(), fo_wo->getWindow());
                XRaiseWindow(X11::getDpy(), menu->getWindow());
            }
        }

        XMaskEvent(X11::getDpy(), KeyPressMask|KeyReleaseMask, &ev);
        if (ev.type == KeyPress) {
            if (ev.xkey.keycode == button) {
                if (fo_wo) {
                    if (raise == TEMP_RAISE) {
                        Workspaces::fixStacking(fo_wo);
                    }
                    // Restore iconified state
                    if (was_iconified) {
                        was_iconified = false;
                        fo_wo->iconify();
                    }
                    fo_wo->setFocused(false);
                }

                menu->selectItemRel(off);
                fo_wo = menu->getItemCurr()->getWORef();
            } else {
                XPutBackEvent(X11::getDpy(), &ev);
                cycling = false;
            }
        } else if (ev.type == KeyRelease) {
            if (IsModifierKey(X11::getKeysymFromKeycode(ev.xkey.keycode))) {
                cycling = false;
            }
        } else {
            XPutBackEvent(X11::getDpy(), &ev);
        }
    }

    X11::ungrabKeyboard();

    // Got something to focus
    if (fo_wo) {
        if (raise == TEMP_RAISE) {
            fo_wo->raise();
            fo_wo->setFocused(true);
        }

        // De-iconify if iconified, user probably wants this
        if (fo_wo->isIconified()) {
            // If the window was iconfied, and sticky
            fo_wo->setWorkspace(Workspaces::getActive());
            fo_wo->mapWindow();
            fo_wo->raise();
        } else if (Raise(raise) == END_RAISE) {
            fo_wo->raise();
        }

        // Give focus
        fo_wo->giveInputFocus();
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
    if (! Config::instance()->parseKey(key_str, mod, key)) {
        return false;
    }

    XEvent ev;
    initSendKeyEvent(ev, wo);

    // Press state modifiers
    map<uint, uint>::iterator it;
    for (it = _state_to_keycode.begin(); it != _state_to_keycode.end(); ++it) {
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
    for (it = _state_to_keycode.begin(); it != _state_to_keycode.end(); ++it) {
        if (mod & it->first) {
            ev.xkey.keycode = it->second;
            XSendEvent(X11::getDpy(), wo->getWindow(), True, KeyPressMask, &ev);
            ev.xkey.state &= ~it->first;
        }
    }

    return true;
}

void
ActionHandler::actionSetOpacity(PWinObj *client, PWinObj *frame, uint focus, uint unfocus)
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
                                     const std::string &initial, Frame *frame, PWinObj *wo)
{
    if (dialog->isMapped()) {
        dialog->unmapWindow();
    } else {
        Geometry gm;
        if (frame) {
            frame->getGeometry(gm);
        } else {
            uint head = X11::getCurrHead();
            X11::getHeadInfo(head, gm);
        }

        dialog->mapCentered(initial, gm, frame ? frame : wo);
    }
}

//! @brief Creates a menu containing a list of Frames currently visible
//! @param show_iconified Flag to show/hide iconified windows
//! @param mru Whether MRU order should be used or not.
PMenu*
ActionHandler::createNextPrevMenu(bool show_iconified, bool mru)
{
    Frame *fr;
    ActionEvent ae; // empty ae, used when inserting
    PMenu *menu =
        new PMenu(WindowManager::instance()->getTheme(),
                  mru?L"MRU Windows":L"Windows",
                  "" /* Empty name*/);

    vector<Frame*>::const_iterator itr = mru?WindowManager::instance()->mru_begin():Frame::frame_begin();
    vector<Frame*>::const_iterator end = mru?WindowManager::instance()->mru_end():Frame::frame_end();
    for (; itr != end; ++itr) {
        fr = *itr;
        if (createMenuInclude(fr, show_iconified)) {
            menu->insert(static_cast<Client*>(fr->getActiveChild())->getTitle()->getVisible(),
                         ae, fr, static_cast<Client*>(fr->getActiveChild())->getIcon());
        }
    }

    return menu;
}

//! @brief Helper to decide wheter or not to include Frame in menu
//! @param frame Frame to check
//! @param show_iconified Wheter or not to include iconified windows
//! @return true if it should be included, else false
bool
ActionHandler::createMenuInclude(Frame *frame, bool show_iconified)
{
    // Make sure the frame is mapped, or on the correct workspace if
    // it's iconified. Also make sure it's possible to give it focus
    // and should not be skipped. The condition is rather complex, so
    // we split it up for readability.
    
    // focw == frame on current workspace
    bool focw = frame->isSticky() || frame->getWorkspace() == Workspaces::getActive();
    
    // ibs == iconified but should be shown
    bool ibs = show_iconified && frame->isIconified() && focw;
    
    // mos == mapped or shown nonetheless 
    bool mos = frame->isMapped() || ibs;
    
    return ! frame->isSkip(SKIP_FOCUS_TOGGLE) && frame->isFocusable() && mos;
}

//! @brief Searches the client list for a client with a title matching title
Client*
ActionHandler::findClientFromTitle(const std::wstring &or_title)
{
    RegexString o_rs;

    if (o_rs.parse_match(or_title, true)) {
        vector<Client*>::const_iterator it(Client::client_begin());
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
        WARN("parent is not a Frame");
        return;
    }

    // Make sure it's visible
    if (! frame->isSticky()
        && (frame->getWorkspace() != Workspaces::getActive())) {
        Workspaces::setWorkspace(frame->getWorkspace(), false);
        WindowManager::instance()->addToMRUFront(frame);
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
