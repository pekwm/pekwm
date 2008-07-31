//
// FrameListMenu.cc for pekwm
// Copyright © 2002-2007 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef MENUS

#include <algorithm>
#include <cstdio>
#include <iostream>

#include "PWinObj.hh"
#include "PDecor.hh"
#include "PMenu.hh"
#include "WORefMenu.hh"
#include "FrameListMenu.hh"

#include "Config.hh"
#include "Client.hh"
#include "Frame.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"

using std::cerr;
using std::endl;
using std::list;
using std::string;
using std::vector;
using std::wstring;

//! @brief FrameListMenu constructor.
//! @param scr Pointer to PScreen.
//! @param theme Pointer to Theme
//! @param type Type of menu.
//! @param title Title of menu.
//! @param name Name of menu
//! @param decor_name Decor name, defaults to MENU
FrameListMenu::FrameListMenu(PScreen *scr, Theme *theme,
                             MenuType type,
                             const std::wstring &title, const std::string &name,
                             const std::string &decor_name)
    : WORefMenu(scr, theme, title, name, decor_name)
{
    _menu_type = type;
}

//! @brief FrameListMenu destructor
FrameListMenu::~FrameListMenu(void)
{
}

// START - PWinObj interface.

//! @brief Rebuilds the menu and if it has any items after it shows it.
void
FrameListMenu::mapWindow(void)
{
    updateFrameListMenu();
    if (size() > 0) {
        WORefMenu::mapWindow();
    }
}

// END - PWinObj interface.

//! @brief
void
FrameListMenu::handleItemExec(PMenu::Item *item)
{
    if (! item) {
        return;
    }

    if (! PWinObj::windowObjectExists(_wo_ref)) {
        _wo_ref = NULL;
    }
    if (! PWinObj::windowObjectExists(item->getWORef())) {
        item->setWORef(NULL);
    }

    switch (_menu_type) {
    case GOTOMENU_TYPE:
    case GOTOCLIENTMENU_TYPE:
        handleGotomenu(dynamic_cast<Client*>(item->getWORef()));
        break;
    case ICONMENU_TYPE:
        handleIconmenu(dynamic_cast<Client*>(item->getWORef()));
        break;
    case ATTACH_CLIENT_TYPE:
    case ATTACH_FRAME_TYPE:
        handleAttach(dynamic_cast<Client*>(_wo_ref), dynamic_cast<Client*>(item->getWORef()),
                     (_menu_type == ATTACH_FRAME_TYPE));
        break;
    case ATTACH_CLIENT_IN_FRAME_TYPE:
    case ATTACH_FRAME_IN_FRAME_TYPE:
        handleAttach(dynamic_cast<Client*>(item->getWORef()), dynamic_cast<Client*>(_wo_ref),
                     (_menu_type == ATTACH_FRAME_IN_FRAME_TYPE));
        break;
    default:
        // do nothing
        break;
    }
}

//! @brief Rebuilds the menu.
void
FrameListMenu::updateFrameListMenu(void)
{
    removeAll();

    wchar_t buf[16];
    wstring name;

    // need to add an action, otherwise it looks as if we don't have anything
    // to exec and thus it doesn't get handled.
    Action action;
    ActionEvent ae;
    ae.action_list.push_back(action);

    // Decide wheter to show clients and iconified.
    bool show_clients = false, show_iconified_only = false;
    if (_menu_type == ATTACH_CLIENT_TYPE || _menu_type == GOTOCLIENTMENU_TYPE) {
        show_clients = true;
    } else if (_menu_type == ICONMENU_TYPE) {
        show_iconified_only = true;
    }

    list<Frame*>::const_iterator it;

    // if we have 1 workspace, we won't put an workspace indicator
    buf[0] = '\0';

    for (uint i = 0; i < Workspaces::instance()->size(); ++i) {
        if (Workspaces::instance()->size() > 1) {
            swprintf(buf, 16, L"<%d> ", i + 1);
        }

        for (it = Frame::frame_begin(); it != Frame::frame_end(); ++it) {
            if (((*it)->getWorkspace() == i) && // sort by workspace
                    // don't include ourselves if we're not doing a gotoclient menu
                    ((_menu_type != GOTOCLIENTMENU_TYPE)
                     ? ((*it)->getActiveChild() != _wo_ref)
                     : true) &&
                    (show_iconified_only
                     ? (*it)->isIconified()
                     : !(*it)->isSkip(SKIP_MENUS))) {
                name = buf;

                if (show_clients) {
                    buildFrameNames(*it, name);

                } else {
                    buildName(*it, name);
                    name.append(L"] ");
                    name.append(static_cast<Client*>((*it)->getActiveChild())->getTitle()->getVisible());

                    insert(name, ae, (*it)->getActiveChild(),
                           static_cast<Client*>((*it)->getActiveChild())->getIcon());
                }
            }
        }
    }

    // remove the last separator, not needed
    if (show_clients && (size() > 0)) {
        remove(_item_list.back());
    }

    buildMenu();
}

//! @brief Builds the name for the frame.
void
FrameListMenu::buildName(Frame* frame, std::wstring &name)
{
    name.append(L"[");
    if (frame->isSticky()) {
        name.append(L"*");
    }
    if (frame->isIconified()) {
        name.append(L".");
    }
    if (frame->isShaded()) {
        name.append(L"^");
    }
    if (frame->getActiveChild()->getLayer() > LAYER_NORMAL) {
        name.append(L"+");
    } else if (frame->getActiveChild()->getLayer() < LAYER_NORMAL) {
        name.append(L"-");
    }
}

//! @brief Builds names for all the clients in a frame.
void
FrameListMenu::buildFrameNames(Frame *frame, std::wstring &pre_name)
{
    wstring name, status_name;

    // need to add an action, otherwise it looks as if we don't have anything
    // to exec and thus it doesn't get handled.
    Action action;
    ActionEvent ae;
    ae.action_list.push_back(action);

    buildName(frame, status_name); // add states to the name

    list<PWinObj*>::iterator it(frame->begin());
    for (; it != frame->end(); ++it) {
        name = pre_name;
        name.append(status_name);
        if (frame->getActiveChild() == *it) {
            name.append(L"A");
        }
        name.append(L"] ");
        name.append(static_cast<Client*>(*it)->getTitle()->getVisible());

        insert(name, ae, *it, static_cast<Client*>(*it)->getIcon());
    }

    // add separator
    PMenu::Item *item = new PMenu::Item(L"");
    item->setType(PMenu::Item::MENU_ITEM_SEPARATOR);
    insert(item);
}

//! @brief Handles gotomeu presses
void
FrameListMenu::handleGotomenu(Client *client)
{
    if (! client) {
        return;
    }
    Frame *frame = static_cast<Frame*>(client->getParent());

    // make sure it's on correct workspace
    if (! frame->isSticky() &&
            (frame->getWorkspace() != Workspaces::instance()->getActive())) {
        Workspaces::instance()->setWorkspace(frame->getWorkspace(), false);
    }
    // make sure it isn't hidden
    if (! frame->isMapped()) {
        frame->mapWindow();
    }

    frame->activateChild(client);
    frame->raise();
    frame->giveInputFocus();
}

//! @brief Handles iconmenu presses
void
FrameListMenu::handleIconmenu(Client *client)
{
    if (! client) {
        return;
    }
    Frame *frame = static_cast<Frame*>(client->getParent());

    // make sure it's on the current workspace
    if (frame->getWorkspace() != Workspaces::instance()->getActive()) {
        frame->setWorkspace(Workspaces::instance()->getActive());
    }

    frame->raise();
    frame->mapWindow();
}

//! @brief Handles attach*menu presses
void
FrameListMenu::handleAttach(Client *client_to, Client *client_from, bool frame)
{
    if (! client_to || ! client_from) {
        return;
    }

    Frame *frame_to = static_cast<Frame*>(client_to->getParent());
    Frame *frame_from = static_cast<Frame*>(client_from->getParent());

    // insert frame
    if (frame) {
        frame_to->addDecor(frame_from);
        // insert client
    } else if (frame_to != frame_from) {
        frame_from->removeChild(client_from);
        client_from->setWorkspace(frame_to->getWorkspace());
        frame_to->addChild(client_from);
        frame_to->activateChild(client_from);
        frame_to->giveInputFocus();
    }
}

#endif // MENUS
