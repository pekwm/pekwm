//
// Frame.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowObject.hh"
#include "Frame.hh"

#include "ScreenInfo.hh"
#include "Config.hh"
#include "AutoProperties.hh"
#include "FrameWidget.hh"
#include "Button.hh"
#include "Client.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"
#include "KeyGrabber.hh"
#ifdef HARBOUR
#include "Harbour.hh"
#endif // HARBOUR

#ifdef MENUS
#include "BaseMenu.hh"
#include "ActionMenu.hh"
#endif //MENUS

#include <algorithm>
#include <functional>
#include <cstdio> // for snprintf

extern "C" {
#include <X11/Xatom.h>
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;
using std::list;
using std::vector;
using std::mem_fun;

Frame::Frame(WindowManager *w, Client *cl) : WindowObject(w->getScreen()->getDisplay()),
_wm(w),
_id(0), _fw(NULL),
_client(NULL), _button(NULL), _class_hint(NULL),
_pointer_x(0), _pointer_y(0),
_old_cx(0), _old_cy(0), _real_height(0)
{
	// setup basic pointers
	_scr = _wm->getScreen();
	_theme = _wm->getTheme();

	_fw = new FrameWidget(_scr, _theme);
	_class_hint = new ClassHint();

	// WindowObject attributes
	// NOTE: You might find this a bit weird, but currently this is the best
	// way I've found for now.
	_window = _fw->getWindow();
	_type = WO_FRAME;

	// get unique id of the frame, if the client didn't have a id
	if (!_wm->isStartup()) {
		long id =
			_wm->getAtomLongValue(cl->getWindow(),
														_wm->getPekwmAtoms()->getAtom(PEKWM_FRAME_ID));
		if (id && (id != -1))
			_id = id;
	} else
		_id = _wm->findUniqueFrameId();

	// get the clients class_hint
	*_class_hint = *cl->getClassHint();

	_scr->grabServer();

	// Before setting position and size up we make sure the decor state
	// of the client match the decore state of the framewidget
 if (!cl->hasBorder())
    _fw->setBorder(false);
  if (!cl->hasTitlebar())
    _fw->setTitlebar(false);

	// Setup position and size
	bool place = false;

	if (cl->getGeometry(_gm)) { // mapped, we need to gravitate
		gravitate(cl, true); // apply gravity
		cl->getGeometry(_gm);

	}	else if (!cl->isPlaced() && // not placed
						 !cl->setPUPosition(_gm)) { // no specified position
		place = true;
	}

	_gm.y -= _fw->getTitleHeight() + _fw->borderTop();
	_gm.width += _fw->borderLeft() + _fw->borderRight();
	_gm.height += _fw->getTitleHeight() + _fw->borderTop() + _fw->borderBottom();

	if (place)
		_wm->getWorkspaces()->placeFrame(this, _gm);

	_old_gm = _gm;

	_scr->ungrabServer(true); // ungrab and sync

	// TO-DO: Fix the order of this. Needed for the workspace insert
	_layer = cl->getLayer();

	// I add these to the list before I insert the client into the frame to
	// be able to skip an extra updateClientList
	_wm->addToFrameList(this);
	_wm->getWorkspaces()->insert(this);

	// now insert the client in the frame we created, do not give it focus.
	insertClient(cl, false);

	// set the window states, shaded, maximized...
	getState(cl);

	// make sure we don't cover anything we shouldn't
	if (!_client->hasStrut()) {
#ifdef HARBOUR
		if (fixGeometry(_wm->getConfig()->getHarbourMaximizeOver())) {
#else // !HARBOUR
		if (fixGeometry(true)) {
#endif // HARBOUR
			updatePosition();
			updateSize();
		}
	}

	// figure out if we should be hidden or not, do not read autoprops
	setWorkspace(_client->getWorkspace()); //, false);
}

Frame::~Frame()
{
	// remove from the lists
	_wm->getWorkspaces()->remove(this);
	_wm->removeFromFrameList(this);

	// if we have any client left in the frame, lets make new frames for them
	if (_client_list.size()) {
		vector<Client*>::iterator it = _client_list.begin();
		for (; it != _client_list.end(); ++it) {
			Frame *frame = new Frame(_wm, *it);
			(*it)->setFrame(frame);
		}
	}
	_client_list.clear();

	if (_class_hint)
		delete _class_hint;
	if (_fw)
		delete _fw;

	_wm->getWorkspaces()->updateClientStackingList(true, true);
}

// START - WindowObject interface.

//! @fn    void mapWindow(void)
//! @brief Unhides the Frame, Title and it's Clients.
void
Frame::mapWindow(void)
{
	if (_mapped)
		return;

	WindowObject::mapWindow();

	for_each(_client_list.begin(), _client_list.end(),
					 mem_fun(&Client::mapWindow));
}

//! @fn    void unmapWindow(void)
//! @brief Hides the Frame, Title and it's Clients.
void
Frame::unmapWindow(void)
{
	if (!_mapped)
		return;

	if (_iconified) {
		for_each(_client_list.begin(), _client_list.end(),
						 mem_fun(&Client::iconify));
	} else {
		for_each(_client_list.begin(), _client_list.end(),
						 mem_fun(&Client::unmapWindow));
	}

	WindowObject::unmapWindow();
}

//! @fn    void iconify(void)
//! @brief Iconifies the Frame.
void
Frame::iconify(void)
{
	if (_iconified)
		return;
	_iconified = true;

	unmapWindow();
}

//! @fn    void setWorkspace(unsigned int workspace)
//! @brief Moves the frame and it's clients to the workspace workspace
//! @param workspace Workspace to switch to.
void
Frame::setWorkspace(unsigned int workspace) //, bool read_autoprops)
{
	if (workspace >= _wm->getWorkspaces()->getNumber())
		return;

	_workspace = workspace;
	//	if (read_autoprops) // to avoid infinite recursion
	//		readAutoprops(APPLY_ON_WORKSPACE);

	vector<Client*>::iterator it = _client_list.begin();
	for (; it != _client_list.end(); ++it)
		(*it)->setWorkspace(_workspace);

	if (!_mapped && !_iconified) {
		if (_sticky || (_workspace == _wm->getWorkspaces()->getActive()))
			mapWindow();
	} else if (!_sticky && (_workspace != _wm->getWorkspaces()->getActive()))
		unmapWindow();
}

//! @fn    void setFocused(bool focused)
//! @brief Redraws the frame.
void
Frame::setFocused(bool focused)
{
	if (_focused == focused)
		return;
	_focused = focused;

	_fw->setFocused(_focused);
}

//! @fn    void giveInputFocus(void)
//! @brief Gives the Frame's active client focus.
void
Frame::giveInputFocus(void)
{
	if (!_mapped)
		return;

	if (_client)
		XSetInputFocus(_dpy, _client->getWindow(),
									 RevertToPointerRoot, CurrentTime);
	else
		XSetInputFocus(_dpy, _scr->getRoot(),
									 RevertToPointerRoot, CurrentTime);
}

// END - WindowObject interface.

//! @fn    void setId(unsigned int id)
//! @brief
void
Frame::setId(unsigned int id)
{
	_id = id;

	vector<Client*>::iterator it = _client_list.begin();
	for (; it != _client_list.end(); ++it) {
		_wm->setAtomLongValue((*it)->getWindow(),
													_wm->getPekwmAtoms()->getAtom(PEKWM_FRAME_ID), id);
	}
}



//! @fn    void loadTheme(void)
//! @brief Loads and if already loaded, it unloads the necessary parts
void
Frame::loadTheme(void)
{
	_fw->loadTheme();

	// move all the clients so that they don't sit on top or, under the titlebar
	vector<Client*>::iterator it = _client_list.begin();
	for (; it != _client_list.end(); ++it) {
		XMoveWindow(_dpy, (*it)->getWindow(),
								_fw->borderLeft(), _fw->getTitleHeight() + _fw->borderTop());
	}

	// update the dimensions of the frame
	_gm.width = _client->getWidth() + _fw->borderLeft() + _fw->borderRight();
	_gm.height = _client->getHeight() + _fw->getTitleHeight() +
		_fw->borderTop() + _fw->borderBottom();

	updateSize();
}

//! @fn    void getState(Client *cl)
//! @brief Gets the state from the Client
void
Frame::getState(Client *cl)
{
	if (!cl)
		return;

	if (_sticky != cl->isSticky())
		_sticky = !_sticky;
	if (_state.maximized_horz != cl->isMaximizedHorz())
		maximize(true, false);
	if (_state.maximized_vert != cl->isMaximizedVert())
		maximize(false, true);
	if (_state.shaded != cl->isShaded())
		shade();
	//	if (m_state.hidden != cl->isHidden()) {
	//		if (m_state_hidden) unhide();
	//	else hide();
	//}
	if (_layer != cl->getLayer())
		_layer = cl->getLayer(); // TO-DO: Restack, No?
	if (_workspace != cl->getWorkspace())
		setWorkspace(cl->getWorkspace());

	if (_fw->hasBorder() != _client->hasBorder())
		toggleBorder(false);
	if (_fw->hasTitlebar() != _client->hasTitlebar())
		toggleTitlebar(false);

	if (_iconified != cl->isIconified()) {
		if (_iconified)
			mapWindow();
		else
			iconify();
	}

	if (_state.skip != cl->getSkip())
		_state.skip = cl->getSkip();
}

//! @fn    void applyState(Client *cl)
//! @brief Applies the frame's state on the Client
void
Frame::applyState(Client *cl)
{
	if (!cl)
		return;

	cl->setSticky(_sticky);
	cl->setMaximizedHorz(_state.maximized_horz);
	cl->setMaximizedVert(_state.maximized_vert);
	cl->setShade(_state.shaded);
	cl->setWorkspace(_workspace);
	cl->setLayer(_layer);

	// fix border / titlebar state
	cl->setBorder(_fw->hasBorder());
	cl->setTitlebar(_fw->hasTitlebar());
	// make sure the window gets synced with titlebar state
	XMoveWindow(_dpy, cl->getWindow(),
						  _fw->borderLeft(), _fw->borderTop() + _fw->getTitleHeight());

	cl->updateWmStates();
}

//! @fn    void updateTitles(void)
//! @brief Update the FrameWidget's list of titles.
void
Frame::updateTitles(void)
{
	_fw->clearTitleList();

	vector<Client*>::iterator it = _client_list.begin();
	for (; it != _client_list.end(); ++it)
		_fw->addTitle((*it)->getName());

	setActiveTitle();
}

//! @fn    void setActiveTitle(void)
//! @brief
void
Frame::setActiveTitle(void)
{
	if (_client_list.size()) {
		vector<Client*>::iterator it = _client_list.begin();
		for (unsigned int i = 0; it != _client_list.end(); ++it, ++i) {
			if ((*it) == _client) {
				_fw->setActiveTitle(i);
			}
		}
	} else
		_fw->setActiveTitle(0);
}

//! @fn    void activateNextClient(void)
//! @brief Makes the next client in the frame active (wraps)
void
Frame::activateNextClient(void)
{
	if (_client_list.size() < 2)
		return; // no client to switch to

	// find the active clients position
	vector<Client*>::iterator it =
		find(_client_list.begin(), _client_list.end(), _client);

	if (++it == _client_list.end())
		it = _client_list.begin();

	activateClient(*it);
}

//! @fn    void activatePrevClient(void)
//! @brief Makes the prev client in the frame active (wraps)
void
Frame::activatePrevClient(void)
{
	if (_client_list.size() < 2)
		return; // no client to switch to

	// find the active clients position
	vector<Client*>::iterator it =
		find(_client_list.begin(), _client_list.end(), _client);

	if (it == _client_list.begin())
		it = _client_list.end();
	--it;

	activateClient(*it);
}

//! @fn    void insertClient(Client *c, bool focus)
//! @brief Insert a client to the frame and sets it active
//! @param c Client to insert.
//! @param focus Defaults to true.
//! @param activate Defaults to true.
void
Frame::insertClient(Client *client, bool focus, bool activate)
{
	if (!client)
		return;
	_client_list.push_back(client);

	client->setFrame(this);
	_wm->setAtomLongValue(client->getWindow(),
												_wm->getPekwmAtoms()->getAtom(PEKWM_FRAME_ID), _id);

	client->filteredReparent(_fw->getWindow(),
													 _fw->borderLeft(),
													 _fw->borderTop() + _fw->getTitleHeight());

	if (!_client || activate) {
		activateClient(client, focus);
	} else {
		// I need to make sure it gets below the active client, so I just
		// lower it. If I wouldn't do this, it would become ontop of the focused
		// window if I changed workspace and back.
		XLowerWindow(_dpy, client->getWindow());
	}
	updateTitles();
}

//! @fn    void removeClient(Client *client)
//! @brief Removes the Client client from the frame
//! Removes the client client, and if the frame becomes empty
//! it'll delete itself otherwise it'll set the first client to active
//!
//! @param client Client to remove
void
Frame::removeClient(Client *client)
{
	if (!client)
		return;

	vector<Client*>::iterator it =
		find(_client_list.begin(), _client_list.end(), client);

	if (it != _client_list.end()) {
		// remove the old one
		it = _client_list.erase(it);
		client->setFrame(NULL);

		// Only move the client to the root if it's alive ( it's running )
		if (client->isAlive()) {
			updateClientSize(client);
			client->move(_gm.x, _gm.y + _fw->getTitleHeight());
			gravitate(client, false);
			client->filteredReparent(_scr->getRoot(),
															 client->getX(), client->getY());
		}

		// Activate a client, if we have any, else remove ourselves
		if (_client_list.size()) {
			if (_client == client) {
				if (it == _client_list.end())
					--it;

				activateClient(*it, false);
			}

			updateTitles();

		} else {
			delete this; // no clients, delete ourselves
		}
	}
}

//! @fn    void detachClient(Client *client)
//! @brief
void
Frame::detachClient(Client *client)
{
	if (!clientIsInFrame(client))
		return;

	if (_client_list.size() > 1) {
		removeClient(client);

		client->move(_gm.x, _gm.y);
		Frame *frame = new Frame(_wm, client);

		client->setFrame(frame);
		client->setWorkspace(_wm->getWorkspaces()->getActive());

		setFocused(false);
	}
}

//! @fn    void insertFrame(Frame* frame, bool focus, bool activate)
//! @brief Inserts a Frame into this frame.
//!
//! @param frame Frame* to get clients from.
//! @param focus Focus active client.
//! @param activate Insert clients behind or not
void
Frame::insertFrame(Frame* frame, bool focus, bool activate)
{
	if (!frame || (frame == this))
		return;

	Client *client = frame->getActiveClient();

	vector<Client*> *c_list = frame->getClientList();
	vector<Client*>::iterator it = c_list->begin();
	for (; it != c_list->end(); ++it) {
		insertClient(*it, false, false);
		(*it)->setWorkspace(_workspace);
	}

	c_list->clear();
	delete frame;

	if (activate)
		activateClient(client, focus);
}

//! @fn    bool clientIsInFrame(Client *c)
//! @brief Checks if Client c is contained in this Frame
//! @return true if the Client c is in the frame else false
bool
Frame::clientIsInFrame(Client *c)
{
	if (!c)
		return false;

	vector<Client*>::iterator it =
		find(_client_list.begin(), _client_list.end(), c);

	if (it != _client_list.end())
		return true;
	return false;
}

//! @fn    void activateClient(Client *client)
//! @brief Activates the Client _client
//! @param client Client to activate.
//! @param focus Defaults to true
void
Frame::activateClient(Client *client, bool focus)
{
	if (!client || !clientIsInFrame(client) || (client == _client))
		return; // no client to activate

	// sync the frame state with the client only if we already had a client
	if (_client)
		applyState(client);
	_client = client;

	updateSize();
	updatePosition();
#ifdef SHAPE
	if (_scr->hasShape()) { // reshape the frame.
		bool status = _fw->setShape(_client->getWindow(), _client->isShaped());
		_client->setShaped(status);
	}
#endif // SHAPE

	XRaiseWindow(_dpy, client->getWindow());
	_fw->raiseBorder();

	if (focus)
		giveInputFocus();

	setActiveTitle();

	_wm->getWorkspaces()->updateClientStackingList(true, true);
}

//! @fn    void activateClientFromPos(int x)
//! @brief
void
Frame::activateClientFromPos(int x)
{
	Client *client = getClientFromPos(x);
	if (client && (client != _client))
		activateClient(client);
}

//! @fn    void activateClientFromNum(unsigned int n)
//! @brief Activates the client numbered n in the frame.
//! @param n Client number to activate.
void
Frame::activateClientFromNum(unsigned int n)
{
	if (n >= _client_list.size())
		return;

	// Only activate if it's another client
	if (_client_list[n] != _client)
		activateClient(_client_list[n]);
}

//! @fn    void moveClientNext(void)
//! @brief Moves the active one step to the right in the frame
//! Moves the active one step to the right in the frame, this is used
//! to get more structure in frames with alot clients grouped
void
Frame::moveClientNext(void)
{
	if (_client_list.size() < 2)
		return;

	vector<Client*>::iterator it =
		find(_client_list.begin(), _client_list.end(), _client);

	if (it != _client_list.end()) {
		vector<Client*>::iterator n_it = it;
		if (++n_it < _client_list.end()) {
			*it = *n_it; // put the next client at the active clients place
			*n_it = _client; // move the active client to the next client

			// changed the order of the client, lets redraw the titlebar
			updateTitles();
		}
	}
}

//! @fn    void moveClientPrev(void)
//! @brief Moves the active one step to the left in the frame
//! Moves the active one step to the left in the frame, this is used
//! to get more structure in frames with alot clients grouped
void
Frame::moveClientPrev(void)
{
	vector<Client*>::iterator it =
		find(_client_list.begin(), _client_list.end(), _client);

	if (it != _client_list.end()) {
		vector<Client*>::iterator n_it = it;
		if (it > _client_list.begin()) {
			*it = *(--n_it); // move previous client to the active client
			*n_it = _client; // move active client back

			// changed the order of the client, lets redraw the titlebar
			updateTitles();
		}
	}
}

//! @fn    Client* getClientFromPos(unsigned int x)
//! @brief Searches the clien positioned at position x of the frame title
//! @param x Where in the title to look
//! @return Pointer to the Client found, if no Client found it returns NULL
Client*
Frame::getClientFromPos(unsigned int x)
{
	unsigned int pos = _fw->getButtonWidthL();
	unsigned int ew = _fw->getTabWidth();

	for (unsigned int i = 0; i < _client_list.size(); ++i, pos += ew) {
		if ((x >= pos) && (x <= (pos + ew)))
			return _client_list[i];
	}

	return NULL;
}

//! @fn    void toggleBorder(bool resize)
//! @brief Hides/Shows the border depending on _client
//! @param resize Defaults to true
void
Frame::toggleBorder(bool resize)
{
	// make sure the frame will remain visible
	if (_state.shaded && _fw->hasBorder() && !_fw->hasTitlebar())
		return;

	_fw->setBorder(!_fw->hasBorder());
	_client->setBorder(_fw->hasBorder());

	if (_fw->hasBorder()) {
		XMoveWindow(_dpy, _client->getWindow(),
								_fw->borderLeft(), _fw->borderTop() + _fw->getTitleHeight());
	} else {
		XMoveWindow(_dpy, _client->getWindow(),
								0, _fw->getTitleHeight());
	}

	if (resize) {
		_gm.width = _client->getWidth() + _fw->borderLeft() + _fw->borderRight();
		_gm.height = _client->getHeight() + _fw->getTitleHeight() +
			_fw->borderTop() + _fw->borderBottom();
	}
	updateSize();
}

//! @fn    void toggleTitlebar(bool resize)
//! @brief Hides/Shows the titlebar depending on _client
//! @param resize Defaults to true
void
Frame::toggleTitlebar(bool resize)
{
	// make sure the frame will remain visible
	if (_state.shaded && _fw->hasTitlebar() && !_fw->hasBorder())
		return;

	_fw->setTitlebar(!_fw->hasTitlebar());
	_client->setTitlebar(_fw->hasTitlebar());

	XMoveWindow(_dpy, _client->getWindow(),
							_fw->borderLeft(), _fw->borderTop() + _fw->getTitleHeight());

	if (resize) {
		_gm.height = _client->getHeight() + _fw->getTitleHeight() +
			_fw->borderTop() + _fw->borderBottom();
	}
	updateSize();
}

//! @fn    void toggleDecor(void)
//! @brief Toggles both the clients border and titlebar.
void
Frame::toggleDecor(void)
{
	// As we are toggeling the decor, which is both the border and titlebar
	// this could cause trouble if I looked at both before deciding which
	// to toggle, I check control the titlebar.
	if (_fw->hasTitlebar()) {
		_client->setTitlebar(false);
		_client->setBorder(false);
	} else {
		_client->setTitlebar(true);
		_client->setBorder(true);
	}

	if (_fw->hasTitlebar() != _client->hasTitlebar())
		toggleTitlebar();
	if (_fw->hasBorder() != _client->hasBorder())
		toggleBorder();
}

//! @fn    void updateSize(void)
//! @brief Resizes the frame and the frame title to the current size.
void
Frame::updateSize(void)
{
	_fw->resize(_gm.width, _state.shaded ? _real_height : _gm.height);

	updateClientSize(_client);
}

//! @fn    void updateSize(unsigned int w, unsigned int h)
//! @brief Resizes the frame and the frame title to the specified size.
//! @param w Width to set the frame to
//! @param h Height to set the frame to
void
Frame::updateSize(unsigned int width, unsigned int height)
{
	_gm.width = width;
	if (_state.shaded)
		_real_height = height;
	else
		_gm.height = height;

	updateSize();
}

//! @fn    void updateClientSize(Client *client)
//! @brief
void
Frame::updateClientSize(Client *client)
{
	client->resize(_gm.width - _fw->borderLeft() - _fw->borderRight(),
								 (_state.shaded ? _real_height : _gm.height) -
								 _fw->getTitleHeight() -
								 _fw->borderTop() - _fw->borderBottom());
}

//! @fn    void updatePosition(void)
//! @brief Moves the frame window to the current position.
void
Frame::updatePosition(void)
{
	_fw->move(_gm.x, _gm.y);
	_client->move(_gm.x, _gm.y + _fw->getTitleHeight());
}

//! @fn    void updatePosition(int x, int y)
//! @brief Moves the frame window to the specified position.
//! @param x New x position
//! @param y New y position
void
Frame::updatePosition(int x, int y)
{
	_gm.x = x;
	_gm.y = y;
	updatePosition();
}

//! @fn    void fixGeometry(bool harbour)
//! @brief Makes sure the frame doesn't cover any struts / the harbour.
bool
Frame::fixGeometry(bool harbour)
{
	Strut *strut = _scr->getStrut();

	Geometry head, before;
	head.x = strut->left;
	head.y = strut->top;
	head.width = _scr->getWidth() - head.x - strut->right;
	head.height = _scr->getHeight() - head.y - strut->bottom;

#ifdef HARBOUR
	if (harbour) {
		switch (_wm->getConfig()->getHarbourPlacement()) {
		case TOP:
			if (_wm->getHarbour()->getSize() > strut->top) {
				head.y = _wm->getHarbour()->getSize();
				head.height = _scr->getHeight() - head.y - strut->bottom;
			}
			break;
		case BOTTOM:
			if (_wm->getHarbour()->getSize() > strut->bottom)
				head.height = _scr->getHeight() - head.y - _wm->getHarbour()->getSize();
			break;
		case LEFT:
			if (_wm->getHarbour()->getSize() > strut->left) {
				head.x = _wm->getHarbour()->getSize();
				head.width = _scr->getWidth() - head.x - strut->right;
			}
			break;
		case RIGHT:
			if (_wm->getHarbour()->getSize() > strut->right)
				head.width = _scr->getWidth() - head.x - _wm->getHarbour()->getSize();
			break;
		}
	}
#endif // HARBOUR

	before = _gm;

	// Fix size
	if (_gm.width > head.width)
		_gm.width = head.width;
	if (_gm.height > head.height)
		_gm.height = head.height;

	// Fix position
	if (_gm.x < head.x)
		_gm.x = head.x;
	else if ((_gm.x + _gm.width) > (head.x + head.width))
		_gm.x = head.x + head.width - _gm.width;
	if (_gm.y < head.y)
		_gm.y = head.y;
	else if ((_gm.y + _gm.height) > (head.y + head.height))
		_gm.y = head.y + head.height - _gm.height;

	return (_gm != before);
}

ActionEvent*
Frame::findMouseButtonAction(unsigned int button, unsigned int mod,
														 MouseButtonType type,
														 list<ActionEvent>* actions)
{
	if (!actions)
		return NULL;

	list<ActionEvent>::iterator it = actions->begin();
	for (; it != actions->end(); ++it) {
		if ((it->sym == button) && (it->mod == mod) &&
				(it->type == unsigned(type))) {
			return &*it;
		}
	}

	return NULL;
}

//! @fn    void doMove(XMotionEvent *ev)
//! @brief Starts moving the window based on the XMotionEvent
void
Frame::doMove(XMotionEvent *ev)
{
	if (!ev || !_client->allowMove())
		return;

	if (!_scr->grabPointer(_scr->getRoot(), ButtonMotionMask|ButtonReleaseMask,
												 _scr->getCursor(ScreenInfo::CURSOR_MOVE)))
		return;

	Config *cfg = _wm->getConfig(); // convenience
	int resistance = signed(cfg->getWWFrame()); // convenience

	unsigned int c_width, c_height;
	calcSizeInCells(c_width, c_height);

	char buf[32];
	snprintf(buf, sizeof(buf), "%dx%d+%d+%d",
					 c_width, c_height, _gm.x, _gm.y);
	_wm->drawInStatusWindow(buf); // to make things less flickery
	_wm->showStatusWindow();
	_wm->drawInStatusWindow(buf);

	if (!cfg->getOpaqueMove()) {
		_scr->grabServer();
		_fw->drawOutline(_gm); // so that first clear will work
	}

	XEvent e;
	while (true) { // breaks when we get an ButtonRelease event
		XMaskEvent(_dpy, ButtonReleaseMask|PointerMotionMask, &e);

		switch (e.type) {
		case MotionNotify:
			if (!cfg->getOpaqueMove())
				_fw->drawOutline(_gm); // clear

			_gm.x = _old_cx + e.xmotion.x_root - _pointer_x;
			_gm.y = _old_cy + e.xmotion.y_root - _pointer_y;

			// I prefer having the EdgeSnap after the FrameSnap, feels better IMHO
			if (_wm->getConfig()->getFrameSnap())
				_wm->getWorkspaces()->checkFrameSnap(_gm, this);
			if(_wm->getConfig()->getEdgeSnap())
				checkEdgeSnap();


			snprintf(buf, sizeof(buf), "%dx%d+%d+%d",
							 c_width, c_height, _gm.x, _gm.y);
			_wm->drawInStatusWindow(buf);

			if (cfg->getOpaqueMove())
				_fw->move(_gm.x, _gm.y);
			else
				_fw->drawOutline(_gm);

			if (resistance) {
				int warp = (e.xmotion.x_root <= resistance) ? -1 : 0;
				if (!warp)
					warp = (e.xmotion.x_root >= signed(_scr->getWidth() - resistance)) ? 1 : 0;

				// found a workspace to warp to
				if (warp) {
					unsigned int x = resistance * 2;
					if (warp < 0)
						x = _scr->getWidth() - x;

					if (!cfg->getOpaqueMove())
						_fw->drawOutline(_gm); // clear

					if (_wm->warpWorkspace(warp, cfg->getWWFrameWrap(), false,
																 x, e.xmotion.y_root)) {

						// set new position of the frame
						_gm.x = _old_cx + x - _pointer_x;
						_gm.y = _old_cy + e.xmotion.y_root - _pointer_y;

						_fw->move(_gm.x, _gm.y);

						// warp client to the new workspace
						setWorkspace(_wm->getWorkspaces()->getActive());

						// make sure the client has focus
						_fw->setFocused(true);
						giveInputFocus();

						// make sure the status window is ontop of the frame
						_wm->showStatusWindow();
					}

					if (!cfg->getOpaqueMove())
						_fw->drawOutline(_gm);
				}
			}
			break;

		case ButtonRelease:
			if (!cfg->getOpaqueMove()) {
				_fw->drawOutline(_gm); // clear

				_scr->ungrabServer(true);
			}

			updatePosition();
			_wm->hideStatusWindow();

			_scr->ungrabPointer();
			return;

			break;
		}
	}
}

//! @fn    void doGroupingDrag(XMotionEvent *ev, Client *client, bool activate)
//! @brief Initiates grouping move, based on a XMotionEvent.
void
Frame::doGroupingDrag(XMotionEvent *ev, Client *client, bool behind)
{
	if (!client)
		return;

	int o_x, o_y;
	o_x = ev ? ev->x_root : 0;
	o_y = ev ? ev->y_root : 0;

	string name("Grouping ");
	if (client->getName()) {
		name += *client->getName();
	} else {
		name += "No Name";
	}

	if (!_scr->grabPointer(_scr->getRoot(), ButtonReleaseMask|PointerMotionMask,
											 None))
		return;

	_wm->drawInStatusWindow(name, o_x, o_y); // to make things less flickery
	_wm->showStatusWindow();
	_wm->drawInStatusWindow(name, o_x, o_y); // redraw after the window has mapped

	XEvent e;
	while (true) { // this breaks when we get an button release
		XMaskEvent(_dpy, PointerMotionMask|ButtonReleaseMask, &e);

		switch (e.type)  {
		case MotionNotify:
			// update the position
			o_x = e.xmotion.x_root;
			o_y = e.xmotion.y_root;

			_wm->drawInStatusWindow(name, o_x, o_y);
			break;

		case ButtonRelease:
			_wm->hideStatusWindow();
			_scr->ungrabPointer();

			Window win;
			int x, y;
			// find the frame we dropped the client on
			XTranslateCoordinates(_dpy, _scr->getRoot(), _scr->getRoot(),
														e.xmotion.x_root, e.xmotion.y_root,
														&x, &y, &win);

			Client *search_client = _wm->findClient(win);

			// if we found a client, and it's not in the current frame and
			// it has a "normal" ( make configurable? ) layer we group
			if (search_client && search_client->getFrame() &&
					!clientIsInFrame(search_client) &&
					(search_client->getLayer() > LAYER_BELOW) &&
					(search_client->getLayer() < LAYER_ONTOP)) {

				// if we currently have focus and the frame exists after we remove
				// this client we need to redraw it as unfocused
				bool focus = behind ? false : (_client_list.size() > 1);

				removeClient(client);

				search_client->getFrame()->insertClient(client, !behind, !behind);

				if (focus)
					setFocused(false);
			}  else if (_client_list.size() > 1) {
				// if we have more than one client in the frame detach this one
				removeClient(client);

				client->move(e.xmotion.x_root, e.xmotion.y_root);

				Frame *frame = new Frame(_wm, client);
				client->setFrame(frame);

				// make sure the client ends up on the current workspace
				client->setWorkspace(_wm->getWorkspaces()->getActive());

				// make sure it get's focus
				setFocused(false);
				frame->giveInputFocus();
			}

			return;
		}
	}
}

//! @fn    void doResize(XMotionEvent *ev)
//! @brief Initiates resizing of a window based on motion event.
void
Frame::doResize(XMotionEvent *ev)
{
	if (!ev)
		return;

	// figure out which part of the window we are in
	bool left = false, top = false;
	if (ev->x < signed(_gm.width / 2))
		left = true;
	if (ev->y < signed(_gm.height / 2))
		top = true;

	doResize(left, true, top, true);
}

//! @fn    void doResize(bool left, bool x, bool top, bool y)
//! @brief Resizes the frame by handling MotionNotify events.
void
Frame::doResize(bool left, bool x, bool top, bool y)
{
	if (!_client->allowResize())
		return;

	if (!_scr->grabPointer(_scr->getRoot(), ButtonMotionMask|ButtonReleaseMask,
												 _scr->getCursor(ScreenInfo::CURSOR_RESIZE)))
		return;

	Config *cfg = _wm->getConfig(); // convenience

	// Only grab if we want to
	if (_wm->getConfig()->getGrabWhenResize())
		_scr->grabServer();

	if (_state.shaded)
		shade(); // make sure the frame isn't shaded

	// Initialize variables
	int start_x, new_x;
	int start_y, new_y;
	unsigned int last_width, old_width;
	unsigned int last_height, old_height;

	start_x = new_x = left ? _gm.x : (_gm.x + _gm.width);
	start_y = new_y = top ? _gm.y : (_gm.y + _gm.height);
	last_width = old_width = _gm.width;
	last_height = old_height = _gm.height;

	// the basepoint of our window
	_old_cx = left ? (_gm.x + _gm.width) : _gm.x;
	_old_cy = top ? (_gm.y + _gm.height) : _gm.y;

	int pointer_x = _gm.x, pointer_y = _gm.y;
	_scr->getMousePosition(pointer_x, pointer_y);

	unsigned int c_width, c_height;
	calcSizeInCells(c_width, c_height);

	char buf[32];
	snprintf(buf, sizeof(buf), "%dx%d+%d+%d",
					 c_width, c_height, _gm.x, _gm.y);
	_wm->drawInStatusWindow(buf); // makes the window resize before we map
	_wm->showStatusWindow();
	_wm->drawInStatusWindow(buf);

	// draw the wire frame of the window, so that the first clear works.
	if (!cfg->getOpaqueResize())
		_fw->drawOutline(_gm);

	XEvent ev;
	while (true) { // breaks when we get an ButtonRelease event
		XMaskEvent(_dpy, ButtonPressMask|ButtonReleaseMask|ButtonMotionMask, &ev);

		switch (ev.type) {
		case MotionNotify:
			if (!cfg->getOpaqueResize())
				_fw->drawOutline(_gm); // clear

			if (x)
				new_x = start_x - pointer_x + ev.xmotion.x;
			if (y)
				new_y = start_y - pointer_y + ev.xmotion.y;

			recalcResizeDrag(new_x, new_y, left, top);

			calcSizeInCells(c_width, c_height);
			snprintf(buf, sizeof(buf), "%dx%d+%d+%d",
							 c_width, c_height, _gm.x, _gm.y);
			_wm->drawInStatusWindow(buf);

			// we need to do this everytime, not as in opaque when we update
			// when something has changed
			if (!cfg->getOpaqueResize())
				_fw->drawOutline(_gm);

			// Here's the deal, we try to be a bit conservative with redraws
			// when resizing, especially as drawing scaled pixmaps is _slow_
			if ((old_width != _gm.width) || (old_height != _gm.height)) {
				if (cfg->getOpaqueResize()) {
					updateSize();
					updatePosition();
				}
				old_width = _gm.width;
				old_height = _gm.height;
			}
		break;

		case ButtonRelease:
			if (!cfg->getOpaqueResize())
				_fw->drawOutline(_gm); // clear

			_wm->hideStatusWindow();

			_scr->ungrabPointer();

			// Make sure the state isn't set to maximized after we've resized.
			if (_state.maximized_horz || _state.maximized_vert) {
				_state.maximized_horz = false;
				_state.maximized_vert = false;
				_client->setMaximizedHorz(false);
				_client->setMaximizedVert(false);
				_client->updateWmStates();
			}

			if (!cfg->getOpaqueResize()) {
				updateSize();
				updatePosition();
			}

			if (_wm->getConfig()->getGrabWhenResize()) {
				_scr->ungrabServer(true);
			}
			return;
		}
	}
}

//! @fn    void recalcResizeDrag(int nx, int ny, bool left, bool top)
//! @brief Updates the width, height of the frame when resizing it.
void
Frame::recalcResizeDrag(int nx, int ny, bool left, bool top)
{
	unsigned int brdr_lr = _fw->borderLeft() + _fw->borderRight();
	unsigned int brdr_tb = _fw->borderTop() + _fw->borderBottom();

	if (left) {
		if (nx >= signed(_old_cx - brdr_lr))
			nx = _old_cx - brdr_lr - 1;
	} else {
		if (nx <= signed(_old_cx + brdr_lr))
			nx = _old_cx + brdr_lr + 1;
	}

	if (top) {
		if (ny >= signed(_old_cy - _fw->getTitleHeight() - brdr_tb))
			ny = _old_cy - _fw->getTitleHeight() - brdr_tb - 1;
	} else {
		if (ny <= signed(_old_cy + _fw->getTitleHeight() + brdr_tb))
			ny = _old_cy + _fw->getTitleHeight() + brdr_tb + 1;
	}

	unsigned int width = left ? (_old_cx - nx) : (nx - _old_cx);
	unsigned int height = top ? (_old_cy - ny) : (ny - _old_cy);

	if (width > _scr->getWidth())
		width = _scr->getWidth();
	if (height > _scr->getHeight())
		height = _scr->getHeight();

	width -= brdr_lr;
	height -= brdr_tb + _fw->getTitleHeight();
	_client->getIncSize(&width, &height, width, height);

	const XSizeHints *hints = _client->getXSizeHints();
	// check so we aren't overriding min or max size
	if (hints->flags & PMinSize) {
		if (signed(width) < hints->min_width)
			width = hints->min_width;
		if (signed(height) < hints->min_height)
			height = hints->min_height;
	}

	if (hints->flags & PMaxSize) {
		if (signed(width) > hints->max_width)
			width = hints->max_width;
		if (signed(height) > hints->max_height)
			height = hints->max_height;
	}

	_gm.width = width + brdr_lr;
	_gm.height = height + _fw->getTitleHeight() + brdr_tb;

	_gm.x = left ? (_old_cx - _gm.width) : _old_cx;
	_gm.y = top ? (_old_cy - _gm.height) : _old_cy;
}

#ifdef KEYS
//! @fn    void doKeyboardMoveResize(void)
//! @brief
void
Frame::doKeyboardMoveResize(void)
{
	if (!_scr->grabPointer(_scr->getRoot(), NoEventMask, None))
		return;
	if (!_scr->grabKeyboard(_scr->getRoot())) {
		_scr->ungrabPointer();
		return;
	}

	Geometry old_gm = _gm; // backup geometry if we cancel
	ActionEvent *ae;
	bool move_resize = true;
	bool opaque = (_wm->getConfig()->getOpaqueMove() &&
								 _wm->getConfig()->getOpaqueResize());

	list<Action>::iterator it;

	// TO-DO: Move this into separate method?
	unsigned int c_width, c_height;
	calcSizeInCells(c_width, c_height);

	char buf[32];
	snprintf(buf, sizeof(buf), "%dx%d+%d+%d",
					 c_width, c_height, _gm.x, _gm.y);
	_wm->drawInStatusWindow(buf); // to make things less flickery
	_wm->showStatusWindow();
	_wm->drawInStatusWindow(buf);

	if (!opaque) {
		_scr->grabServer();
		_fw->drawOutline(_gm); // so that the first clear works
	}

	XEvent ev;
	while (move_resize) { // breaks when we press the correct key
		XMaskEvent(_dpy, KeyPressMask, &ev);

		if ((ae = _wm->getKeyGrabber()->findMoveResizeAction(&ev.xkey))) {
			for (it = ae->action_list.begin(); it != ae->action_list.end(); ++it) {
				if (!opaque)
					_fw->drawOutline(_gm); // clear last position

				switch (it->action) {
				case MOVE_HORIZONTAL:
					_gm.x += it->param_i;
					if (opaque)
						_fw->move(_gm.x, _gm.y);
					break;
				case MOVE_VERTICAL:
					_gm.y += it->param_i;
					if (opaque)
						_fw->move(_gm.x, _gm.y);
					break;
				case RESIZE_HORIZONTAL:
					resizeHorizontal(it->param_i);
					if (opaque)
						updateSize();
					break;
				case RESIZE_VERTICAL:
					resizeVertical(it->param_i);
					if (opaque)
						updateSize();
					break;
				case MOVE_SNAP:
					_wm->getWorkspaces()->checkFrameSnap(_gm, this);
					checkEdgeSnap();
					if (opaque)
						_fw->move(_gm.x, _gm.y);
					break;
				case MOVE_CANCEL:
					_gm = old_gm; // restore position

					if (opaque) {
						updatePosition();
						updateSize();
					}

					move_resize = false;
					break;
				case MOVE_END:
					updateSize();
					updatePosition();
					move_resize = false;
					break;
				default:
					// do nothing
					break;
				}

				if (move_resize && !opaque)
					_fw->drawOutline(_gm); // update the positionc

				calcSizeInCells(c_width, c_height);
				snprintf(buf, sizeof(buf), "%dx%d+%d+%d",
								 c_width, c_height, _gm.x, _gm.y);
				_wm->drawInStatusWindow(buf);
			}
		}

	}

	_wm->hideStatusWindow();

	if (!opaque)
		_scr->ungrabServer(true);

	_scr->ungrabKeyboard();
	_scr->ungrabPointer();
}
#endif // KEYS

//! @fn    moveToEdge(Edge edge)
//! @brief Moves the frame to one of the edges of the screen.
void
Frame::moveToEdge(Edge edge)
{
	Geometry head;
#ifdef XINERAMA
	unsigned int head_nr =
		_scr->getHead(_gm.x + (_gm.width / 2), _gm.y + (_gm.height / 2)); 
	_scr->getHeadInfo(head_nr, head);
#else // !XINERAMA
	head.x = 0;
	head.y = 0;
	head.width = _scr->getWidth();
	head.height = _scr->getHeight();
#endif // XINERAMA

	switch (edge) {
	case TOP_LEFT:
		_gm.x = head.x;
		_gm.y = head.y;
		break;
	case TOP_EDGE:
		_gm.x = head.x + ((head.width - _gm.width) / 2);
		_gm.y = head.y;
		break;
	case TOP_RIGHT:
		_gm.x = head.x + head.width - _gm.width;
		_gm.y = head.y;
		break;
	case RIGHT_EDGE:
		_gm.x = head.x + head.width - _gm.width;
		_gm.y = head.y + ((head.height - _gm.height) / 2);
		break;
	case BOTTOM_RIGHT:
		_gm.x = head.x + head.width - _gm.width;
		_gm.y = head.y + head.height - _gm.height;
		break;
	case BOTTOM_EDGE:
		_gm.x = head.x + ((head.width - _gm.width) / 2);
		_gm.y = head.y + head.height - _gm.height;
		break;
	case BOTTOM_LEFT:
		_gm.x = head.x;
		_gm.y = head.y + head.height - _gm.height;
		break;
	case LEFT_EDGE:
		_gm.x = head.x;
		_gm.y = head.y + ((head.height - _gm.height) / 2);
		break;
	case CENTER:
		_gm.x = head.x + ((head.width - _gm.width) / 2);
		_gm.y = head.y + ((head.height - _gm.height) / 2);
	default:
		// DO NOTHING
		break;
	}

	updatePosition();
}

//! @fn    void resizeHorizontal(int size_diff)
//! @brief In/Decreases the size of the window horizontally
//! @param size_diff How much to resize
void
Frame::resizeHorizontal(int size_diff)
{
	if (!size_diff || !_client->allowResize())
		return;

	if (_client->isShaded())
		shade();

	unsigned int width = _gm.width;
	unsigned int client_width = width - _fw->borderLeft() - _fw->borderRight();

	XSizeHints *size_hints = _client->getXSizeHints();

	// If we have a a ResizeInc hint set, let's use it instead of the param
	if (size_hints->flags&PResizeInc) {
		if (size_diff > 0) {	// increase the size
			_gm.width += size_hints->width_inc;
			// only decrease if we are over 0 pixels in the end
		} else if (client_width > unsigned(size_hints->width_inc)) {
			_gm.width -= size_hints->width_inc;
		}
	} else if (signed(client_width + size_diff) > 0) {
		_gm.width += size_diff;
	}

	if (width > _gm.width)
		client_width -= width - _gm.width;
	else if (width < _gm.width)
		client_width += _gm.width - width;

	// check if we overide min/max size hints
	if (size_diff > 0) {
		if ((size_hints->flags&PMaxSize) &&
				(client_width > unsigned(size_hints->max_width))) {
			_gm.width -= client_width - size_hints->max_width;
		}
	} else if ((size_hints->flags&PMinSize) &&
						 (client_width < unsigned(size_hints->min_width))) {
		_gm.width += size_hints->min_width - client_width;
	}

	if (_state.maximized_horz) {
		_state.maximized_horz = false;
		_client->setMaximizedHorz(false);
		_client->updateWmStates();
	}
}

//! @fn    void resizeVertical(int size_diff)
//! @brief In/Decreases the size of the window vertically
//! @param size_diff How much to resize
void
Frame::resizeVertical(int size_diff)
{
	if (!_client->allowResize() || !size_diff)
		return;

	if (_client->isShaded())
		shade();

	unsigned int height = _gm.height;
	unsigned int client_height = height - _fw->borderTop() - _fw->borderBottom();

	XSizeHints *size_hints = _client->getXSizeHints();

	// If we have a a ResizeInc hint set, let's use it instead of the param
	if (size_hints->flags&PResizeInc) {
		if (size_diff > 0) {	// increase the size
			_gm.height += size_hints->height_inc;
			// only decrease if we are over 0 pixels in the end
		} else if (client_height > unsigned(size_hints->height_inc)) {
			_gm.height -= size_hints->height_inc;
		}
	} else if (signed(client_height + size_diff) > 0) {
		_gm.height += size_diff;
	}

	if (height > _gm.height)
		client_height -= height - _gm.height;
	else if (height < _gm.height)
		client_height += _gm.height - height;

	// check if we overide min/max size hints
	if (size_diff > 0) {
		if ((size_hints->flags&PMaxSize) &&
				(client_height > unsigned(size_hints->max_height))) {
			_gm.height -= client_height - size_hints->max_height;
		}
	} else if ((size_hints->flags&PMinSize) &&
						 (client_height < unsigned(size_hints->min_height))) {
		_gm.height += size_hints->min_height - client_height;
	}

	if (_state.maximized_vert) {
		_state.maximized_vert = false;
		_client->setMaximizedVert(false);
		_client->updateWmStates();
	}
}

//! @fn    void raise(void)
//! @brief Moves the frame ontop of all windows
void
Frame::raise(void)
{
	_wm->getWorkspaces()->raise(this);
	_wm->getWorkspaces()->updateClientStackingList(false, true);
}

//! @fn    void lower(void)
//! @brief Moves the frame beneath all windows
void
Frame::lower(void)
{
	_wm->getWorkspaces()->lower(this);
	_wm->getWorkspaces()->updateClientStackingList(false, true);
}

//! @fn    void shade(void)
//! @brief Toggles the frames shade state
void
Frame::shade(void)
{
	// we won't shade windows like gkrellm and xmms
	if (!_fw->hasBorder() && !_fw->hasTitlebar())
		return;

	_fw->shade();

	if (_state.shaded) {
		_gm.height = _real_height;
	} else {
		_real_height = _gm.height;
		_gm.height = _fw->getHeight();
	}

	_state.shaded = !_state.shaded;

	_client->setShade(_state.shaded);
	_client->updateWmStates();
}

//! @fn    void stick(void)
//! @brief Toggles the Frame's sticky state
void
Frame::stick(void)
{
	_client->setSticky(_sticky); // TO-DO: FRAME
	_client->stick();

	_sticky = !_sticky;

	// make sure it's visible/hidden
	setWorkspace(_wm->getWorkspaces()->getActive());
}

//! @fn    void maximize(bool horz, bool vert)
//! @brief Toggles current clients max size
void
Frame::maximize(bool horz, bool vert)
{
	if (_client->getTransientWindow())
		return;	// We don't maximize transients

	// this means full (de)maximization
	if (horz == vert) {
		if (_state.maximized_horz != _state.maximized_vert) {
			horz = !_state.maximized_horz;
			vert = !_state.maximized_vert;
		}
	}

	XSizeHints *size_hint = _client->getXSizeHints(); // convenience
#ifdef XINERAMA
		Geometry head;
		unsigned int head_nr = 0;

		if (_scr->hasXinerama()) {
			head_nr =
				_scr->getHead(_gm.x + (_gm.width / 2), _gm.y + (_gm.height / 2));
		}
		_scr->getHeadInfo(head_nr, head);
#endif // XINERAMA

	if (_state.shaded)
		shade();

	if (horz && _client->allowMaximizeHorz()) {
		if (_state.maximized_horz) { // demaximize
			_gm.x = _old_gm.x;
			_gm.width = _old_gm.width;

		} else { // maximize
			unsigned int h_decor = _fw->borderLeft() + _fw->borderRight();

			_old_gm.x = _gm.x;
			_old_gm.width = _gm.width;

#ifdef XINERAMA
			_gm.x = head.x;
			_gm.width = head.width;
#else // !XINERAMA
			_gm.x = 0;
			_gm.width = _scr->getWidth();
#endif // XINERAMA

			if ((size_hint->flags&PMaxSize) &&
					(_gm.width > (size_hint->max_width + h_decor))) {
				_gm.width = size_hint->max_width + h_decor;
			}

			if (size_hint->flags&PResizeInc) { // conform to width_inc
				int b_x = (size_hint->flags&PBaseSize)
					? size_hint->base_width
					: (size_hint->flags&PMinSize) ? size_hint->min_width : 0;

				_gm.width -= h_decor;
				_gm.width -= (_gm.width - b_x) % size_hint->width_inc;
				_gm.width += h_decor;
			}
		}

		_state.maximized_horz = !_state.maximized_horz;
		_client->setMaximizedHorz(_state.maximized_horz);
	}

	if (vert && _client->allowMaximizeVert()) {
		if (_state.maximized_vert) { // Demaximize
			_gm.y = _old_gm.y;
			_gm.height = _old_gm.height;

		} else { // maximize
			unsigned int v_decor = _fw->getTitleHeight() +
				_fw->borderTop() + _fw->borderBottom();

			_old_gm.y = _gm.y;
			_old_gm.height = _gm.height;

#ifdef XINERAMA
			_gm.y = head.y;
			_gm.height = head.height;
#else // !XINERAMA
			_gm.y = 0;
			_gm.height = _scr->getHeight();
#endif // XINERAMA

			if ((size_hint->flags&PMaxSize) &&
					(_gm.height > (size_hint->max_height + v_decor))) {
				_gm.height = size_hint->max_height + v_decor;
			}

			if (size_hint->flags&PResizeInc) { // conform to height_inc
				int b_y = (size_hint->flags&PBaseSize)
					? size_hint->base_height
					: (size_hint->flags&PMinSize) ? size_hint->min_height : 0;

				_gm.height -= v_decor;
				_gm.height -= (_gm.height - b_y) % size_hint->height_inc;
				_gm.height += v_decor;
			}
		}

		_state.maximized_vert = !_state.maximized_vert;
		_client->setMaximizedVert(_state.maximized_vert);
	}

#ifdef HARBOUR
	fixGeometry(!_wm->getConfig()->getHarbourMaximizeOver());
#else // !HARBOUR
	fixGeometry(false);
#endif // HARBOUR

	updateSize();
	updatePosition();

	_client->updateWmStates();
}

//! @fn    void alwaysOnTop(void);
//! @brief
void
Frame::alwaysOnTop(void)
{
	_client->alwaysOnTop(_client->getLayer() < LAYER_ONTOP);
	_layer = _client->getLayer();

	raise();
}

//! @fn    void alwaysBelow(void)
//! @brief
void
Frame::alwaysBelow(void)
{
	_client->alwaysBelow(_client->getLayer() > LAYER_BELOW);
	_layer = _client->getLayer();

	lower();
}

//! @fn    void readAutoprops(unsigned int type)
//! @brief Reads autoprops for the active client.
//! @param type Defaults to APPLY_ON_RELOAD
void
Frame::readAutoprops(unsigned int type)
{
	if ((type != APPLY_ON_RELOAD) && (type != APPLY_ON_WORKSPACE))
		return;

	_class_hint->title = *_client->getName();
	AutoProperty *data =
		_wm->getAutoProperties()->findAutoProperty(_class_hint, _workspace, type);
	_class_hint->title = "";

	if (!data)
		return;

	// Set the correct group of the window
	_class_hint->group = data->group_name;

	// TO-DO: Change this behaviour?
	if ((_class_hint == _client->getClassHint()) &&
			(_client->getTransientWindow() &&
			 !data->isApplyOn(APPLY_ON_TRANSIENT))) {
		return;
	}

	if (data->isMask(AP_STICKY) && (_sticky != data->sticky))
		stick();
	if (data->isMask(AP_ICONIFIED) && (_iconified != data->iconified)) {
		if (_iconified) mapWindow();
		else iconify();
	}
	if (data->isMask(AP_WORKSPACE)) {
		// I do this to avoid coming in an eternal loop.
		if (type == APPLY_ON_WORKSPACE)
			_workspace = data->workspace;
		else if (_workspace != data->workspace)
			setWorkspace(data->workspace); // TO-DO: FIX, false); // do not read autoprops again
	}			

	if (data->isMask(AP_SHADED) && (_state.shaded != data->shaded))
		shade();
	if (data->isMask(AP_LAYER) && (data->layer <= LAYER_MENU)) {
		_client->setLayer(data->layer);
		raise(); // restack the frame
	}
	if (data->isMask(AP_GEOMETRY)) {
		// Read size before position so negative position works
		if (data->gm_mask&(WidthValue|HeightValue)) {
			if (data->gm_mask&WidthValue)
				_gm.width = data->gm.width;
			if (data->gm_mask&HeightValue)
				_gm.height = data->gm.height;
			updateSize();
		}
		// Read position
		if (data->gm_mask&(XValue|YValue)) {
			if (data->gm_mask&XValue) {
				_gm.x = data->gm.x;
				if (data->gm_mask&XNegative)
					_gm.x += _scr->getWidth() - _gm.width;
			}
			if (data->gm_mask&YValue) {
				_gm.y = data->gm.y;
				if (data->gm_mask&YNegative)
					_gm.y += _scr->getHeight() - _gm.height;
			}
			updatePosition();
		}
	}
	if (data->isMask(AP_BORDER) && (_fw->hasBorder() != data->border))
		toggleBorder();
	if (data->isMask(AP_TITLEBAR) && (_fw->hasTitlebar() != data->titlebar))
		toggleTitlebar();

	// TO-DO: What about the non-active clients?
	if (data->isMask(AP_SKIP))
		_client->_state.skip = data->skip;
}

#ifdef MENUS
void
Frame::showWindowMenu(void)
{
	ActionMenu* menu = (ActionMenu*) _wm->getMenu(WINDOWMENU_TYPE);
	if (menu->isMapped()) {
		menu->unmapAll(); 
	} else {
		menu->setClient(_client);
		menu->mapUnderMouse();
		menu->giveInputFocus();
	}
}
#endif // MENUS

//! @fn    void checkEdgeSnap(void)
//! @brief
void
Frame::checkEdgeSnap(void)
{
	int snap = _wm->getConfig()->getEdgeSnap(); // convenience
#ifdef HARBOUR
	unsigned int h_size = _wm->getHarbour()->getSize(); // convenience
#endif // HARBOUR

	Geometry head;
#ifdef XINERAMA
	_scr->getHeadInfo(_scr->getHead(_gm.x, _gm.y), head);
#ifdef HARBOUR
	switch (_wm->getConfig()->getHarbourPlacement()) {
	case TOP:
		head.y += h_size;
	case BOTTOM:
		head.height -= h_size;
		break;
	case LEFT:
		head.x += h_size;
	case RIGHT:
		head.width -= h_size;
		break;
	}
#endif // HARBOUR
#else // !XINERAMA
	head.x = _scr->getStrut()->left;
	head.y = _scr->getStrut()->top;
	head.width = _scr->getWidth() - head.x - _scr->getStrut()->right;
	head.height = _scr->getHeight() - head.y - _scr->getStrut()->bottom;
#ifdef HARBOUR
	switch (_wm->getConfig()->getHarbourPlacement()) {
	case TOP:
		if (h_size > _scr->getStrut()->top) {
			head.y = h_size;
			head.height = _scr->getHeight() - head.y - _scr->getStrut()->bottom;
		}
		break;
	case BOTTOM:
		if (h_size > _scr->getStrut()->bottom)
			head.height = _scr->getHeight() - head.y - h_size;
		break;
	case LEFT:
		if (h_size > _scr->getStrut()->left) {
			head.x = h_size;
			head.width = _scr->getWidth() - head.x - _scr->getStrut()->right;
		}
		break;
	case RIGHT:
		if (h_size > _scr->getStrut()->right)
			head.width = _scr->getWidth() - head.x - h_size;;
		break;
	}
#endif // HARBOUR
#endif // XINERAMA

	// Move beyond edges of screen
	if (_gm.x == signed(head.x + head.width - _gm.width)) {
		_gm.x = head.x + head.width - _gm.width + 1;
	} else if (_gm.x == head.x) {
		_gm.x = head.x - 1;
	}
	if (_gm.y == signed(head.y + head.height - snap)) {
		_gm.y = head.y + head.height - snap - 1;
	} else if (_gm.y == head.y) {
		_gm.y = head.y - 1;
	}

	// Snap to edges of screen
	if ((_gm.x >= (head.x - snap)) && (_gm.x <= (head.x + snap))) {
		_gm.x = head.x;
	} else if ((_gm.x + _gm.width) >= (head.x + head.width - snap) &&
						 ((_gm.x + _gm.width) <= (head.x + head.width + snap))) {
		_gm.x = head.x + head.width - _gm.width;
	}
	if ((_gm.y >= (head.y - snap)) && (_gm.y <= (head.y + snap))) {
		_gm.y = head.y;
	} else if (((_gm.y + _fw->getHeight()) >= (head.y + head.height - snap)) &&
						 ((_gm.y + _fw->getHeight()) <= (head.y + head.height + snap))) {
		_gm.y = head.y + head.height - _fw->getHeight();
	}
}

//! @fn    void calcSizeInCells(unsigned int &width, unsigned int &height)
//! @brief Figure out how large the frame is in cells.
void
Frame::calcSizeInCells(unsigned int &width, unsigned int &height)
{
	const XSizeHints *hints = _client->getXSizeHints();

	if (hints->flags&PResizeInc) {
		width = (_gm.width -
						 _fw->borderLeft() - _fw->borderRight()) / hints->width_inc;
		height = (_gm.height - _fw->getTitleHeight()
							- _fw->borderTop() - _fw->borderBottom()) / hints->height_inc;
	} else {
		width = _gm.width;
		height = _gm.height;
	}
}

//! @fn    ActionEvent* handleButtonEvent(XButtonEvent *e)
//! @brief Handle buttons presses.
//! Handle button presses, first searches for titlebar buttons if no were
//! found titlebar actions is performed
//!
//! @param e XButtonEvent to examine
ActionEvent*
Frame::handleButtonEvent(XButtonEvent *e)
{
#ifdef MENUS
	if (e->type == ButtonPress)
		_wm->getMenu(WINDOWMENU_TYPE)->unmapAll();
#endif // MENUS

	// Remove the button from the state
	e->state &= ~Button1Mask & ~Button2Mask & ~Button3Mask
		& ~Button4Mask & ~Button5Mask;
	// Also remove NumLock, ScrollLock and CapsLock
	e->state &= ~_scr->getNumLock() & ~_scr->getScrollLock() & ~LockMask;

	ActionEvent *ae = NULL;

	// First try to do something about frame buttons

	Button *button; // used for searching titlebar button
	if (_button) {
		if (e->type == ButtonRelease) {
			if (_button == _fw->findButton(e->subwindow)) {
				ae = _button->findAction(e);

				// We don't want to execute this both on press and release!
				if (ae && (ae->isOnlyAction(RESIZE)))
					ae = NULL;
			}

			// restore the pressed buttons state
			if (_focused) {
				_button->setState(BUTTON_FOCUSED);
			}	else {
				_button->setState(BUTTON_UNFOCUSED);
			}
			_button = NULL;
		}

	} else if (e->subwindow && (button = _fw->findButton(e->subwindow))) {
		if (e->type == ButtonPress) {
			button->setState(BUTTON_PRESSED);

			ae = button->findAction(e);

			// If the button is used for resizing the window we want to be able
			// to resize it directly when pressing the button and not have to
			// wait until we release it, therefor this execption
			if (ae && ae->isOnlyAction(RESIZE)) {
				button->setState(_focused ? BUTTON_FOCUSED : BUTTON_UNFOCUSED);
			} else {
				_button = button; // set pressed button

				ae = NULL; // we'll execute actions on release ( except resize ) 
			}
		}
	} else {
		MouseButtonType mb = BUTTON_PRESS;

		// Used to compute the pointer position on click
		// used in the motion handler when doing a window move.
		_old_cx = _gm.x;
		_old_cy = _gm.y;
		_pointer_x = e->x_root;
		_pointer_y = e->y_root;

		// Allow us to get clicks from anywhere on the window.
		XAllowEvents(_dpy, ReplayPointer, CurrentTime);

		// Handle clicks on the Frames title
		if (e->window == _fw->getTitleWindow()) {
			if (e->type == ButtonRelease) {
				// first we check if it's a double click
				if ((e->time - _wm->getLastFrameClick(e->button - 1)) <
						_wm->getConfig()->getDoubleClickTime()) {

					_wm->setLastFrameClick(e->button - 1, 0);
					mb = BUTTON_DOUBLE;

				} else {
					_wm->setLastFrameClick(e->button - 1, e->time);
					mb = BUTTON_RELEASE;
				}
			}

			ae = findMouseButtonAction(e->button, e->state, mb,
																 _wm->getConfig()->getMouseFrameList());

			// Clicks on the Clients window
			// NOTE: If the we're matching against the subwindow we need to make
			// sure that the state is 0, meaning we didn't have any modifier
			// because if we don't care about the modifier we'll get two actions
			// performed when using modifiers.
		}	else if ((e->window == _client->getWindow()) ||
							 (!e->state && (e->subwindow == _client->getWindow()))) {
			if (e->type == ButtonRelease)
				mb = BUTTON_RELEASE;

			ae = findMouseButtonAction(e->button, e->state, mb,
																 _wm->getConfig()->getMouseClientList());

			// Border clicks that migh initiate resizing
		} else if ((e->window == _fw->getWindow()) && e->subwindow &&
							 (e->subwindow != _fw->getTitleWindow())) {
			if (e->type == ButtonPress) {

				// and do the testing by looking at the subwindow instead.
				bool x = false, y = false;
				bool left = false, top = false;
				bool resize = true;

				switch (_fw->getBorderPosition(e->subwindow)) {
				case BORDER_TOP_LEFT:
					x = y = left = top = true;
					break;
				case BORDER_TOP:
					y = top = true;
					break;
				case BORDER_TOP_RIGHT:
					x = y = top = true;
					break;
				case BORDER_LEFT:
					x = left = true;
					break;
				case BORDER_RIGHT:
					x = true;
					break;
				case BORDER_BOTTOM_LEFT:
					x = y = left = true;
					break;
				case BORDER_BOTTOM:
					y = true;
					break;
				case BORDER_BOTTOM_RIGHT:
					x = y = true;
					break;
				default:
					resize = false;
					break;
				}

				if (resize && !_state.shaded) {
					doResize(left, x, top, y);
				}
			}
		}
	}

	return ae;
}

//! @fn    void handleMotionNotifyEvent(XMotionEvent *ev)
//! @brief Handles Motion Events, if we have a button pressed nothing happens,
ActionEvent*
Frame::handleMotionNotifyEvent(XMotionEvent *ev)
{
	// This is true when we have a title button pressed and then we don't want
	// to be able to drag windows around, therefore we ignore the event
	if (_button)
		return NULL;

	unsigned int button = 0;

	// Figure out wich button we have pressed
	if (ev->state&Button1Mask)
		button = BUTTON1;
	else if (ev->state&Button2Mask)
		button = BUTTON2;
	else if (ev->state&Button3Mask)
		button = BUTTON3;
	else if (ev->state&Button4Mask)
		button = BUTTON4;
	else if (ev->state&Button5Mask)
		button = BUTTON5;

	// Remove the button from the state so that I can match it with == on
	// the mousebutton mod
	ev->state &= ~Button1Mask & ~Button2Mask & ~Button3Mask
		& ~Button4Mask & ~Button5Mask;
	// Also remove NumLock, ScrollLock and CapsLock
	ev->state &= ~_scr->getNumLock() & ~_scr->getScrollLock() & ~LockMask;

	ActionEvent *ae = NULL;
	if (ev->window == _fw->getTitleWindow()) {
		ae = findMouseButtonAction(button, ev->state, BUTTON_MOTION,
															 _wm->getConfig()->getMouseFrameList());
	} else if (ev->window == _client->getWindow()) {
		ae = findMouseButtonAction(button, ev->state, BUTTON_MOTION,
															 _wm->getConfig()->getMouseClientList());
	}

	// check motion threshold
	if (ae && ae->threshold) {
		if (((ev->x_root > _pointer_x)
					? (ev->x_root > (_pointer_x + signed(ae->threshold)))
					: (ev->x_root < (_pointer_x - signed(ae->threshold)))) ||
				((ev->y_root > _pointer_y)
					? (ev->y_root > (_pointer_y + signed(ae->threshold)))
					: (ev->y_root < (_pointer_y - signed(ae->threshold)))))
			return ae;
		else
			return NULL;
	}

	return ae;
}

//! @fn    void gravitate(Client *client, bool apply)
//! @brief
void
Frame::gravitate(Client *client, bool apply)
{

	if (!_fw->hasTitlebar())
		return;

	int gravity = NorthWestGravity;
	if (client->getXSizeHints()->flags&PWinGravity)
		gravity = client->getXSizeHints()->win_gravity;

	int dy = _fw->getTitleHeight();  
   
	switch (gravity) {  
	case NorthWestGravity:  
	case NorthEastGravity:  
	case NorthGravity:  
		// do nothing, it's ok this way  
		break;  
	case CenterGravity:  
		dy /= 2;  
		break;  
	default:  
		dy = 0; // no gravity, set it to 0  
		break;  
	}  
   
	if (apply) // apply gravity  
		client->_gm.y += dy;  
	else // remove gravity  
		client->_gm.y -= dy;  
}

// Below this Client message handling is done

//! @fn    void handleConfigureRequest(XConfigureRequestEvent *ev, Client *client)
//! @brief Handle XConfgiureRequestEvents
void
Frame::handleConfigureRequest(XConfigureRequestEvent *ev, Client *client)
{
	if (client != _client)
		return; // only handle the active client's events

#ifdef DEBUG
	cerr << __FILE__ << "@" << __LINE__ << ": "
			 << "handleConfigureRequest(" << ev->window << ")" << endl
			 << "x: " << ev->x << " y: " << ev->y << endl
			 << "width: " << ev->width << " height: " << ev->height << endl
			 << "above: " << ev->above << endl
			 << "detail: " << ev->detail << endl << endl;
#endif // DEBUG

	if (ev->value_mask&(CWX|CWY|CWWidth|CWHeight)) {
		// Update the geometry so that it matches the frame
		if (ev->value_mask&CWX)
			_gm.x = ev->x - _fw->borderLeft();
		if (ev->value_mask&CWY)
			_gm.y = ev->y - _fw->borderTop() - _fw->getTitleHeight();
		if (ev->value_mask&CWWidth)
			_gm.width = ev->width + _fw->borderLeft() + _fw->borderRight();
		if (ev->value_mask&CWHeight)
			_gm.height = ev->height + _fw->borderTop() + _fw->borderBottom() +
				_fw->getTitleHeight();

		updateSize();
		updatePosition();

		// can't be shaded after the resize
		_state.shaded = false;

#ifdef SHAPE
		if (ev->value_mask&(CWWidth|CWHeight)) {
			bool status = _fw->setShape(_client->getWindow(), _client->isShaped());
			_client->setShaped(status);
		}
#endif // SHAPE
	}

	// update the stacking
	if (ev->value_mask&CWStackMode) {
		if (ev->value_mask&CWSibling) {
			switch(ev->detail) {
			case Above:
				_wm->getWorkspaces()->stackAbove(this, ev->above);
				break;
			case Below:
				_wm->getWorkspaces()->stackBelow(this, ev->above);
				break;
			case TopIf:
			case BottomIf:
				// TO-DO: What does occlude mean?
				break;
			}
		} else {
			switch(ev->detail) {
			case Above:
				raise();
				break;
			case Below:
				lower();
				break;
			case TopIf:
			case BottomIf:
				// TO-DO: Why does the manual say that it should care about siblings
				// even if we don't have any specified?
				break;
			}
		}
	}
}

//! @fn    void handleClientMessage(XClientMessage *ev, Client *client)
//! @brief
void
Frame::handleClientMessage(XClientMessageEvent *ev, Client *client)
{
	EwmhAtoms *ewmh = _wm->getEwmhAtoms(); // convenience

	bool add = false, remove = false, toggle = false;

	if (ev->message_type == ewmh->getAtom(STATE)) {
		if(ev->data.l[0]== NET_WM_STATE_REMOVE) remove = true;
		else if(ev->data.l[0]== NET_WM_STATE_ADD) add = true;
		else if(ev->data.l[0]== NET_WM_STATE_TOGGLE)	toggle = true;

		// actions that only is going to be applied on the active client
		if (client == _client) {
		// There is no modal support in pekwm yet
//		if ((ev->data.l[1] == (long) ewmh->getAtom(STATE_MODAL)) ||
//			(ev->data.l[2] == (long) ewmh->getAtom(STATE_MODAL))) {
//			is_modal=true;
//		}

			if ((ev->data.l[1] == long(ewmh->getAtom(STATE_STICKY))) ||
					(ev->data.l[2] == long(ewmh->getAtom(STATE_STICKY)))) {
				// Well, I'm setting the desktop because it updates the desktop state
				// because if we are sticky we appear on all. Also, if we unstick
				// we should be on the current desktop
				if (add && !_sticky) stick();
				else if (remove && _sticky) stick();
				else if (toggle) stick();

				setWorkspace(_wm->getWorkspaces()->getActive());
			}

			if ((ev->data.l[1] == long(ewmh->getAtom(STATE_MAXIMIZED_HORZ))) ||
					(ev->data.l[2] == long(ewmh->getAtom(STATE_MAXIMIZED_HORZ)))) {
				if (add) _state.maximized_horz = false;
				else if (remove) _state.maximized_horz = true;
				maximize(true, false);
			}

			if ((ev->data.l[1] == long(ewmh->getAtom(STATE_MAXIMIZED_VERT))) ||
					(ev->data.l[2] == long(ewmh->getAtom(STATE_MAXIMIZED_VERT)))) {
				if (add) _state.maximized_vert = false;
				else if (remove) _state.maximized_vert = true;
				maximize(false, true);
			}

			if ((ev->data.l[1] == long(ewmh->getAtom(STATE_SHADED))) ||
					(ev->data.l[2] == long(ewmh->getAtom(STATE_SHADED)))) {
				if (add) _state.shaded = false;
				else if (remove) _state.shaded = true;

				shade();
			}

			if ((ev->data.l[1] == long(ewmh->getAtom(STATE_HIDDEN))) ||
					(ev->data.l[2] == long(ewmh->getAtom(STATE_HIDDEN)))) {
				if (add && !_iconified) iconify();
				else if (remove && _iconified) mapWindow();
				else if (toggle) {
					if (_iconified) mapWindow();
					else iconify();
				}
			}

 			//if ((ev->data.l[1] == long(ewmh->getAtom(state_fullscreen))) ||
 			//		(ev->data.l[2] == long(ewmh->getAtom(state_fullscreen)))) {
 			//	// TO-DO: Add support for toggling net_wm_state_fullscreen
 			//}
		}

		/*
		if ((ev->data.l[1] == long(ewmh->getAtom(state_skip_taskbar)) ||
				(ev->data.l[2] == long(ewmh->getAtom(state_skip_taskbar))) {
		if (state_add) m_state.skip_taskbar = true;
			else if (state_remove) m_state.skip_taskbar = false;
			else if (state_toggle)
				m_state.skip_taskbar = (m_state.skip_taskbar) ? true : false;
		}

		if ((ev->data.l[1] == long(ewmh->getAtom(state_skip_pager)) ||
				(ev->data.l[2] == long(ewmh->getAtom(state_skip_pager))) {
			if (state_add) m_state.skip_pager = true;
			else if (state_remove) m_state.skip_pager = false;
			else if (state_toggle)
				m_state.skip_pager = (m_state.skip_pager) ? true : false;
		}
		*/

		_client->updateWmStates();
	} else if (ev->message_type == ewmh->getAtom(NET_ACTIVE_WINDOW)) {
		if (client != _client) {
			activateClient(client);
		} else {
			if (!_mapped)
				mapWindow();
			giveInputFocus();
		}
	} else if (ev->message_type == ewmh->getAtom(NET_CLOSE_WINDOW)) {
		client->close();
	} else if (ev->message_type == ewmh->getAtom(NET_WM_DESKTOP)) {
		if (client == _client)
			setWorkspace(ev->data.l[0]);
	} else if (ev->message_type == _wm->getIcccmAtoms()->getAtom(WM_CHANGE_STATE) &&
						 (ev->format == 32) && (ev->data.l[0] == IconicState)) {
		if (client == _client)
			iconify();
	}
}

//! @fn    void handlePropertyChange(XPropertyEvent *ev, Client *client)
//! @brief
void
Frame::handlePropertyChange(XPropertyEvent *ev, Client *client)
{
	EwmhAtoms *ewmh = _wm->getEwmhAtoms(); // convenience

	if (ev->atom == ewmh->getAtom(NET_WM_DESKTOP)) {
		if (client == _client) {
			int workspace =
				_wm->getAtomLongValue(client->getWindow(),
															ewmh->getAtom(NET_WM_DESKTOP));

			if ((workspace != -1) && (workspace != signed(_workspace)))
				setWorkspace(workspace);
		}
	} else if (ev->atom == ewmh->getAtom(NET_WM_STRUT)) {
		client->getStrutHint();
	} else if (ev->atom == ewmh->getAtom(NET_WM_NAME)) {
		// TO-DO: UTF8 support so we can handle the hint
		client->getXClientName();
		updateTitles();
	} else if (ev->atom == XA_WM_NAME) {
//	if (!m_has_extended_net_name)
		client->getXClientName();
		updateTitles();
	} else if (ev->atom == XA_WM_NORMAL_HINTS) {
		client->getWMNormalHints();
	} else if (ev->atom == XA_WM_TRANSIENT_FOR) {
		client->getTransientForHint();
	}
}
