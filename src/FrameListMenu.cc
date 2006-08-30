//
// FrameListMenu.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#ifdef MENUS

#include "WindowObject.hh"
#include "BaseMenu.hh"
#include "FrameListMenu.hh"

#include "Client.hh"
#include "Frame.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"

#include <algorithm>
#include <cstdio>

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::list;
using std::vector;

//! @fn    FrameListMenu(WindowManager *w, MenuType type)
//! @brief Constructor for FrameListMenu.
FrameListMenu::FrameListMenu(WindowManager *w, MenuType type) :
BaseMenu(w->getScreen(), w->getTheme(), w->getWorkspaces()),
_wm(w), _client(NULL)
{
	_menu_type = type;

	switch(_menu_type) {
	case GOTOMENU_TYPE:
		setName("GoTo");
		break;
	case ICONMENU_TYPE:
		setName("IconMenu");
		break;
	case ATTACH_CLIENT_TYPE:
		setName("Attach Client");
		break;
	case ATTACH_FRAME_TYPE:
		setName("Attach Frame");
		break;
	case ATTACH_CLIENT_IN_FRAME_TYPE:
		setName("Attach Client In Frame");
		break;
	case ATTACH_FRAME_IN_FRAME_TYPE:
		setName("Attach Frame In Frame");
		break;
	default:
		// do nothing
		break;
	}
}

// START - WindowObject interface.

//! @fn    void mapWindow(void)
//! @brief Rebuilds the menu and if it has any items after it shows it.
void
FrameListMenu::mapWindow(void)
{
	updateFrameListMenu();
	if (size()) {
		_client = _wm->getFocusedClient();
		BaseMenu::mapWindow();
	}
}

// END - WindowObject interface.

//! @fn    ~FrameListMenu()
//! @brief Destructor for FrameListMenu
FrameListMenu::~FrameListMenu()
{
}

//! @fn    void handleButton1Release(BaseMenuItem *curr)
//! @brief
void
FrameListMenu::handleButton1Release(BaseMenuItem *curr)
{
	if (!curr)
		return;

	const list<Client*> &c_list = _wm->getClientList();
	list<Client*>::const_iterator it;

	it = find(c_list.begin(), c_list.end(), _client);
	if (it == c_list.end())
		_client = NULL;
	it = find(c_list.begin(), c_list.end(), curr->client);
	if (it == c_list.end())
		curr->client = NULL;

	switch (_menu_type) {
	case GOTOMENU_TYPE:
		handleGotomenu(curr->client);
		break;
	case ICONMENU_TYPE:
		handleIconmenu(curr->client);
		break;
	case ATTACH_CLIENT_TYPE:
		handleAttachClient(curr->client);
		break;
	case ATTACH_FRAME_TYPE:
		handleAttachFrame(curr->client);
		break;
	case ATTACH_CLIENT_IN_FRAME_TYPE:
		handleAttachClientInFrame(curr->client);
		break;
	case ATTACH_FRAME_IN_FRAME_TYPE:
		handleAttachFrameInFrame(curr->client);
		break;
	default:
		// do nothing
		break;
	}
}

//! @fn    void updateFrameListMenu(void)
//! @brief Rebuilds the menu.
void
FrameListMenu::updateFrameListMenu(void)
{
	removeAll();

	char buf[16];
	string name;
	ActionEvent ae;

	// Decide wheter to show clients and iconified.
	bool show_clients = false, show_iconified_only = false;
	if (_menu_type == ATTACH_CLIENT_TYPE)
		show_clients = true;
	else if (_menu_type == ICONMENU_TYPE)
		show_iconified_only = true;

	const list<Frame*> &f_list = _wm->getFrameList();
	list<Frame*>::const_iterator it;

	for (unsigned int i = 0; i < _workspaces->getNumber(); ++i) {
		snprintf(buf, sizeof(buf), "<%d> ", i + 1);

		for (it = f_list.begin(); it != f_list.end(); ++it) {
			if (((*it)->getWorkspace() == i) &&
					(show_iconified_only
					 ? (*it)->isIconified() : !(*it)->isSkip(SKIP_MENUS))) {
				name = buf;
					
				if (show_clients && ((*it)->getNumClients() > 1)) {
					buildFrameNames(*it, name);
				} else {
					buildName(*it, name);
					name.append("] ");
					name.append(*(*it)->getActiveClient()->getName());

					insert(name, ae, (*it)->getActiveClient());
				}
			}
		}
	}

	updateMenu(); // rebuild the menu
}

//! @fn    void buildName(Frame* frame, string& name)
//! @brief Builds the name for the frame.
inline void
FrameListMenu::buildName(Frame* frame, string& name)
{
	name.append("[");
	if (frame->isSticky())
		name.append("*");
	if (frame->isIconified())
		name.append(".");
	if (frame->isShaded())
		name.append("^");
	if (frame->getActiveClient()->getLayer() > LAYER_NORMAL)
		name.append("+");
	else if (frame->getActiveClient()->getLayer() < LAYER_NORMAL)
		name.append("-");
}

//! @fn    void buildFrameNames(Frame* frame, string& pre_name)
//! @brief Builds names for all the clients in a frame.
void
FrameListMenu::buildFrameNames(Frame* frame, string& pre_name)
{
	vector<Client*> *c_list = frame->getClientList();
	vector<Client*>::iterator it = c_list->begin();

	string name, status_name;
	ActionEvent ae;
	buildName(frame, status_name); // add states to the name

	for (unsigned int i = 0; it != c_list->end(); ++i, ++it) {
		name = pre_name;
		if (i) {
			if (i == (c_list->size() - 1))
				name.append("\\");
			else
				name.append("|");
		} else
			name.append("/");

		name.append(status_name);
		if (frame->isActiveClient(*it))
			name.append("A");
		name.append("]");
		name.append(*(*it)->getName());

		insert(name, ae, *it);
	}
}

//! @fn    void handleGotomenu(Client* client)
//! @brief
void
FrameListMenu::handleGotomenu(Client* client)
{
	if (!client)
		return;
	Frame *frame = client->getFrame();

	// make sure it's on correct workspace
	if (!frame->isSticky() &&
			(frame->getWorkspace() != _wm->getWorkspaces()->getActive())) {
		_wm->setWorkspace(frame->getWorkspace(), false);
	}
	if (!frame->isMapped()) // make sure it isn't hidden
		frame->mapWindow();

	frame->raise();
	frame->giveInputFocus();
}

//! @fn    void handleIconmenu(Client* client)
//! @brief
void
FrameListMenu::handleIconmenu(Client* client)
{
	if (!client)
		return;
	Frame *frame = client->getFrame();

	if (frame->getWorkspace() != _wm->getWorkspaces()->getActive())
		frame->setWorkspace(_wm->getWorkspaces()->getActive());

	_workspaces->raise(frame);
	frame->mapWindow();
}

//! @fn    void handleAttachClient(Client* client)
//! @brief
void
FrameListMenu::handleAttachClient(Client* client)
{
	if (!client || !_client)
		return;
	Frame *frame = client->getFrame();
	Frame *cl_frame = _client->getFrame();

	if (cl_frame != frame) {
		frame->removeClient(client);
		client->setWorkspace(cl_frame->getWorkspace());
		cl_frame->insertClient(client);
	}
}

//! @fn    void handleAttachFrame(Client* client)
//! @brief
void
FrameListMenu::handleAttachFrame(Client* client)
{
	if (!client || !_client)
		return;

	_client->getFrame()->insertFrame(client->getFrame());
}

//! @fn    void handleAttachClientInFrame(Client* client)
//! @brief
void
FrameListMenu::handleAttachClientInFrame(Client* client)
{
	if (!client || !_client)
		return;
	Frame *frame = client->getFrame();
	Frame *cl_frame = _client->getFrame();

	if (cl_frame != frame) {
		cl_frame->removeClient(_client);
		_client->setWorkspace(cl_frame->getWorkspace());
		frame->insertClient(_client);
	}
}

//! @fn    void handleAttachFrameInFrame(Client* client)
//! @brief
void
FrameListMenu::handleAttachFrameInFrame(Client* client)
{
	if (!client || !_client)
		return;

	client->getFrame()->insertFrame(_client->getFrame());
}

#endif // MENUS
