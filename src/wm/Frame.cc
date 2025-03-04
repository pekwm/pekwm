//
// Frame.cc for pekwm
// Copyright (C) 2002-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <algorithm>
#include <cstdio>

extern "C" {
#include <X11/Xatom.h>
}

#include "Debug.hh"
#include "PDecor.hh"
#include "Frame.hh"

#include "Compat.hh"
#include "X11.hh"
#include "Config.hh"
#include "ActionHandler.hh"
#include "AutoProperties.hh"
#include "Client.hh"
#include "ClientMgr.hh"
#include "ManagerWindows.hh"
#include "Workspaces.hh"

#include "tk/PWinObj.hh"
#include "tk/X11Util.hh"

std::vector<Frame*> Frame::_frames;
std::vector<uint> Frame::_frameid_list;

ActionEvent Frame::_ae_move = ActionEvent(Action(ACTION_MOVE));
ActionEvent Frame::_ae_resize = ActionEvent(Action(ACTION_RESIZE));
ActionEvent Frame::_ae_move_resize = ActionEvent(Action(ACTION_MOVE_RESIZE));

Frame* Frame::_tag_frame = nullptr;
bool Frame::_tag_behind = false;


Frame::Frame()
	: PDecor(None, true, false),
	  _id(0),
	  _client(nullptr),
	  _non_fullscreen_decor_state(0),
	  _non_fullscreen_layer(LAYER_NORMAL)
{
}

Frame::Frame(Client *client, AutoProperty *ap)
	: PDecor(client->getWindow(), false, true, client->getAPDecorName()),
	  _id(0),
	  _client(client),
	  _non_fullscreen_decor_state(0),
	  _non_fullscreen_layer(LAYER_NORMAL)
{
	// PWinObj attributes
	_type = WO_FRAME;

	// PDecor attributes
	_decor_cfg_child_move_overloaded = true;
	_decor_cfg_bpr_replay_pointer = true;
	_decor_cfg_bpr_al_title = MOUSE_ACTION_LIST_TITLE_FRAME;
	_decor_cfg_bpr_al_child = MOUSE_ACTION_LIST_CHILD_FRAME;

	grabButtons();

	// get unique id of the frame, if the client didn't have an id
	if (pekwm::isStarting()) {
		Cardinal id;
		if (X11::getCardinal(client->getWindow(), PEKWM_FRAME_ID, id)) {
			_id = id;
		}
	} else {
		_id = findFrameID();
	}

	// get the clients class_hint
	_class_hint = client->getClassHint();

	X11::grabServer();

	// We don't send any ConfigurRequests during setup, we send one when we
	// are finished to minimize traffic and confusion
	client->setConfigureRequestLock(true);

	// Before setting position and size up we make sure the decor state
	// of the client match the decore state of the framewidget
	if (! client->hasBorder()) {
		setBorder(STATE_UNSET);
	}
	if (! client->hasTitlebar()) {
		setTitlebar(STATE_UNSET);
	}

	// We first get the size of the window as it will be needed when placing
	// the window with the help of WinGravity.
	resizeChild(client->getWidth(), client->getHeight());

	// setup position
	bool place = false;
	if (client->isViewable() || client->isPlaced()
	    || (client->cameWithPosition()
		&& ! client->isCfgDeny(CFG_DENY_POSITION))) {
		moveChild(client->getX(), client->getY());
	} else {
		place = pekwm::config()->isPlaceNew();
	}

	// override both position and size with autoproperties
	bool ap_geometry = false;
	if (ap) {
		if (ap->isMask(AP_FRAME_GEOMETRY|AP_CLIENT_GEOMETRY)) {
			ap_geometry = true;

			setupAPGeometry(client, ap);

			if (ap->frame_gm_mask&(X_VALUE|Y_VALUE)
			    || ap->client_gm_mask&(X_VALUE|Y_VALUE)) {
				place = false;
			}
		}

		if (ap->isMask(AP_PLACE_NEW)) {
			place = ap->place_new;
		}
	}

	// still need a position?
	if (place) {
		Workspaces::layout(this, client->getTransientForClientWindow(),
				   ap ? ap->win_layouter_types : 0);
	}

	_old_gm = _gm;
	_non_fullscreen_decor_state = client->getDecorState();
	_non_fullscreen_layer = client->getLayer();

	X11::ungrabServer(true); // ungrab and sync

	// now insert the client in the frame we created.
	addChild(client);

	// needs to be done before the workspace insert and after the client
	// has been inserted, in order for layer settings to be propagated
	Frame::setLayer(client->getLayer());

	// I add these to the list before I insert the client into the frame to
	// be able to skip an extra updateClientList
	_frames.push_back(this);
	Workspaces::addToMRUBack(this);

	activateChild(client);

	// set the window states, shaded, maximized...
	getState(client);

	if (! _client->hasStrut() && ! ap_geometry) {
		if (fixGeometry()) {
			moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
		}
	}

	client->setConfigureRequestLock(false);
	client->configureRequestSend();

	// Figure out if we should be hidden or not, do not read autoprops
	PDecor::setWorkspace(_client->getWorkspace());

	woListAdd(this);
	_wo_map[_window] = this;
}

Frame::~Frame(void)
{
	// remove from lists
	_wo_map.erase(_window);
	woListRemove(this);
	_frames.erase(std::remove(_frames.begin(), _frames.end(), this),
		      _frames.end());
	Workspaces::removeFromMRU(this);
	if (_tag_frame == this) {
		_tag_frame = 0;
	}

	returnFrameID(_id);

	workspacesRemove();
}

void
Frame::grabButtons(void)
{
	// grab buttons so that we can reply them
	const std::vector<BoundButton> &cmab =
		pekwm::config()->getClientMouseActionButtons();
	std::vector<BoundButton>::const_iterator it = cmab.begin();
	for (; it != cmab.end(); ++it) {
		if (it->button == BUTTON_ANY) {
			continue;
		}

		std::vector<uint>::const_iterator mit = it->mods.begin();
		for (; mit != it->mods.end(); ++mit) {
			// only grabbing non-modifier buttons, the rest will
			// be grabbed by the client code.
			if (*mit) {
				continue;
			}
			X11Util::grabButton(it->button, *mit,
					    ButtonPressMask|ButtonReleaseMask,
					    _window, GrabModeSync);
		}
	}
}

// START - PWinObj interface.

/**
 * Return the active clients window type, defaults to WINDOW_TYPE_NORMAL.
 */
AtomName
Frame::getWinType() const
{
	Client *client = getActiveClient();
	return client ? client->getWinType() : WINDOW_TYPE_NORMAL;
}

//! @brief Iconifies the Frame.
void
Frame::iconify(void)
{
	if (_iconified) {
		return;
	}
	_iconified = true;

	unmapWindow();
}

/**
 * Toggles the Frame's sticky state
 */
void
Frame::toggleSticky()
{
	if (isSticky() == _client->isSticky()) {
		_client->toggleSticky();
	}
	_sticky = ! _sticky;

	// make sure it's visible/hidden
	PDecor::setWorkspace(Workspaces::getActive());
	updateDecor();
}

//! @brief Sets workspace on frame, wrapper to allow autoproperty loading
void
Frame::setWorkspace(unsigned int workspace)
{
	// Duplicate the behavior done in PDecor::setWorkspace to have a sane
	// value on _workspace and not NET_WM_STICKY_WINDOW.
	if (workspace != NET_WM_STICKY_WINDOW) {
		// Check for DisallowedActions="SetWorkspace".
		if (! _client->allowChangeWorkspace()) {
			return;
		}

		// First we set the workspace, then load autoproperties for
		// possible overrun of workspace and then set the workspace.
		_workspace = workspace;
		readAutoprops(APPLY_ON_WORKSPACE);
		workspace = _workspace;
	}

	PDecor::setWorkspace(workspace);
	updateDecor();
}

void
Frame::setLayer(Layer layer)
{
	PDecor::setLayer(layer);

	if (_client->getLayer() != layer) {
		_client->setLayer(layer);

		LayerObservation observation(layer);
		pekwm::observerMapping()->notifyObservers(_client,
							  &observation);
	}
}

// event handlers

ActionEvent*
Frame::handleMotionEvent(XMotionEvent *ev)
{
	// This is true when we have a title button pressed and then we don't
	// want to be able to drag windows around, therefore we ignore the
	// event
	if (_button) {
		return 0;
	}

	Config* cfg = pekwm::config();
	std::vector<ActionEvent>* al = nullptr;
	uint button = X11::getButtonFromState(ev->state);

	if (ev->window == getTitleWindow()) {
		al = cfg->getMouseActionList(MOUSE_ACTION_LIST_TITLE_FRAME);
	} else if (ev->window == _client->getWindow()) {
		al = cfg->getMouseActionList(MOUSE_ACTION_LIST_CHILD_FRAME);
	} else {
		uint pos = getBorderPosition(ev->subwindow);

		// If ev->subwindow wasn't one of the border windows, perhaps
		// ev->window is.
		if (pos == BORDER_NO_POS) {
			pos = getBorderPosition(ev->window);
		}
		if (pos != BORDER_NO_POS) {
			al = cfg->getBorderListFromPosition(pos);
		}
	}

	ActionEvent* ae = ActionHandler::findMouseAction(button, ev->state,
							 MOUSE_EVENT_MOTION,
							 al);

	// check motion threshold
	if (ae && (ae->threshold > 0)) {
		if (! ActionHandler::checkAEThreshold(ev->x_root, ev->y_root,
						      _pointer_x, _pointer_y,
						      ae->threshold)) {
			ae = nullptr;
		}
	}

	return ae;
}

ActionEvent*
Frame::handleEnterEvent(XCrossingEvent *ev)
{
	// Run event handler to get hoovering to work but ignore action
	// returned.
	PDecor::handleEnterEvent(ev);

	std::vector<ActionEvent> *al = 0;
	Config *cfg = pekwm::config();

	if (ev->window == getTitleWindow() || findButton(ev->window)) {
		al = cfg->getMouseActionList(MOUSE_ACTION_LIST_TITLE_FRAME);
	} else if (ev->subwindow == _client->getWindow()) {
		al = cfg->getMouseActionList(MOUSE_ACTION_LIST_CHILD_FRAME);
	} else {
		uint pos = getBorderPosition(ev->window);
		if (pos != BORDER_NO_POS) {
			al = cfg->getBorderListFromPosition(pos);
		}
	}

	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
					      MOUSE_EVENT_ENTER, al);
}

ActionEvent*
Frame::handleLeaveEvent(XCrossingEvent *ev)
{
	// Run event handler to get hoovering to work but ignore action
	// returned.
	PDecor::handleLeaveEvent(ev);

	MouseActionListName ln = MOUSE_ACTION_LIST_TITLE_FRAME;
	if (ev->window == _client->getWindow()) {
		ln = MOUSE_ACTION_LIST_CHILD_FRAME;
	}

	std::vector<ActionEvent>* al = pekwm::config()->getMouseActionList(ln);
	return ActionHandler::findMouseAction(BUTTON_ANY, ev->state,
					      MOUSE_EVENT_LEAVE, al);
}

ActionEvent*
Frame::handleMapRequest(XMapRequestEvent *ev)
{
	if (ev->window != _client->getWindow()) {
		return 0;
	}

	if (! _sticky && _workspace != Workspaces::getActive()) {
		P_LOG("Ignoring MapRequest, not on current workspace!");
		return 0;
	}

	mapWindow();

	return 0;
}

ActionEvent*
Frame::handleUnmapEvent(XUnmapEvent *ev)
{
	std::vector<PWinObj*>::iterator it = _children.begin();
	for (; it != _children.end(); ++it) {
		if (*(*it) == ev->window) {
			(*it)->handleUnmapEvent(ev);
			break;
		}
	}

	return nullptr;
}

// END - PWinObj interface.

#ifdef PEKWM_HAVE_SHAPE
void
Frame::handleShapeEvent(XShapeEvent *ev)
{
	if (ev->window != _client->getWindow()) {
		return;
	}
	applyBorderShape(ev->kind);
}
#endif // PEKWM_HAVE_SHAPE

// START - PDecor interface.

bool
Frame::allowMove(void) const
{
	return _client->allowMove();
}

/**
 * Return active client, or 0 if no clients or active child is not a Client.
 */
Client*
Frame::getActiveClient() const
{
	if (getActiveChild() && getActiveChild()->getType() == WO_CLIENT) {
		return static_cast<Client*>(getActiveChild());
	}
	return nullptr;
}

//! @brief Adds child to the frame.
void
Frame::addChild(PWinObj *child, std::vector<PWinObj*>::iterator *it)
{
	PDecor::addChild(child, it);
	X11::setCardinal(child->getWindow(), PEKWM_FRAME_ID, _id);
	child->lower();

	Client *client = dynamic_cast<Client*>(child);
	if (client && client->demandsAttention()) {
		incrAttention();
	}
}

/**
 * Add child preserving order from the previous pekwm run.
 */
void
Frame::addChildOrdered(Client *child)
{
	std::vector<PWinObj*>::iterator it(_children.begin());
	for (; it != _children.end(); ++it) {
		Client *client = static_cast<Client*>(*it);
		if (child->getInitialFrameOrder()
		    < client->getInitialFrameOrder()) {
			break;
		}
	}

	addChild(child, &it);
}

//! @brief Removes child from the frame.
void
Frame::removeChild(PWinObj *child, bool do_delete)
{
	if (static_cast<Client *>(child)->demandsAttention()) {
		decrAttention();
	}
	PDecor::removeChild(child, do_delete);
}

/**
 * Activates child in Frame, updating it's state and re-loading decor
 * rules to match updated title.
 */
void
Frame::activateChild(PWinObj *child)
{
	Client *new_client = static_cast<Client*>(child);
	bool client_changed = _client != new_client;
	if (client_changed) {
		applyState(new_client);
	}
	setFrameExtents(new_client);

	_client = new_client;
	if (_client->demandsAttention()) {
		_client->setDemandsAttention(false);
		decrAttention();
	}
	PDecor::activateChild(child);

	// applyBorderShape() uses current active child, so we need to activate
	// the child before setting shape
	if (X11::hasExtensionShape()) {
		applyBorderShape(ShapeBounding);
	}

	setOpacity(_client);

	if (_focused) {
		child->giveInputFocus();
	}

	// Reload the title and update the decoration if needed.
	handleTitleChange(_client, false);

	if (client_changed && ! pekwm::isStarting()) {
		Workspaces::updateClientList();
	}
}

/**
 * Called when child order is updated, re-sets the titles and updates
 * the _PEKWM_FRAME hints.
 */
void
Frame::updatedChildOrder(void)
{
	titleClear();

	std::vector<PWinObj*>::iterator it(_children.begin());
	for (long num = 0; it != _children.end(); ++num, ++it) {
		Client *client = static_cast<Client*>(*it);
		client->setPekwmFrameOrder(num);
		titleAdd(client->getTitle());
	}

	updatedActiveChild();
}

/**
 * Set the active title and update the _PEKWM_FRAME hints.
 */
void
Frame::updatedActiveChild(void)
{
	titleSetActive(0);

	uint size = _children.size();
	for (uint i = 0; i < size; ++i) {
		Client *client = static_cast<Client*>(_children[i]);
		client->setPekwmFrameActive(_child == client);
		if (_child == client) {
			titleSetActive(i);
		}
	}

	renderTitle();
}

void
Frame::getDecorInfo(char *buf, uint size, const Geometry& gm)
{
	uint width, height;
	calcSizeInCells(width, height, gm);
	snprintf(buf, size, "%u+%u+%d+%d", width, height, gm.x, gm.y);
}

void
Frame::giveInputFocus(void)
{
	if (_client->demandsAttention()) {
		_client->setDemandsAttention(false);
		decrAttention();
	}
	PDecor::giveInputFocus();
}

bool
Frame::setShaded(StateAction sa)
{
	// Check for DisallowedActions="Shade"
	if (! _client->allowShade()) {
		sa = STATE_UNSET;
	}

	bool shaded = isShaded();
	if (shaded != PDecor::setShaded(sa)) {
		_client->getState().shaded = isShaded();
		_client->updateEwmhStates();
	}
	return isShaded();
}

/**
 * Decor has been changed or titlebar/border state has been changed,
 * update the _NET_FRAME_EXTENTS.
 */
void
Frame::decorUpdated(void)
{
	if (_client) {
		setFrameExtents(_client);
	}
}

/**
 * Return decor name for the current client or attention decor if
 * Frame has client which demands attention.
 */
std::string
Frame::getDecorName()
{
	if (! demandAttention()) {
		std::string name = _client->getAPDecorName();
		if (! name.empty()) {
			return name;
		}
	}
	return PDecor::getDecorName();
}

// END - PDecor interface.

//! @brief Sets _PEKWM_FRAME_ID on all children in the frame
void
Frame::setId(uint id)
{
	_id = id;
	std::vector<PWinObj*>::iterator it = _children.begin();
	for (; it != _children.end(); ++it) {
		X11::setCardinal((*it)->getWindow(), PEKWM_FRAME_ID, id);
	}
}

/**
 * Sync Frame state with the state from the Client
 */
void
Frame::getState(Client *client)
{
	if (! client) {
		return;
	}

	bool b_client_iconified = client->isIconified ();
	if (_sticky != client->isSticky()) {
		_sticky = ! _sticky;
	}
	if (_maximized_horz != client->isMaximizedHorz()) {
		setStateMaximized(STATE_TOGGLE, true, false, false);
	}
	if (_maximized_vert != client->isMaximizedVert()) {
		setStateMaximized(STATE_TOGGLE, false, true, false);
	}
	if (isShaded() != client->isShaded()) {
		setShaded(STATE_TOGGLE);
	}
	if (getLayer() != client->getLayer()) {
		setLayer(client->getLayer());
	}
	if (_workspace != client->getWorkspace()) {
		PDecor::setWorkspace(client->getWorkspace());
	}

	// We need to set border and titlebar before setting fullscreen, as
	// fullscreen will unset border and titlebar if needed.
	if (hasBorder() != _client->hasBorder()) {
		setBorder(STATE_TOGGLE);
	}
	if (hasTitlebar() != _client->hasTitlebar()) {
		setTitlebar(STATE_TOGGLE);
	}
	if (_fullscreen != client->isFullscreen()) {
		setStateFullscreen(STATE_TOGGLE);
	}

	if (_iconified != b_client_iconified) {
		if (_iconified) {
			mapWindow();
		} else {
			iconify();
		}
	}

	if (_skip != client->getSkip()) {
		setSkip(client->getSkip());
	}
}

/**
 * Applies the frame's state on the Client
 */
void
Frame::applyState(Client *client)
{
	if (! client) {
		return;
	}

	client->setSticky(_sticky);
	client->getState().maximized_horz = _maximized_horz;
	client->getState().maximized_vert = _maximized_vert;
	client->getState().shaded = isShaded();
	client->setWorkspace(_workspace);
	client->setLayer(getLayer());

	// fix border / titlebar state
	client->setBorder(hasBorder());
	client->setTitlebar(hasTitlebar());
	// make sure the window has the correct mapped state
	if (_mapped != client->isMapped()) {
		if (! _mapped) {
			client->unmapWindow();
		} else {
			client->mapWindow();
		}
	}

	client->updateEwmhStates();
}

void
Frame::setFrameExtents(Client *cl)
{
	Cardinal extents[4];
	extents[0] = bdLeft(this);
	extents[1] = bdRight(this);
	extents[2] = bdTop(this) + titleHeight(this);
	extents[3] = bdBottom(this);
	X11::setCardinals(cl->getWindow(), NET_FRAME_EXTENTS, extents, 4);
}

//! @brief Sets skip state.
void
Frame::setSkip(uint skip)
{
	PDecor::setSkip(skip);
	_client->setSkip(skip);
}

//! @brief Find Frame with id.
//! @param id ID to search for.
//! @return Frame if found, else 0.
Frame*
Frame::findFrameFromID(uint id)
{
	frame_cit it = _frames.begin();
	for (; it != _frames.end(); ++it) {
		if ((*it)->getId() == id) {
			return *it;
		}
	}
	return nullptr;
}

void
Frame::setupAPGeometry(Client *client, AutoProperty *ap)
{
	// frame geomtry overides client geometry

	// get client geometry
	if (ap->isMask(AP_CLIENT_GEOMETRY)) {
		Geometry gm(client->getGeometry());
		applyGeometry(gm, ap->client_gm, ap->client_gm_mask);

		if (ap->client_gm_mask&(X_VALUE|Y_VALUE)) {
			moveChild(gm.x, gm.y);
		}
		if(ap->client_gm_mask&(WIDTH_VALUE|HEIGHT_VALUE)) {
			resizeChild(gm.width, gm.height);
		}
		P_TRACE("applied ClientGeometry property "
			<< ap->client_gm << " -> " << gm);
	}

	// get frame geometry
	if (ap->isMask(AP_FRAME_GEOMETRY)) {
		Geometry screen_gm = X11::getScreenGeometry();
		Geometry gm(_gm);
		applyGeometry(gm, ap->frame_gm, ap->frame_gm_mask, screen_gm);
		moveResize(gm, ap->frame_gm_mask);
		P_TRACE("applied FrameGeometry property "
			<< ap->frame_gm << " -> " << gm);
	}
}

void
Frame::applyGeometry(Geometry &gm, const Geometry &ap_gm, int mask)
{
	applyGeometry(gm, ap_gm, mask, X11::getScreenGeometry());
}

/**
 * Apply ap_gm on gm using screen_gm for percent/negative.
 *
 * @param gm Geometry to modify.
 * @param ap_gm Geometry to get values from.
 * @param mask Geometry mask.
 * @param screen_gm Geometry of the screen/head for position.
 */
void
Frame::applyGeometry(Geometry &gm, const Geometry &ap_gm, int mask,
		     const Geometry &screen_gm)
{
	// Read size before position so negative position works, if size is
	// < 1 consider it to be full screen size.
	if (mask & WIDTH_VALUE) {
		if (mask & WIDTH_PERCENT) {
			gm.width = int(screen_gm.width
					* (float(ap_gm.width) / 100));
		} else if (ap_gm.width < 1) {
			gm.width = screen_gm.width;
		} else {
			gm.width = ap_gm.width;
		}
	}

	if (mask & HEIGHT_VALUE) {
		if (mask & HEIGHT_PERCENT) {
			gm.height = int(screen_gm.height
					* (float(ap_gm.height) / 100));
		} else if (ap_gm.height < 1) {
			gm.height = screen_gm.height;
		} else {
			gm.height = ap_gm.height;
		}
	}

	// Read position
	if (mask & X_VALUE) {
		if (mask & X_PERCENT) {
			gm.x = int(screen_gm.width * (float(ap_gm.x) / 100));
		} else if (mask & X_NEGATIVE) {
			gm.x = screen_gm.width - gm.width - ap_gm.x;
		} else {
			gm.x = screen_gm.x + ap_gm.x;
		}
	}
	if (mask & Y_VALUE) {
		if (mask & Y_PERCENT) {
			gm.y = int(screen_gm.height * (float(ap_gm.y) / 100));
		} else if (mask & Y_NEGATIVE) {
			gm.y = screen_gm.height - gm.height - ap_gm.y;
		} else {
			gm.y = screen_gm.y + ap_gm.y;
		}
	}
}

//! @brief Finds free Frame ID.
//! @return First free Frame ID.
uint
Frame::findFrameID(void)
{
	uint id = 0;

	if (_frameid_list.size()) {
		// Check for used Frame IDs
		id = _frameid_list.back();
		_frameid_list.pop_back();
	} else {
		// No free, get next number (Frame is not in list when this is
		// called.)
		id = _frames.size() + 1;
	}

	return id;
}

//! @brief Returns Frame ID to used frame id list.
//! @param id ID to return.
void
Frame::returnFrameID(uint id)
{
	std::vector<uint>::iterator it(_frameid_list.begin());
	for (; it != _frameid_list.end() && id < *it; ++it)
		;
	_frameid_list.insert(it, id);
}

//! @brief Resets Frame IDs.
void
Frame::resetFrameIDs(void)
{
	frame_it it(_frames.begin());
	for (uint id = 1; it != _frames.end(); ++id, ++it) {
		(*it)->setId(id);
	}
}

/**
 * Remove given client from frame moving it to a new frame at the
 * specified coordinates.
 *
 * @return new Frame on success, nullptr if client is not part of frame or is
 *         the only client in the frame.
 */
Frame*
Frame::detachClient(Client *client, int x, int y)
{
	if (client->getParent() != this) {
		return nullptr;
	} else if (_children.size() < 2) {
		move(x, y);
		return nullptr;
	}

	removeChild(client);

	client->move(x, y + bdTop(this));

	Frame *frame = new Frame(client, nullptr);
	frame->workspacesInsert();
	client->setParent(frame);
	client->setWorkspace(Workspaces::getActive());

	setFocused(false);

	return frame;
}

//! @brief Makes sure the frame doesn't cover any struts / the harbour.
bool
Frame::fixGeometry(void)
{
	uint head_nr = X11Util::getNearestHead(*this);
	Geometry head, before;
	if (_fullscreen) {
		X11::getHeadInfo(head_nr, head);
	} else {
		pekwm::rootWo()->getHeadInfoWithEdge(head_nr, head);
	}

	before = _gm;

	// fix size
	if (_gm.width > head.width) {
		_gm.width = head.width;
	}
	if (_gm.height > head.height) {
		_gm.height = head.height;
	}

	// fix position
	if (_gm.x < head.x) {
		_gm.x = head.x;
	} else if ((_gm.x + _gm.width) > (head.x + head.width)) {
		_gm.x = head.x + head.width - _gm.width;
	}
	if (_gm.y < head.y) {
		_gm.y = head.y;
	} else if ((_gm.y + _gm.height) > (head.y + head.height)) {
		_gm.y = head.y + head.height - _gm.height;
	}

	return (_gm != before);
}

void
Frame::clearMaximizedStatesAfterResize(void)
{
	if (_maximized_horz || _maximized_vert) {
		_maximized_horz = false;
		_maximized_vert = false;
		_client->getState().maximized_horz = false;
		_client->getState().maximized_vert = false;
		_client->updateEwmhStates();
	}
}

void
Frame::moveToHead(const std::string& arg)
{
	int head_nr;
	try {
		head_nr = std::stoi(arg);
	} catch (std::invalid_argument&) {
		head_nr = -1;
	}

	if (head_nr != -1) {
		// valid value from stoi above
	} else if (pekwm::ascii_ncase_equal(arg, "LEFT")) {
		head_nr = X11Util::getNearestHead(*this,
						  DIRECTION_LEFT,
						  DIRECTION_NO);
	} else if (pekwm::ascii_ncase_equal(arg, "RIGHT")) {
		head_nr = X11Util::getNearestHead(*this,
						  DIRECTION_RIGHT,
						  DIRECTION_NO);
	} else if (pekwm::ascii_ncase_equal(arg, "UP")) {
		head_nr = X11Util::getNearestHead(*this,
						  DIRECTION_NO,
						  DIRECTION_UP);
	} else if (pekwm::ascii_ncase_equal(arg, "DOWN")) {
		head_nr = X11Util::getNearestHead(*this,
						  DIRECTION_NO,
						  DIRECTION_DOWN);
	} else {
		head_nr = X11::findHeadByName(arg);
		if (head_nr == -1) {
			P_ERR("unrecognized MoveToHead argument: " << arg);
		}
	}
	if (head_nr != -1) {
		moveToHead(head_nr);
	}
}

void
Frame::moveToHead(int head_nr)
{
	int curr_head_nr = X11Util::getNearestHead(*this);
	if (curr_head_nr == head_nr || head_nr >= X11::getNumHeads()) {
		return;
	}

	Geometry old_gm, new_gm;
	if (isFullscreen()) {
		X11::getHeadInfo(curr_head_nr, old_gm);
		X11::getHeadInfo(head_nr, new_gm);
	} else {
		pekwm::rootWo()->getHeadInfoWithEdge(curr_head_nr, old_gm);
		pekwm::rootWo()->getHeadInfoWithEdge(head_nr, new_gm);
	}

	_gm.x = new_gm.x + (_gm.x - old_gm.x);
	_gm.y = new_gm.y + (_gm.y - old_gm.y);

	// Resize if the window is fullscreen or maximized
	Client *client = getActiveClient();
	if (isFullscreen() || (client && client->isMaximizedHorz())) {
		_gm.width = new_gm.width;
	}
	if (isFullscreen() || (client && client->isMaximizedVert())) {
		_gm.height = new_gm.height;
	}

	// Ensure the window fits in the new head.
	_gm.width = std::min(_gm.width, new_gm.width);
	_gm.height = std::min(_gm.height, new_gm.height);

	if ((_gm.x + _gm.width) > (new_gm.x + new_gm.width)) {
		_gm.x = new_gm.x + new_gm.width - _gm.width;
	}
	if ((_gm.y + _gm.height) > (new_gm.y + new_gm.height)) {
		_gm.y = new_gm.y + new_gm.height - _gm.height;
	}

	moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
}

//! @brief Moves the Frame to the screen edge ori ( considering struts )
void
Frame::moveToEdge(OrientationType ori)
{
	uint head_nr;
	Geometry head, real_head;

	head_nr = X11Util::getNearestHead(*this);
	X11::getHeadInfo(head_nr, real_head);
	pekwm::rootWo()->getHeadInfoWithEdge(head_nr, head);

	switch (ori) {
	case TOP_LEFT:
		_gm.x = head.x;
		_gm.y = head.y;
		break;
	case TOP_EDGE:
		_gm.y = head.y;
		break;
	case TOP_CENTER_EDGE:
		_gm.x = real_head.x + ((real_head.width - _gm.width) / 2);
		_gm.y = head.y;
		break;
	case TOP_RIGHT:
		_gm.x = head.x + head.width - _gm.width;
		_gm.y = head.y;
		break;
	case BOTTOM_RIGHT:
		_gm.x = head.x + head.width - _gm.width;
		_gm.y = head.y + head.height - _gm.height;
		break;
	case BOTTOM_EDGE:
		_gm.y = head.y + head.height - _gm.height;
		break;
	case BOTTOM_CENTER_EDGE:
		_gm.x = real_head.x + ((real_head.width - _gm.width) / 2);
		_gm.y = head.y + head.height - _gm.height;
		break;
	case BOTTOM_LEFT:
		_gm.x = head.x;
		_gm.y = head.y + head.height - _gm.height;
		break;
	case LEFT_EDGE:
		_gm.x = head.x;
		break;
	case LEFT_CENTER_EDGE:
		_gm.x = head.x;
		_gm.y = real_head.y + ((real_head.height - _gm.height) / 2);
		break;
	case RIGHT_EDGE:
		_gm.x = head.x + head.width - _gm.width;
		break;
	case RIGHT_CENTER_EDGE:
		_gm.x = head.x + head.width - _gm.width;
		_gm.y = real_head.y + ((real_head.height - _gm.height) / 2);
		break;
	case CENTER:
		_gm.x = real_head.x + ((real_head.width - _gm.width) / 2);
		_gm.y = real_head.y + ((real_head.height - _gm.height) / 2);
	default:
		// DO NOTHING
		break;
	}

	move(_gm.x, _gm.y);
}

//! @brief Updates all inactive childrens geometry and state
void
Frame::updateInactiveChildInfo(void)
{
	std::vector<PWinObj*>::iterator it = _children.begin();
	for (; it != _children.end(); ++it) {
		if ((*it) != _client) {
			applyState(static_cast<Client*>(*it));
			(*it)->resize(getChildWidth(), getChildHeight());
		}
	}
}

// STATE actions begin

//! @brief Toggles current clients max size
//! @param sa State to set
//! @param horz Include horizontal in (de)maximize
//! @param vert Include vertcical in (de)maximize
//! @param fill Limit size by other frame boundaries ( defaults to false )
void
Frame::setStateMaximized(StateAction sa, bool horz, bool vert, bool fill)
{
	if (sa != STATE_TOGGLE
	    && ! fill
	    && (! horz || _maximized_horz == sa)
	    && (! vert || _maximized_vert == sa)) {
		// already set as requested, do nothing
		return;
	}

	setShaded(STATE_UNSET);
	setStateFullscreen(STATE_UNSET);

	// make sure the two states are in sync if toggling
	if ((horz == vert) && (sa == STATE_TOGGLE)) {
		if (_maximized_horz != _maximized_vert) {
			horz = ! _maximized_horz;
			vert = ! _maximized_vert;
		}
	}

	XSizeHints hints = _client->getActiveSizeHints();

	Geometry head;
	pekwm::rootWo()->getHeadInfoWithEdge(X11Util::getNearestHead(*this),
					     head);

	int max_x, max_r, max_y, max_b;
	max_x = head.x;
	max_r = head.width + head.x;
	max_y = head.y;
	max_b = head.height + head.y;

	if (fill) {
		getMaxBounds(max_x, max_r, max_y, max_b);

		// make sure vert and horz gets set if fill is on
		sa = STATE_SET;
	}

	if (horz && (fill || _client->allowMaximizeHorz())) {
		if ((sa == STATE_SET) ||
		    ((sa == STATE_TOGGLE) && ! _maximized_horz)) {
			// maximize
			uint h_decor = _gm.width - getChildWidth();

			if (! fill) {
				_old_gm.x = _gm.x;
				_old_gm.width = _gm.width;
			}

			_gm.x = max_x;
			_gm.width = max_r - max_x;

			if ((hints.flags&PMaxSize)
			    && (_gm.width > (hints.max_width + h_decor))) {
				_gm.width = hints.max_width + h_decor;
			}
			_maximized_horz = ! fill;
		} else if ((sa == STATE_UNSET) ||
			   ((sa == STATE_TOGGLE) && _maximized_horz)) {
			// demaximize
			_gm.x = _old_gm.x;
			_gm.width = _old_gm.width;
			_maximized_horz = false;
		}
		_client->getState().maximized_horz = _maximized_horz;
	}

	if (vert && (fill || _client->allowMaximizeVert())) {
		if ((sa == STATE_SET) ||
		    ((sa == STATE_TOGGLE) && ! _maximized_vert)) {
			// maximize
			uint v_decor = _gm.height - getChildHeight();

			if (! fill) {
				_old_gm.y = _gm.y;
				_old_gm.height = _gm.height;
			}

			_gm.y = max_y;
			_gm.height = max_b - max_y;

			if ((hints.flags&PMaxSize) &&
			    (_gm.height > (hints.max_height + v_decor))) {
				_gm.height = hints.max_height + v_decor;
			}
			_maximized_vert = ! fill;
		} else if ((sa == STATE_UNSET) ||
			   ((sa == STATE_TOGGLE) && _maximized_vert)) {
			// demaximize
			_gm.y = _old_gm.y;
			_gm.height = _old_gm.height;
			_maximized_vert = false;
		}
		_client->getState().maximized_vert = _maximized_vert;
	}

	// harbour already considered
	fixGeometry();
	// keep x and keep y ( make conform to inc size )
	downSize(_gm, true, true);

	moveResize(_gm.x, _gm.y, _gm.width, _gm.height);

	_client->updateEwmhStates();
}

//! @brief Set fullscreen state
void
Frame::setStateFullscreen(StateAction sa)
{
	// Check for DisallowedActions="Fullscreen".
	if (! _client->allowFullscreen()) {
		sa = STATE_UNSET;
	}

	if (! ActionUtil::needToggle(sa, _fullscreen)) {
		return;
	}

	bool lock = _client->setConfigureRequestLock(true);

	if (_fullscreen) {
		bool state_has_border =
			(_non_fullscreen_decor_state&DECOR_BORDER)
			== DECOR_BORDER;
		if (state_has_border != hasBorder()) {
			setBorder(STATE_TOGGLE);
		}
		bool state_has_titlebar =
			(_non_fullscreen_decor_state&DECOR_TITLEBAR)
			== DECOR_TITLEBAR;
		if (state_has_titlebar != hasTitlebar()) {
			setTitlebar(STATE_TOGGLE);
		}
		_gm = _old_gm;

	} else {
		_old_gm = _gm;
		_non_fullscreen_decor_state = _client->getDecorState();
		_non_fullscreen_layer = _client->getLayer();

		setBorder(STATE_UNSET);
		setTitlebar(STATE_UNSET);

		Geometry head;
		uint nr = X11Util::getNearestHead(*this);
		X11::getHeadInfo(nr, head);

		_gm = head;
	}

	_fullscreen = !_fullscreen;
	_client->getState().fullscreen = _fullscreen;

	moveResize(_gm.x, _gm.y, _gm.width, _gm.height);

	// Re-stack window if fullscreen is above other windows.
	if (pekwm::config()->isFullscreenAbove()
	    && _client->getLayer() != LAYER_DESKTOP) {
		setLayer(_fullscreen
				? LAYER_ABOVE_DOCK
				: _non_fullscreen_layer);
		raise();
	}

	_client->setConfigureRequestLock(lock);
	_client->configureRequestSend();

	_client->updateEwmhStates();
}

void
Frame::clearMaximizedStates(void)
{
	clearMaximizedStatesAfterResize();
}

void
Frame::setStateSticky(StateAction sa)
{
	// Check for DisallowedActions="Stick".
	if (! _client->allowStick()) {
		sa = STATE_UNSET;
	}

	if (ActionUtil::needToggle(sa, _sticky)) {
		toggleSticky();
	}
}

void
Frame::setStateAlwaysOnTop(StateAction sa)
{
	if (! ActionUtil::needToggle(sa, getLayer() >= LAYER_ONTOP)) {
		return;
	}

	_client->alwaysOnTop(getLayer() < LAYER_ONTOP);
	setLayer(_client->getLayer());

	raise();
}

void
Frame::setStateAlwaysBelow(StateAction sa)
{
	if (! ActionUtil::needToggle(sa, getLayer() == LAYER_BELOW)) {
		return;
	}

	_client->alwaysBelow(getLayer() > LAYER_BELOW);
	setLayer(_client->getLayer());

	lower();
}

//! @brief Hides/Shows the border depending on _client
//! @param sa State to set
void
Frame::setStateDecorBorder(StateAction sa)
{
	bool border = hasBorder();

	// state changed, update client and atom state
	if (border != setBorder(sa)) {
		_client->setBorder(hasBorder());

		// update the _PEKWM_FRAME_DECOR hint
		X11::setCardinal(_client->getWindow(), PEKWM_FRAME_DECOR,
				 _client->getDecorState());
	}
}

//! @brief Hides/Shows the titlebar depending on _client
//! @param sa State to set
void
Frame::setStateDecorTitlebar(StateAction sa)
{
	bool titlebar = hasTitlebar();

	// state changed, update client and atom state
	if (titlebar != setTitlebar(sa)) {
		_client->setTitlebar(hasTitlebar());

		X11::setCardinal(_client->getWindow(), PEKWM_FRAME_DECOR,
				 _client->getDecorState());
	}
}

void
Frame::setStateIconified(StateAction sa)
{
	// Check for DisallowedActions="Iconify".
	if (! _client->allowIconify()) {
		sa = STATE_UNSET;
	}

	if (! ActionUtil::needToggle(sa, _iconified)) {
		return;
	}

	if (_iconified) {
		mapWindow();
	} else {
		iconify();
	}
}

//! (Un)Sets the tagged Frame.
//!
//! @param sa Set/Unset or Toggle the state.
//! @param behind Should tagged actions be behind (non-focused).
void
Frame::setStateTagged(StateAction sa, bool behind)
{
	if (ActionUtil::needToggle(sa, (_tag_frame != 0))) {
		_tag_frame = (this == _tag_frame) ? 0 : this;
		_tag_behind = behind;
	}
}

void
Frame::setStateSkip(StateAction sa, uint skip)
{
	if (! ActionUtil::needToggle(sa, _skip&skip)) {
		return;
	}

	if (_skip&skip) {
		_skip &= ~skip;
	} else {
		_skip |= skip;
	}

	setSkip(_skip);
}

//! @brief Sets client title
void
Frame::setStateTitle(StateAction sa, Client *client, const std::string &title)
{
	if (sa == STATE_SET) {
		client->getTitle()->setUser(title);

	} else if (sa == STATE_UNSET) {
		client->getTitle()->setUser("");
		client->readName();
	} else {
		if (client->getTitle()->isUserSet()) {
			client->getTitle()->setUser("");
		} else {
			client->getTitle()->setUser(title);
		}
	}

	// Set PEKWM_TITLE atom to preserve title on client between sessions.
	X11::setUtf8String(client->getWindow(), PEKWM_TITLE,
			   client->getTitle()->getUser());

	renderTitle();
}

//! @brief Sets clients marked state.
//! @param sa
//! @param client
void
Frame::setStateMarked(StateAction sa, Client *client)
{
	if (! client || ! ActionUtil::needToggle(sa, client->isMarked())) {
		return;
	}

	// Set marked state and re-render title to update visual marker.
	client->setStateMarked(sa);
	renderTitle();
}

void
Frame::setStateOpaque(StateAction sa)
{
	if (! ActionUtil::needToggle(sa, _opaque)) {
		return;
	}
	_client->setOpaque(!_opaque);
	setOpaque(!_opaque);
}
// STATE actions end

void
Frame::getMaxBounds(int &max_x,int &max_r, int &max_y, int &max_b)
{
	int f_r, f_b;
	int x, y, r, b;

	f_r = getRX();
	f_b = getBY();

	frame_it it = _frames.begin();
	for (; it != _frames.end(); ++it) {
		if (! (*it)->isMapped()) {
			continue;
		}

		x = (*it)->getX();
		y = (*it)->getY();
		r = (*it)->getRX();
		b = (*it)->getBY();

		// update max borders when other frame border lies between this
		// border and prior max border (originally screen/head edge)
		if ((r >= max_x)
		    && (r <= _gm.x)
		    && ! ((y >= f_b) || (b <= _gm.y))) {
			max_x = r;
		}
		if ((x <= max_r)
		    && (x >= f_r)
		    && ! ((y >= f_b) || (b <= _gm.y))) {
			max_r = x;
		}
		if ((b >= max_y)
		    && (b <= _gm.y)
		    && ! ((x >= f_r) || (r <= _gm.x))) {
			max_y = b;
		}
		if ((y <= max_b)
		    && (y >= f_b)
		    && ! ((x >= f_r) || (r <= _gm.x))) {
			max_b = y;
		}
	}
}

void
Frame::setGeometry(const std::string& geometry, int head, bool honour_strut)
{
	Geometry gm;
	int mask = X11::parseGeometry(geometry, gm);
	if (! mask) {
		return;
	}

	Geometry screen_gm = X11::getScreenGeometry();
	if (head != -1) {
		if (head == -2) {
			head = X11Util::getNearestHead(*this);
		}

		if (honour_strut) {
			pekwm::rootWo()->getHeadInfoWithEdge(head, screen_gm);
		} else {
			screen_gm = X11::getHeadGeometry(head);
		}
	}

	Geometry applied_gm;
	applyGeometry(applied_gm, gm, mask, screen_gm);
	moveResize(applied_gm, mask);
}

void
Frame::growDirection(uint direction)
{
	Geometry head;
	pekwm::rootWo()->getHeadInfoWithEdge(X11Util::getNearestHead(*this),
					     head);

	switch (direction) {
	case DIRECTION_UP:
		_gm.height = getBY() - head.y;
		_gm.y = head.y;
		break;
	case DIRECTION_DOWN:
		_gm.height = head.y + head.height - _gm.y;
		break;
	case DIRECTION_LEFT:
		_gm.width = getRX() - head.x;
		_gm.x = head.x;
		break;
	case DIRECTION_RIGHT:
		_gm.width = head.x + head.width - _gm.x;
		break;
	default:
		break;
	}

	downSize(_gm,
		 (direction != DIRECTION_LEFT), (direction != DIRECTION_UP));

	moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
}

//! @brief Closes the frame and all clients in it
void
Frame::close(void)
{
	std::vector<PWinObj*>::iterator it = _children.begin();
	for (; it != _children.end(); ++it) {
		static_cast<Client*>(*it)->close();
	}
}

/**
 * Reads autoprops for the active client.
 *
 * @param type Defaults to APPLY_ON_RELOAD
 */
void
Frame::readAutoprops(ApplyOn type)
{
	if ((type != APPLY_ON_RELOAD) && (type != APPLY_ON_WORKSPACE)) {
		return;
	}

	_class_hint.title = _client->getTitle()->getReal();
	AutoProperty *data =
		pekwm::autoProperties()->findAutoProperty(_class_hint,
							  _workspace, type);
	_class_hint.title = "";

	if (! data) {
		return;
	}

	// Set the correct group of the window
	_class_hint.group = data->group_name;

	if (_class_hint == _client->getClassHint()
	    && (_client->isTransient()
		&& ! data->isApplyOn(APPLY_ON_TRANSIENT))) {
		return;
	}

	if (data->isMask(AP_STICKY) && _sticky != data->sticky) {
		toggleSticky();
	}
	if (data->isMask(AP_SHADED) && (isShaded() != data->shaded)) {
		setShaded(STATE_UNSET);
	}
	if (data->isMask(AP_MAXIMIZED_HORIZONTAL)
	    && (_maximized_horz != data->maximized_horizontal)) {
		setStateMaximized(STATE_TOGGLE, true, false, false);
	}
	if (data->isMask(AP_MAXIMIZED_VERTICAL)
	    && (_maximized_vert != data->maximized_vertical)) {
		setStateMaximized(STATE_TOGGLE, false, true, false);
	}
	if (data->isMask(AP_FULLSCREEN) && (_fullscreen != data->fullscreen)) {
		setStateFullscreen(STATE_TOGGLE);
	}

	if (data->isMask(AP_ICONIFIED) && (_iconified != data->iconified)) {
		if (_iconified) {
			mapWindow();
		} else {
			iconify();
		}
	}
	if (data->isMask(AP_WORKSPACE)) {
		// In order to avoid eternal recursion, the workspace is only
		// set here and then actually called PDecor::setWorkspace from
		// Frame::setWorkspace
		if (type == APPLY_ON_WORKSPACE) {
			_workspace = data->workspace;
		}	else if (_workspace != data->workspace) {
			// Call PDecor directly to avoid recursion.
			PDecor::setWorkspace(data->workspace);
		}
	}

	if (data->isMask(AP_SHADED) && (isShaded() != data->shaded)) {
		setShaded(STATE_TOGGLE);
	}
	if (data->isMask(AP_LAYER) && (data->layer <= LAYER_MENU)) {
		_client->setLayer(data->layer);
		raise(); // restack the frame
	}

	if (data->isMask(AP_FRAME_GEOMETRY|AP_CLIENT_GEOMETRY)) {
		setupAPGeometry(_client, data);

		// Apply changes
		moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
	}

	if (data->isMask(AP_BORDER) && (hasBorder() != data->border)) {
		setStateDecorBorder(STATE_TOGGLE);
	}
	if (data->isMask(AP_TITLEBAR) && (hasTitlebar() != data->titlebar)) {
		setStateDecorTitlebar(STATE_TOGGLE);
	}

	if (data->isMask(AP_SKIP)) {
		_client->setSkip(data->skip);
		setSkip(_client->getSkip());
	}

	if (data->isMask(AP_FOCUSABLE)) {
		_client->setFocusable(data->focusable);
	}
}

/**
 * Return how large the frame is in cells.
 */
void
Frame::calcSizeInCells(uint &width, uint &height, const Geometry& gm)
{
	XSizeHints hints = _client->getActiveSizeHints();

	if (hints.flags&PResizeInc) {
		int c_width = gm.width - bdLeft(this) - bdRight(this);
		if (c_width < 1) {
			c_width = 1;
		}
		int c_height = gm.height - decorHeight(this);
		if (c_height < 1) {
			c_height = 1;
		}
		width = (c_width - hints.base_width) / hints.width_inc;
		height = (c_height - hints.base_height) / hints.height_inc;
	} else {
		width = gm.width;
		height = gm.height;
	}
}

void
Frame::setGravityPosition(int gravity, int &x, int &y, int diff_w, int diff_h)
{
	switch (gravity) {
	case NorthWestGravity:
		break;
	case NorthGravity:
		x = x - diff_w / 2;
		break;
	case NorthEastGravity:
		x = x - diff_w;
		break;
	case WestGravity:
		y = y - diff_h /2;
		break;
	case CenterGravity:
		x = x - diff_w / 2;
		y = y - diff_h /2;
		break;
	case EastGravity:
		x = x - diff_w;
		y = y - diff_h /2;
		break;
	case SouthWestGravity:
		y = y - diff_h;
		break;
	case SouthGravity:
		x = x - diff_w / 2;
		y = y - diff_h;
		break;
	case SouthEastGravity:
		x = x - diff_w;
		y = y - diff_h;
		break;
	case StaticGravity:
	case ForgetGravity:
	default:
		// keep position
		break;
	}
}
/**
 * Makes gm conform to _clients width and height inc.
 */
void
Frame::downSize(Geometry &gm, bool keep_x, bool keep_y)
{
	XSizeHints hints = _client->getActiveSizeHints();

	// conform to width_inc
	if (hints.flags&PResizeInc) {
		int o_r = getRX();
		int b_x = (hints.flags&PBaseSize)
			? hints.base_width
			: (hints.flags&PMinSize) ? hints.min_width : 0;

		gm.width -= (getChildWidth() - b_x) % hints.width_inc;
		if (! keep_x) {
			gm.x = o_r - gm.width;
		}
	}

	// conform to height_inc
	if (hints.flags&PResizeInc) {
		int o_b = getBY();
		int b_y = (hints.flags&PBaseSize)
			? hints.base_height
			: (hints.flags&PMinSize) ? hints.min_height : 0;

		gm.height -= (getChildHeight() - b_y) % hints.height_inc;
		if (! keep_y) {
			gm.y = o_b - gm.height;
		}
	}
}

// Below this Client message handling is done

//! @brief Handle XConfgiureRequestEvents
void
Frame::handleConfigureRequest(XConfigureRequestEvent *ev, Client *client)
{
	if (client != _client) {
		_client->configureRequestSend();
		return; // only handle the active client's events
	}

	// Update the stacking (ev->value_mask&CWSibling should not happen)
	if (! client->isCfgDeny(CFG_DENY_STACKING)
	    && ev->value_mask&CWStackMode) {
		if (ev->detail == Above) {
			raise();
		} else if (ev->detail == Below) {
			lower();
		} // ignore TopIf, BottomIf, Opposite - PekWM is a reparenting
		  // WM
	}

	// Update the geometry if requested
	bool chg_size = ! _client->isCfgDeny(CFG_DENY_SIZE)
		&& (ev->value_mask&(CWWidth|CWHeight));
	bool chg_pos  = ! _client->isCfgDeny(CFG_DENY_POSITION)
		&& (ev->value_mask&(CWX|CWY));
	if (! (chg_size || chg_pos)) {
		_client->configureRequestSend();
		return;
	}

	if (pekwm::config()->isFullscreenDetect()
	    && (ev->value_mask&(CWX|CWY|CWWidth|CWHeight))
		== (CWX|CWY|CWWidth|CWHeight)
	    && isRequestGeometryFullscreen(ev)) {
		if (isFullscreen()) {
			_client->configureRequestSend();
		} else {
			setStateFullscreen(STATE_SET);
		}
		return;
	}

	Geometry gm = _gm;

	if (chg_size) {
		int diff_w = ev->width - gm.width + decorWidth(this);
		gm.width += diff_w;
		int diff_h = ev->height - gm.height + decorHeight(this);
		gm.height += diff_h;

		if (!chg_pos) {
			int grav = _client->getActiveSizeHints().win_gravity;
			setGravityPosition(grav, gm.x, gm.y, diff_w, diff_h);
		}
	}

	if (chg_pos) {
		gm.x = ev->x;
		gm.y = ev->y;
	}

	X11::keepVisible(gm);

	if (pekwm::config()->isFullscreenDetect() && isFullscreen()) {
		_old_gm = gm;
		setStateFullscreen(STATE_UNSET);
		return;
	}

	moveResize(gm.x, gm.y, gm.width, gm.height);
}

/**
 * Check if requested size if "fullscreen"
 */
bool
Frame::isRequestGeometryFullscreen(XConfigureRequestEvent *ev)
{
	if (_client->isCfgDeny(CFG_DENY_SIZE)
	    || _client->isCfgDeny(CFG_DENY_POSITION)
	    || ! _client->allowFullscreen()) {
		return false;
	}

	int nearest_head = X11::getNearestHead(ev->x, ev->y);
	Geometry gm_request(ev->x, ev->y, ev->width, ev->height);
	Geometry gm_screen(X11::getScreenGeometry());
	Geometry gm_head(X11::getHeadGeometry(nearest_head));

	bool is_fullscreen = false;
	if (gm_request == gm_screen || gm_request == gm_head) {
		is_fullscreen = true;
	} else {
		downSize(gm_screen, true, true);
		downSize(gm_head, true, true);
		if (gm_request == gm_screen || gm_request == gm_head) {
			is_fullscreen = true;
		}
	}
	return is_fullscreen;
}

/**
 * Handle client message.
 */
ActionEvent*
Frame::handleClientMessage(XClientMessageEvent *ev, Client *client)
{
	ActionEvent *ae = nullptr;

	if (ev->message_type == X11::getAtom(STATE)) {
		handleClientStateMessage(ev, client);
	} else if (ev->message_type == X11::getAtom(NET_ACTIVE_WINDOW)) {
		if (! client->isCfgDeny(CFG_DENY_ACTIVE_WINDOW)) {
			// Active child if it's not the active child
			if (client != _client) {
				activateChild(client);
			}

			// If we aren't mapped we check if we make sure we're
			// on the right workspace and then map the window.
			if (! _mapped) {
				if (_workspace != Workspaces::getActive()
				    && !isSticky()) {
					Workspaces::setWorkspace(_workspace,
								 false);
				}
				mapWindow();
			}
			// Seems as if raising the window is implied in
			// activating it
			raise();
			giveInputFocus();
		}
	} else if (ev->message_type == X11::getAtom(NET_CLOSE_WINDOW)) {
		client->close();
	} else if (ev->message_type == X11::getAtom(NET_WM_DESKTOP)) {
		if (client == _client) {
			setWorkspace(ev->data.l[0]);
		}
	} else if (ev->message_type == X11::getAtom(WM_CHANGE_STATE) &&
		   (ev->format == 32) && (ev->data.l[0] == IconicState)) {
		if (client == _client) {
			iconify();
		}
	} else if (ev->message_type == X11::getAtom(NET_WM_MOVERESIZE)
		   && ev->format == 32) {
		switch (ev->data.l[2]) {
		case NET_WM_MOVERESIZE_SIZE_TOPLEFT:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_TOP_LEFT);
			break;
		case NET_WM_MOVERESIZE_SIZE_TOP:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_TOP);
			break;
		case NET_WM_MOVERESIZE_SIZE_TOPRIGHT:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_TOP_RIGHT);
			break;
		case NET_WM_MOVERESIZE_SIZE_RIGHT:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_RIGHT);
			break;
		case NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_BOTTOM_RIGHT);
			break;
		case NET_WM_MOVERESIZE_SIZE_BOTTOM:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_BOTTOM);
			break;
		case NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_BOTTOM_LEFT);
			break;
		case NET_WM_MOVERESIZE_SIZE_LEFT:
			ae = &_ae_resize;
			ae->action_list[0].setParamI(0, BORDER_LEFT);
			break;
		case NET_WM_MOVERESIZE_MOVE:
			ae = &_ae_move;
			ae->action_list[0].setParamI(ev->data.l[0],
						     ev->data.l[1]);
			break;
		case NET_WM_MOVERESIZE_SIZE_KEYBOARD:
		case NET_WM_MOVERESIZE_MOVE_KEYBOARD:
			ae = &_ae_move_resize;
			break;
		}
	}
	return ae;
}

/**
 * Handle _NET_WM_STATE atom.
 */
void
Frame::handleClientStateMessage(XClientMessageEvent *ev, Client *client)
{
	StateAction sa;
	if (getStateActionFromMessage(ev, sa)) {
		handleStateAtom(sa, ev->data.l[1], client);
		if (ev->data.l[2] != 0) {
			handleStateAtom(sa, ev->data.l[2], client);
		}
		client->updateEwmhStates();
	}
}

/**
 * Get StateAction from NET_WM atom.
 */
bool
Frame::getStateActionFromMessage(XClientMessageEvent *ev, StateAction &sa)
{
	if (ev->data.l[0] == NET_WM_STATE_REMOVE) {
		sa = STATE_UNSET;
	} else if (ev->data.l[0] == NET_WM_STATE_ADD) {
		sa = STATE_SET;
	} else if (ev->data.l[0] == NET_WM_STATE_TOGGLE) {
		sa = STATE_TOGGLE;
	} else {
		P_DBG("unknown _NET_WM_STATE client message " << ev->data.l[0]);
		return false;
	}
	return true;
}

/**
 * Handle state atom for client.
 */
void
Frame::handleStateAtom(StateAction sa, Atom atom, Client *client)
{
	if (client == _client) {
		handleCurrentClientStateAtom(sa, atom, client);
	}

	switch (atom) {
	case STATE_SKIP_TASKBAR:
		client->setStateSkip(sa, SKIP_TASKBAR);
		break;
	case STATE_SKIP_PAGER:
		client->setStateSkip(sa, SKIP_PAGER);
		break;
	case STATE_DEMANDS_ATTENTION:
		bool ostate = client->demandsAttention();
		bool nstate = (sa == STATE_SET
			       || (sa == STATE_TOGGLE && ! ostate));
		client->setDemandsAttention(nstate);
		if (ostate != nstate) {
			if (ostate) {
				decrAttention();
			} else {
				incrAttention();
			}
		}
		break;
	}
}

/**
 * Handle state atom for actions that apply only on active client.
 */
void
Frame::handleCurrentClientStateAtom(StateAction sa, Atom atom, Client *client)
{
	if (atom == X11::getAtom(STATE_STICKY)) {
		setStateSticky(sa);
	}
	if (atom == X11::getAtom(STATE_MAXIMIZED_HORZ)
	    && ! client->isCfgDeny(CFG_DENY_STATE_MAXIMIZED_HORZ)) {
		setStateMaximized(sa, true, false, false);
	}
	if (atom == X11::getAtom(STATE_MAXIMIZED_VERT)
	    && ! client->isCfgDeny(CFG_DENY_STATE_MAXIMIZED_VERT)) {
		setStateMaximized(sa, false, true, false);
	}
	if (atom == X11::getAtom(STATE_SHADED)) {
		setShaded(sa);
	}
	if (atom == X11::getAtom(STATE_HIDDEN)
	    && ! client->isCfgDeny(CFG_DENY_STATE_HIDDEN)) {
		setStateIconified(sa);
	}
	if (atom == X11::getAtom(STATE_FULLSCREEN)
	    && ! client->isCfgDeny(CFG_DENY_STATE_FULLSCREEN)) {
		setStateFullscreen(sa);
	}
	if (atom == X11::getAtom(STATE_ABOVE)
	    && ! client->isCfgDeny(CFG_DENY_STATE_ABOVE)) {
		setStateAlwaysOnTop(sa);
	}
	if (atom == X11::getAtom(STATE_BELOW)
	    && ! client->isCfgDeny(CFG_DENY_STATE_BELOW)) {
		setStateAlwaysBelow(sa);
	}
}

void
Frame::handlePropertyChange(XPropertyEvent *ev, Client *client)
{
	if (ev->atom == X11::getAtom(NET_WM_DESKTOP)) {
		if (client == _client) {
			Cardinal workspace;
			if (X11::getCardinal(client->getWindow(),
					     NET_WM_DESKTOP, workspace)) {
				if (workspace != (Cardinal) _workspace)
					setWorkspace(workspace);
			}
		}
	} else if (ev->atom == X11::getAtom(NET_WM_STRUT)) {
		client->getStrutHint();
	} else if (ev->atom == X11::getAtom(NET_WM_NAME)
		   || ev->atom == XA_WM_NAME) {
		handleTitleChange(client, true);
	} else if (ev->atom == X11::getAtom(MOTIF_WM_HINTS)) {
		client->readMwmHints();
		if (! isFullscreen() && _client == client) {
			setBorder(_client->hasBorder()
					? STATE_SET
					: STATE_UNSET);
			setTitlebar(_client->hasTitlebar()
					? STATE_SET
					: STATE_UNSET);
		}
	} else if (ev->atom == XA_WM_NORMAL_HINTS) {
		client->getWMNormalHints();
	} else if (ev->atom == XA_WM_TRANSIENT_FOR) {
		client->getTransientForHint();
	} else if (ev->atom == X11::getAtom(WM_HINTS)) {
		bool ostate = client->demandsAttention();
		client->getWMHints();
		bool nstate = client->demandsAttention();
		if (ostate != nstate) {
			if (ostate) {
				decrAttention();
			} else {
				incrAttention();
			}
		}
	} else if (ev->atom == X11::getAtom(WM_PROTOCOLS)) {
		client->getWMProtocols();
	}
}

/**
 * Handle title change, find decoration rules based on changed title
 * and update if changed.
 */
void
Frame::handleTitleChange(Client *client, bool read_name)
{
	// Update title
	if (read_name) {
		client->readName();
	}

	if (client != _client || ! updateDecor()) {
		// Render title as either the title changed was not the active
		// title or the name change did not cause the decor to change.
		renderTitle();
	}
}

/**
 * Insert Frame into Workspaces and ensure client list is updated.
 */
void
Frame::workspacesInsert()
{
	Workspaces::insert(this);
	Workspaces::updateClientList();
}

/**
 * Remove Frame from Workspaces and ensure client list is updated.
 */
void
Frame::workspacesRemove()
{
	Workspaces::remove(this);
	Workspaces::updateClientList();
}
