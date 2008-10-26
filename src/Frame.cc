//
// Frame.cc for pekwm
// Copyright © 2002-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>

extern "C" {
#include <X11/Xatom.h>
}

#include "PWinObj.hh"
#include "PDecor.hh"
#include "Frame.hh"

#include "Compat.hh"
#include "PScreen.hh"
#include "Config.hh"
#include "ActionHandler.hh"
#include "AutoProperties.hh"
#include "Client.hh"
#include "ScreenResources.hh"
#include "StatusWindow.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"
#include "KeyGrabber.hh"
#ifdef HARBOUR
#include "Harbour.hh"
#endif // HARBOUR

#ifdef MENUS
#include "PMenu.hh"
#endif //MENUS

#include "Theme.hh"

using std::cerr;
using std::endl;
using std::find;
using std::list;
using std::mem_fun;
using std::string;
using std::vector;
using std::wstring;

list<Frame*> Frame::_frame_list = list<Frame*>();
vector<uint> Frame::_frameid_list = vector<uint>();

Frame* Frame::_tag_frame = 0;
bool Frame::_tag_behind = false;

//! @brief Frame constructor
Frame::Frame(Client *client, AutoProperty *ap)
    : PDecor(WindowManager::inst()->getScreen()->getDpy(), WindowManager::inst()->getTheme(),
             Frame::getClientDecorName(client)),
      _id(0), _client(0), _class_hint(0),
      _non_fullscreen_decor_state(0), _non_fullscreen_layer(LAYER_NORMAL)
{
    // setup basic pointers
    _scr = WindowManager::inst()->getScreen();

    // PWinObj attributes
    _type = WO_FRAME;

    // PDecor attributes
    _decor_cfg_child_move_overloaded = true;
    _decor_cfg_bpr_replay_pointer = true;
    _decor_cfg_bpr_al_title = MOUSE_ACTION_LIST_TITLE_FRAME;
    _decor_cfg_bpr_al_child = MOUSE_ACTION_LIST_CHILD_FRAME;

    // grab buttons so that we can reply them
    for (uint i = 0; i < BUTTON_NO; ++i) {
        XGrabButton(_dpy, i, AnyModifier, _window,
                    True, ButtonPressMask|ButtonReleaseMask,
                    GrabModeSync, GrabModeAsync, None, None);
    }

    // get unique id of the frame, if the client didn't have an id
    if (! WindowManager::inst()->isStartup()) {
        long id;

        if (AtomUtil::getLong(client->getWindow(),
                              PekwmAtoms::instance()->getAtom(PEKWM_FRAME_ID), id)) {
            _id = id;
        }

    } else {
        _id = findFrameID();
    }

    // get the clients class_hint
    _class_hint = new ClassHint();
    *_class_hint = *client->getClassHint();

    _scr->grabServer();

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
    if (client->isViewable() || client->isPlaced()) {
        moveChild(client->getX(), client->getY());
    } else if (client->setPUPosition()) {
        calcGravityPosition(client->getXSizeHints()->win_gravity,
                            client->getX(), client->getY(), _gm.x, _gm.y);
        move(_gm.x, _gm.y);
    } else {
        place = Config::instance()->isPlaceNew();
    }

    // override both position and size with autoproperties
    if (ap) {
        if (ap->isMask(AP_FRAME_GEOMETRY|AP_CLIENT_GEOMETRY)) {
            setupAPGeometry(client, ap);

            if (ap->frame_gm_mask&(XValue|YValue) ||
                    ap->client_gm_mask&(XValue|YValue)) {
                place = false;
            }
        }

        if (ap->isMask(AP_PLACE_NEW)) {
            place = ap->place_new;
        }
    }

    // still need a position?
    if (place) {
        Workspaces::instance()->placeWo(this, client->getTransientWindow());
    }

    _old_gm = _gm;
    _non_fullscreen_decor_state = client->getDecorState();
    _non_fullscreen_layer = client->getLayer();

    _scr->ungrabServer(true); // ungrab and sync

    // needs to be done before the workspace insert
    setLayer(client->getLayer());

    // I add these to the list before I insert the client into the frame to
    // be able to skip an extra updateClientList
    _frame_list.push_back(this);
    WindowManager::inst()->addToFrameList(this);
    Workspaces::instance()->insert(this);

    // now insert the client in the frame we created, do not give it focus.
    addChild(client);
    activateChild(client);

    // set the window states, shaded, maximized...
    getState(client);

    if (! _client->hasStrut()) {
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

//! @brief Frame destructor
Frame::~Frame(void)
{
    // remove from lists
    _wo_map.erase(_window);
    woListRemove(this);
    _frame_list.remove(this);
    Workspaces::instance()->remove(this);
    WindowManager::inst()->removeFromFrameList(this);
    if (_tag_frame == this) {
        _tag_frame = 0;
    }

    returnFrameID(_id);

    if (_class_hint) {
        delete _class_hint;
    }

    Workspaces::instance()->updateClientStackingList(true, true);
}

// START - PWinObj interface.

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

//! @brief Toggles the Frame's sticky state
void
Frame::stick(void)
{
    _client->setSticky(_sticky); // FIXME: FRAME
    _client->stick();

    _sticky = ! _sticky;

    // make sure it's visible/hidden
    PDecor::setWorkspace(Workspaces::instance()->getActive());
}

//! @brief Sets workspace on frame, wrapper to allow autoproperty loading
void
Frame::setWorkspace(uint workspace)
{
    // Duplicate the behavior done in PDecor::setWorkspace to have a sane
    // value on _workspace and not NET_WM_STICKY_WINDOW.
    if (workspace != NET_WM_STICKY_WINDOW) {
        // First we set the workspace, then load autoproperties for possible
        // overrun of workspace and then set the workspace.
        _workspace = workspace;
        readAutoprops(APPLY_ON_WORKSPACE);
        workspace = _workspace;
    }

    PDecor::setWorkspace(workspace);
}

// event handlers

//! @brief
ActionEvent*
Frame::handleMotionEvent(XMotionEvent *ev)
{
    // This is true when we have a title button pressed and then we don't want
    // to be able to drag windows around, therefore we ignore the event
    if (_button) {
        return 0;
    }

    ActionEvent *ae = 0;
    uint button = _scr->getButtonFromState(ev->state);

    if (ev->window == getTitleWindow()) {
        ae = ActionHandler::findMouseAction(button, ev->state, MOUSE_EVENT_MOTION,
                                            Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_TITLE_FRAME));
    } else if (ev->window == _client->getWindow()) {
        ae = ActionHandler::findMouseAction(button, ev->state, MOUSE_EVENT_MOTION,
                                            Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_CHILD_FRAME));
    } else {
        uint pos = getBorderPosition(ev->subwindow);

        // If ev->subwindow wasn't one of the border windows, perhaps ev->window is.
        if (pos == BORDER_NO_POS) {
            pos = getBorderPosition(ev->window);
        }
        
        if (pos != BORDER_NO_POS) {
            list<ActionEvent> *bl = Config::instance()->getBorderListFromPosition(pos);
            ae = ActionHandler::findMouseAction(button, ev->state, MOUSE_EVENT_MOTION, bl);
        }
    }

    // check motion threshold
    if (ae && (ae->threshold > 0)) {
        if (! ActionHandler::checkAEThreshold(ev->x_root, ev->y_root,
                                             _pointer_x, _pointer_y, ae->threshold)) {
            ae = 0;
        }
    }

    return ae;
}

//! @brief
ActionEvent*
Frame::handleEnterEvent(XCrossingEvent *ev)
{
    // Run event handler to get hoovering to work but ignore action
    // returned.
    PDecor::handleEnterEvent(ev);

    ActionEvent *ae = 0;
    list<ActionEvent> *al = 0;

    if (ev->window == getTitleWindow()) {
        al = Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_TITLE_FRAME);

    } else if (ev->subwindow == _client->getWindow()) {
        al = Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_CHILD_FRAME);

    } else {
        uint pos = getBorderPosition(ev->window);
        if (pos != BORDER_NO_POS) {
            al = Config::instance()->getBorderListFromPosition(pos);
        }
    }

    if (al) {
        ae = ActionHandler::findMouseAction(BUTTON_ANY, ev->state, MOUSE_EVENT_ENTER, al);
    }

    return ae;
}

//! @brief
ActionEvent*
Frame::handleLeaveEvent(XCrossingEvent *ev)
{
    // Run event handler to get hoovering to work but ignore action
    // returned.
    PDecor::handleLeaveEvent(ev);

    ActionEvent *ae;

    MouseActionListName ln = MOUSE_ACTION_LIST_TITLE_FRAME;
    if (ev->window == _client->getWindow()) {
        ln = MOUSE_ACTION_LIST_CHILD_FRAME;
    }

    ae = ActionHandler::findMouseAction(BUTTON_ANY, ev->state, MOUSE_EVENT_LEAVE,
                                        Config::instance()->getMouseActionList(ln));

    return ae;
}

//! @brief
ActionEvent*
Frame::handleMapRequest(XMapRequestEvent *ev)
{
    if (! _client || (ev->window != _client->getWindow())) {
        return 0;
    }

    if (! _sticky && (_workspace != Workspaces::instance()->getActive())) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Ignoring MapRequest, not on current workspace!" << endl;
#endif // DEBUG
        return 0;
    }

    mapWindow();

    return 0;
}

//! @brief
ActionEvent*
Frame::handleUnmapEvent(XUnmapEvent *ev)
{
    list<PWinObj*>::iterator it(_child_list.begin());
    for (; it != _child_list.end(); ++it) {
        if (*(*it) == ev->window) {
            (*it)->handleUnmapEvent(ev);
            break;
        }
    }

    return 0;
}

// END - PWinObj interface.

#ifdef HAVE_SHAPE
//! @brief
void
Frame::handleShapeEvent(XAnyEvent *ev)
{
    if (! _client || (ev->window != _client->getWindow())) {
        return;
    }
    _client->setShaped(setShape());
}
#endif // HAVE_SHAPE

// START - PDecor interface.

//! @brief Adds child to the frame.
void
Frame::addChild (PWinObj *child)
{
    PDecor::addChild (child);
    AtomUtil::setLong (child->getWindow (),
                       PekwmAtoms::instance ()->getAtom (PEKWM_FRAME_ID), _id);
    child->lower ();
}

//! @brief Removes child from the frame.
void
Frame::removeChild (PWinObj *child, bool do_delete)
{
    PDecor::removeChild (child, do_delete);
}

/**
 * Activates child in Frame, updating it's state and re-loading decor
 * rules to match updated title.
 */
void
Frame::activateChild (PWinObj *child)
{
    // Sync the frame state with the client only if we already had a client
    if (_client && _client != child) {
        applyState(static_cast<Client*>(child));
    }

    _client = static_cast<Client*>(child);
    PDecor::activateChild(child);

    // setShape uses current active child, so we need to activate the
    // child before setting shape
#ifdef HAVE_SHAPE
    if (PScreen::instance()->hasExtensionShape()) {
        _client->setShaped(setShape());
    }
#endif // HAVE_SHAPE

    if (_focused) {
        child->giveInputFocus();
    }
    
    // Reload decor rules if needed.
    handleTitleChange(_client);

    Workspaces::instance()->updateClientStackingList(true, true);
}

//! @brief
void
Frame::updatedChildOrder(void)
{
    titleClear();

    list<PWinObj*>::iterator it(_child_list.begin());
    for (; it != _child_list.end(); ++it) {
        titleAdd(static_cast<Client*>(*it)->getTitle());
    }

    updatedActiveChild();
}

//! @brief
void
Frame::updatedActiveChild(void)
{
    titleSetActive(0);

    list<PWinObj*>::iterator it(_child_list.begin());
    for (uint i = 0; it != _child_list.end(); ++i, ++it) {
        if (_child == *it) {
            titleSetActive(i);
            break;
        }
    }

    renderTitle();
}

//! @brief
void
Frame::getDecorInfo(wchar_t *buf, uint size)
{
    uint width, height;
    if (_client) {
        calcSizeInCells(width, height);
    } else {
        width = _gm.width;
        height = _gm.height;
    }
    swprintf(buf, size, L"%d+%d+%d+%d", width, height, _gm.x, _gm.y);
}

//! @brief
void
Frame::setShaded(StateAction sa)
{
    bool shaded = isShaded();
    PDecor::setShaded(sa);
    if (shaded != isShaded()) {
        _client->setShade(isShaded());
        _client->updateEwmhStates();
    }
}

//! @brief
int
Frame::resizeHorzStep(int diff) const
{
    if (! _client) {
        return diff;
    }

    int diff_ret = 0;
    uint min = _gm.width - getChildWidth();
    if (min == 0) { // borderless windows, we don't want X errors
        min = 1;
    }
    const XSizeHints *hints = _client->getXSizeHints(); // convenience

    // if we have ResizeInc hint set we use it instead of pixel diff
    if (hints->flags&PResizeInc) {
        if (diff > 0) {
            diff_ret = hints->width_inc;
        } else if ((_gm.width - hints->width_inc) >= min) {
            diff_ret = -hints->width_inc;
        }
    } else if ((_gm.width + diff) >= min) {
        diff_ret = diff;
    }

    // check max/min size hints
    if (diff > 0) {
        if ((hints->flags&PMaxSize) &&
                ((getChildWidth() + diff) > unsigned(hints->max_width))) {
            diff_ret = _gm.width - hints->max_width + min;
        }
    } else if ((hints->flags&PMinSize) &&
               ((getChildWidth() + diff) < unsigned(hints->min_width))) {
        diff_ret = _gm.width - hints->min_width + min;
    }

    return diff_ret;
}

//! @brief
int
Frame::resizeVertStep(int diff) const
{
    if (! _client) {
        return diff;
    }

    int diff_ret = 0;
    uint min = _gm.height - getChildHeight();
    if (min == 0) { // borderless windows, we don't want X errors
        min = 1;
    }
    const XSizeHints *hints = _client->getXSizeHints(); // convenience

    // if we have ResizeInc hint set we use it instead of pixel diff
    if (hints->flags&PResizeInc) {
        if (diff > 0) {
            diff_ret = hints->height_inc;
        } else if ((_gm.height - hints->height_inc) >= min) {
            diff_ret = -hints->height_inc;
        }
    } else {
        diff_ret = diff;
    }

    // check max/min size hints
    if (diff > 0) {
        if ((hints->flags&PMaxSize) &&
                ((getChildHeight() + diff) > unsigned(hints->max_height))) {
            diff_ret = _gm.height - hints->max_height + min;
        }
    } else if ((hints->flags&PMinSize) &&
               ((getChildHeight() + diff) < unsigned(hints->min_height))) {
        diff_ret = _gm.height - hints->min_width + min;
    }

    return diff_ret;
}

// END - PDecor interface.

//! @brief Sets _PEKWM_FRAME_ID on all children in the frame
void
Frame::setId(uint id)
{
    _id = id;

    list<PWinObj*>::iterator it(_child_list.begin());
    long atom = PekwmAtoms::instance()->getAtom(PEKWM_FRAME_ID);
    for (; it != _child_list.end(); ++it) {
        AtomUtil::setLong((*it)->getWindow(), atom, id);
    }
}

//! @brief Gets the state from the Client
void
Frame::getState(Client *cl)
{
    if (! cl)
        return;

    bool b_client_iconified = cl->isIconified ();

    if (_sticky != cl->isSticky())
        _sticky = ! _sticky;
    if (_maximized_horz != cl->isMaximizedHorz())
        setStateMaximized(STATE_TOGGLE, true, false, false);
    if (_maximized_vert != cl->isMaximizedVert())
        setStateMaximized(STATE_TOGGLE, false, true, false);
    if (isShaded() != cl->isShaded())
        setShaded(STATE_TOGGLE);
    if (_layer != cl->getLayer())
        setLayer(cl->getLayer());
    if (_workspace != cl->getWorkspace())
        PDecor::setWorkspace(cl->getWorkspace());

    // We need to set border and titlebar before setting fullscreen, as
    // fullscreen will unset border and titlebar if needed.
    if (hasBorder() != _client->hasBorder())
        setBorder(STATE_TOGGLE);
    if (hasTitlebar() != _client->hasTitlebar())
        setTitlebar(STATE_TOGGLE);
    if (_fullscreen != cl->isFullscreen())
        setStateFullscreen(STATE_TOGGLE);

    if (_iconified != b_client_iconified) {
        if (_iconified) {
            mapWindow();
        } else {
            iconify();
        }
    }

    if (_skip != cl->getSkip()) {
        setSkip(cl->getSkip());
    }
}

//! @brief Applies the frame's state on the Client
void
Frame::applyState(Client *cl)
{
    if (! cl) {
        return;
    }
    
    cl->setSticky(_sticky);
    cl->setMaximizedHorz(_maximized_horz);
    cl->setMaximizedVert(_maximized_vert);
    cl->setShade(isShaded());
    cl->setWorkspace(_workspace);
    cl->setLayer(_layer);

    // fix border / titlebar state
    cl->setBorder(hasBorder());
    cl->setTitlebar(hasTitlebar());
    // make sure the window has the correct mapped state
    if (_mapped != cl->isMapped()) {
        if (! _mapped) {
            cl->unmapWindow();
        } else {
            cl->mapWindow();
        }
    }

    cl->updateEwmhStates();
}

//! @brief Sets skip state.
void
Frame::setSkip(uint skip)
{
    PDecor::setSkip(skip);

    // Propagate changes on client as well
    if (_client) {
        _client->setSkip(skip);
    }
}

//! @brief Find Frame with Window
//! @param win Window to search for.
//! @return Frame if found, else 0.
Frame*
Frame::findFrameFromWindow(Window win)
{
    // Validate input window.
    if ((win == None) || (win == PScreen::instance()->getRoot())) {
        return 0;
    }

    list<Frame*>::iterator it(_frame_list.begin());
    for(; it !=_frame_list.end(); ++it) {
        if (win == (*it)->getWindow()) { // operator == does more than that
            return (*it);
        }
    }

    return 0;
}

//! @brief Find Frame with id.
//! @param id ID to search for.
//! @return Frame if found, else 0.
Frame*
Frame::findFrameFromID(uint id)
{
    list<Frame*>::iterator it(Frame::frame_begin());
    for (; it != Frame::frame_end(); ++it) {
        if ((*it)->getId() == id) {
            return (*it);
        }
    }

    return 0;
}

//! @brief
void
Frame::setupAPGeometry(Client *client, AutoProperty *ap)
{
    // frame geomtry overides client geometry

    // get client geometry
    if (ap->isMask(AP_CLIENT_GEOMETRY)) {
        Geometry gm(client->_gm);
        applyAPGeometry(gm, ap->client_gm, ap->client_gm_mask);

        if (ap->client_gm_mask&(XValue|YValue)) {
            moveChild(gm.x, gm.y);
        }
        if(ap->client_gm_mask&(WidthValue|HeightValue)) {
            resizeChild(gm.width, gm.height);
        }
    }

    // get frame geometry
    if (ap->isMask(AP_FRAME_GEOMETRY)) {
        applyAPGeometry(_gm, ap->frame_gm, ap->frame_gm_mask);
        if (ap->frame_gm_mask&(XValue|YValue)) {
            move(_gm.x, _gm.y);
        }
        if (ap->frame_gm_mask&(WidthValue|HeightValue)) {
            resize(_gm.width, _gm.height);
        }
    }
}

//! @brief Apply autoproperties geometry.
//! @param gm Geometry to modify.
//! @param ap_gm Geometry to get values from.
//! @param mask Geometry mask.
void
Frame::applyAPGeometry(Geometry &gm, const Geometry &ap_gm, int mask)
{
    // Read size before position so negative position works, if size is
    // < 1 consider it to be full screen size.
    if (mask&WidthValue) {
        if (ap_gm.width < 1) {
            gm.width = _scr->getWidth();
        } else {
            gm.width = ap_gm.width;
        }
    }
    
    if (mask&HeightValue) {
        if (ap_gm.height < 1) {
        } else {
            gm.height = ap_gm.height;
        }
    }

    // Read position
    if (mask&XValue) {
        gm.x = ap_gm.x;
        if (mask&XNegative) {
            gm.x += _scr->getWidth() - gm.width;
        }
    }
    if (mask&YValue) {
        gm.y = ap_gm.y;
        if (mask&YNegative) {
            gm.y += _scr->getHeight() - gm.height;
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
        // No free, get next number (Frame is not in list when this is called.)
        id = _frame_list.size() + 1;
    }

    return id;
}

//! @brief Returns Frame ID to used frame id list.
//! @param id ID to return.
void
Frame::returnFrameID(uint id)
{
    vector<uint>::iterator it(_frameid_list.begin());
    for (; it != _frameid_list.end() && id < *it; ++it)
        ;
    _frameid_list.insert(it, id);
}

/**
 * Return decor name matching clients property, defaults to
 * PDecor::DEFAULT_DECOR_NAME
 */
std::string
Frame::getClientDecorName(Client *client)
{
  DecorProperty *prop = WindowManager::inst()->getAutoProperties()->findDecorProperty(client->getClassHint());
  return prop ? prop->getName() : PDecor::DEFAULT_DECOR_NAME;
}

//! @brief Resets Frame IDs.
void
Frame::resetFrameIDs(void)
{
    list<Frame*>::iterator it(_frame_list.begin());
    for (uint id = 1; it != _frame_list.end(); ++id, ++it) {
        (*it)->setId(id);
    }
}


//! @brief Removes the client from the Frame and creates a new Frame for it
void
Frame::detachClient(Client *client)
{
    if (client->getParent() != this) {
        return;
    }

    if (_child_list.size() > 1) {
        removeChild(client);

        client->move(_gm.x, _gm.y + borderTop());
        Frame *frame = new Frame(client, 0);

        client->setParent(frame);
        client->setWorkspace(Workspaces::instance()->getActive());

        setFocused(false);
    }
}

//! @brief Makes sure the frame doesn't cover any struts / the harbour.
bool
Frame::fixGeometry(void)
{
    Geometry head, before;
    PScreen::instance()->getHeadInfoWithEdge(getNearestHead(), head);

    before = _gm;

    // fix size
    if (_gm.width > head.width) {
        _gm.width = head.width;
    } if (_gm.height > head.height) {
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

//! @brief Initiates grouping move, based on a XMotionEvent.
void
Frame::doGroupingDrag(XMotionEvent *ev, Client *client, bool behind) // FIXME: rewrite
{
    if (! client) {
        return;
    }

    int o_x, o_y;
    o_x = ev ? ev->x_root : 0;
    o_y = ev ? ev->y_root : 0;

    wstring name(L"Grouping ");
    if (client->getTitle()->getVisible().size() > 0) {
        name += client->getTitle()->getVisible();
    } else {
        name += L"No Name";
    }

    bool status = _scr->grabPointer(_scr->getRoot(),
                                    ButtonReleaseMask|PointerMotionMask, None);
    if (status != true) {
        return;
    }

    StatusWindow *sw = StatusWindow::instance();

    sw->draw(name); // resize window and render bg
    sw->move(o_x, o_y);
    sw->mapWindowRaised();
    sw->draw(name); // redraw after map

    XEvent e;
    while (true) { // this breaks when we get an button release
        XMaskEvent(_dpy, PointerMotionMask|ButtonReleaseMask, &e);

        switch (e.type)  {
        case MotionNotify:
            // update the position
            o_x = e.xmotion.x_root;
            o_y = e.xmotion.y_root;

            sw->move(o_x, o_y);
            sw->draw(name);
            break;

        case ButtonRelease:
            sw->unmapWindow();
            _scr->ungrabPointer();

            Client *search = 0;

            // only group if we have grouping turned on
            if (WindowManager::inst()->isAllowGrouping()) {
                int x, y;
                Window win;

                // find the frame we dropped the client on
                XTranslateCoordinates(_dpy, _scr->getRoot(), _scr->getRoot(),
                                      e.xmotion.x_root, e.xmotion.y_root,
                                      &x, &y, &win);

                search = Client::findClient(win);
            }

            // if we found a client, and it's not in the current frame and
            // it has a "normal" ( make configurable? ) layer we group
            if (search && search->getParent() &&
                    (search->getParent() != this) &&
                    (search->getLayer() > LAYER_BELOW) &&
                    (search->getLayer() < LAYER_ONTOP)) {

                // if we currently have focus and the frame exists after we remove
                // this client we need to redraw it as unfocused
                bool focus = behind ? false : (_child_list.size() > 1);

                removeChild(client);

                Frame *frame = static_cast<Frame*>(search->getParent());
                frame->addChild(client);
                if (! behind) {
                    frame->activateChild(client);
                    frame->giveInputFocus();
                }

                if (focus) {
                    setFocused(false);
                }

            }  else if (_child_list.size() > 1) {
                // if we have more than one client in the frame detach this one
                removeChild(client);

                client->move(e.xmotion.x_root, e.xmotion.y_root);

                Frame *frame = new Frame(client, 0);
                client->setParent(frame);

                // make sure the client ends up on the current workspace
                client->setWorkspace(Workspaces::instance()->getActive());

                // make sure it get's focus
                setFocused(false);
                frame->giveInputFocus();
            }

            return;
        }
    }
}

//! @brief Initiates resizing of a window based on motion event
void
Frame::doResize(XMotionEvent *ev)
{
    if (! ev) {
        return;
    }
    
    // figure out which part of the window we are in
    bool left = false, top = false;
    if (ev->x < signed(_gm.width / 2)) {
        left = true;
    }
    if (ev->y < signed(_gm.height / 2)) {
        top = true;
    }
    
    doResize(left, true, top, true);
}

//! @brief Initiates resizing of a window based border position
void
Frame::doResize(BorderPosition pos)
{
    bool x = false, y = false;
    bool left = false, top = false, resize = true;

    switch (pos) {
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

    if (resize) {
        doResize(left, x, top, y);
    }
}

//! @brief Resizes the frame by handling MotionNotify events.
void
Frame::doResize(bool left, bool x, bool top, bool y)
{
    if (! _client->allowResize() || isShaded()) {
        return;
    }

    if (! _scr->grabPointer(_scr->getRoot(), ButtonMotionMask|ButtonReleaseMask,
                           ScreenResources::instance()->getCursor(ScreenResources::CURSOR_RESIZE))) {
        return;
    }

    setShaded(STATE_UNSET); // make sure the frame isn't shaded

    // Initialize variables
    int start_x, new_x;
    int start_y, new_y;
    uint last_width, old_width;
    uint last_height, old_height;

    start_x = new_x = left ? _gm.x : (_gm.x + _gm.width);
    start_y = new_y = top ? _gm.y : (_gm.y + _gm.height);
    last_width = old_width = _gm.width;
    last_height = old_height = _gm.height;

    // the basepoint of our window
    _click_x = left ? (_gm.x + _gm.width) : _gm.x;
    _click_y = top ? (_gm.y + _gm.height) : _gm.y;

    int pointer_x = _gm.x, pointer_y = _gm.y;
    _scr->getMousePosition(pointer_x, pointer_y);

    wchar_t buf[128];
    getDecorInfo(buf, 128);

    StatusWindow *sw = StatusWindow::instance();
    if (Config::instance()->isShowStatusWindow()) {
        sw->draw(buf, true);
        sw->mapWindowRaised();
        sw->draw(buf);
    }

    bool outline = ! Config::instance()->getOpaqueResize();

    // grab server, we don't want invert traces
    if (outline) {
        _scr->grabServer();
    }

    XEvent ev;
    bool exit = false;
    while (exit != true) {
        if (outline) {
            drawOutline(_gm);
        }
        XMaskEvent(_dpy, ButtonPressMask|ButtonReleaseMask|ButtonMotionMask, &ev);
        if (outline) {
            drawOutline(_gm); // clear
        }

        switch (ev.type) {
        case MotionNotify:
            if (x) {
                new_x = start_x - pointer_x + ev.xmotion.x;
            }
            if (y) {
                new_y = start_y - pointer_y + ev.xmotion.y;
            }

            recalcResizeDrag(new_x, new_y, left, top);

            getDecorInfo(buf, 128);
            if (Config::instance()->isShowStatusWindow()) {
                sw->draw(buf, true);
            }

            // only updated when needed when in opaque mode
            if (! outline) {
                if ((old_width != _gm.width) || (old_height != _gm.height)) {
                    moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
                }
                old_width = _gm.width;
                old_height = _gm.height;
            }
            break;
        case ButtonRelease:
            exit = true;
            XPutBackEvent(_dpy, &ev);
            break;
        }
    }

    if (Config::instance()->isShowStatusWindow()) {
        sw->unmapWindow();
    }

    _scr->ungrabPointer();

    // Make sure the state isn't set to maximized after we've resized.
    if (_maximized_horz || _maximized_vert) {
        _maximized_horz = false;
        _maximized_vert = false;
        _client->setMaximizedHorz(false);
        _client->setMaximizedVert(false);
        _client->updateEwmhStates();
    }

    if (outline) {
        moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
        _scr->ungrabServer(true);
    }
}

//! @brief Updates the width, height of the frame when resizing it.
void
Frame::recalcResizeDrag(int nx, int ny, bool left, bool top)
{
    uint brdr_lr = borderLeft() + borderRight();
    uint brdr_tb = borderTop() + borderBottom();

    if (left) {
        if (nx >= signed(_click_x - brdr_lr))
            nx = _click_x - brdr_lr - 1;
    } else {
        if (nx <= signed(_click_x + brdr_lr))
            nx = _click_x + brdr_lr + 1;
    }

    if (top) {
        if (ny >= signed(_click_y - getTitleHeight() - brdr_tb))
            ny = _click_y - getTitleHeight() - brdr_tb - 1;
    } else {
        if (ny <= signed(_click_y + getTitleHeight() + brdr_tb))
            ny = _click_y + getTitleHeight() + brdr_tb + 1;
    }

    uint width = left ? (_click_x - nx) : (nx - _click_x);
    uint height = top ? (_click_y - ny) : (ny - _click_y);

    width -= brdr_lr;
    height -= brdr_tb + getTitleHeight();
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
    _gm.height = height + getTitleHeight() + brdr_tb;

    _gm.x = left ? (_click_x - _gm.width) : _click_x;
    _gm.y = top ? (_click_y - _gm.height) : _click_y;
}

//! @brief Moves the Frame to the screen edge ori ( considering struts )
void
Frame::moveToEdge(OrientationType ori)
{
    uint head_nr;
    Geometry head, real_head;

    head_nr = getNearestHead();
    _scr->getHeadInfo(head_nr, real_head);
    _scr->getHeadInfoWithEdge(head_nr, head);

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
    if (! _client) {
        return;
    }
    
    list<PWinObj*>::iterator it(_child_list.begin());
    for (; it != _child_list.end(); ++it) {
        if (*it != _client) {
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
    // we don't want to maximize transients
    if (_client->getTransientWindow()) {
        return;
    }

    setShaded(STATE_UNSET);

    // make sure the two states are in sync if toggling
    if ((horz == vert) && (sa == STATE_TOGGLE)) {
        if (_maximized_horz != _maximized_vert) {
            horz = ! _maximized_horz;
            vert = ! _maximized_vert;
        }
    }

    XSizeHints *size_hint = _client->getXSizeHints(); // convenience

    Geometry head;
    _scr->getHeadInfoWithEdge(getNearestHead(), head);

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
        // maximize
        if ((sa == STATE_SET) ||
                ((sa == STATE_TOGGLE) && ! _maximized_horz)) {
            uint h_decor = _gm.width - getChildWidth();

            if (! fill) {
                _old_gm.x = _gm.x;
                _old_gm.width = _gm.width;
            }

            _gm.x = max_x;
            _gm.width = max_r - max_x;

            if ((size_hint->flags&PMaxSize) &&
                    (_gm.width > (size_hint->max_width + h_decor))) {
                _gm.width = size_hint->max_width + h_decor;
            }
            // demaximize
        } else if ((sa == STATE_UNSET) ||
                   ((sa == STATE_TOGGLE) && _maximized_horz)) {
            _gm.x = _old_gm.x;
            _gm.width = _old_gm.width;
        }

        // we unset the maximized state if we use maxfill
        _maximized_horz = fill ? false : ! _maximized_horz;
        _client->setMaximizedHorz(_maximized_horz);
    }

    if (vert && (fill || _client->allowMaximizeVert())) {
        // maximize
        if ((sa == STATE_SET) ||
                ((sa == STATE_TOGGLE) && ! _maximized_vert)) {
            uint v_decor = _gm.height - getChildHeight();

            if (! fill) {
                _old_gm.y = _gm.y;
                _old_gm.height = _gm.height;
            }

            _gm.y = max_y;
            _gm.height = max_b - max_y;

            if ((size_hint->flags&PMaxSize) &&
                    (_gm.height > (size_hint->max_height + v_decor))) {
                _gm.height = size_hint->max_height + v_decor;
            }
            // demaximize
        } else if ((sa == STATE_UNSET) ||
                   ((sa == STATE_TOGGLE) && _maximized_vert)) {
            _gm.y = _old_gm.y;
            _gm.height = _old_gm.height;
        }

        // we unset the maximized state if we use maxfill
        _maximized_vert = fill ? false : ! _maximized_vert;
        _client->setMaximizedVert(_maximized_vert);
    }

    fixGeometry(); // harbour already considered
    downSize(true, true); // keep x and keep y ( make conform to inc size )

    moveResize(_gm.x, _gm.y, _gm.width, _gm.height);

    _client->updateEwmhStates();
}

//! @brief Set fullscreen state
void
Frame::setStateFullscreen(StateAction sa)
{
    if (! ActionUtil::needToggle(sa, _fullscreen)) {
        return;
    }

    bool lock = _client->setConfigureRequestLock(true);

    if (_fullscreen) {
        if ((_non_fullscreen_decor_state&DECOR_BORDER) != hasBorder()) {
            setBorder(STATE_TOGGLE);
        }
        if ((_non_fullscreen_decor_state&DECOR_TITLEBAR) != hasTitlebar()) {
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
        uint nr = getNearestHead();
        _scr->getHeadInfo(nr, head);

        _gm = head;
    }

    _fullscreen = !_fullscreen;
    _client->setFullscreen(_fullscreen);

    moveResize(_gm.x, _gm.y, _gm.width, _gm.height);

    // Re-stack window if fullscreen is above other windows.
    if (Config::instance()->isFullscreenAbove()) {
        _layer = _fullscreen ? LAYER_ABOVE_DOCK : _non_fullscreen_layer;
        raise();
    }
    
    _client->setConfigureRequestLock(lock);
    _client->configureRequestSend();

    _client->updateEwmhStates();
}

//! @brief
void
Frame::setStateSticky(StateAction sa)
{
    if (ActionUtil::needToggle(sa, _sticky)) {
        stick();
    }
}

//! @brief
void
Frame::setStateAlwaysOnTop(StateAction sa)
{
    if (! ActionUtil::needToggle(sa, _layer == LAYER_ONTOP)) {
        return;
    }

    _client->alwaysOnTop(_layer < LAYER_ONTOP);
    setLayer(_client->getLayer());

    raise();
}

//! @brief
void
Frame::setStateAlwaysBelow(StateAction sa)
{
    if (! ActionUtil::needToggle(sa, _layer == LAYER_BELOW)) {
        return;
    }

    _client->alwaysBelow(_layer > LAYER_BELOW);
    setLayer(_client->getLayer());

    lower();
}

//! @brief Hides/Shows the border depending on _client
//! @param sa State to set
void
Frame::setStateDecorBorder(StateAction sa)
{
    bool border = hasBorder();

    setBorder(sa);

    // state changed, update client and atom state
    if (border != hasBorder()) {
        _client->setBorder(hasBorder());

        // update the _PEKWM_FRAME_DECOR hint
        AtomUtil::setLong(_client->getWindow(),
                          PekwmAtoms::instance()->getAtom(PEKWM_FRAME_DECOR),
                          _client->getDecorState());
    }
}

//! @brief Hides/Shows the titlebar depending on _client
//! @param sa State to set
void
Frame::setStateDecorTitlebar(StateAction sa)
{
    bool titlebar = hasTitlebar();

    setTitlebar(sa);

    // state changed, update client and atom state
    if (titlebar != hasTitlebar()) {
        _client->setTitlebar(hasTitlebar());

        AtomUtil::setLong(_client->getWindow(),
                          PekwmAtoms::instance()->getAtom(PEKWM_FRAME_DECOR),
                          _client->getDecorState());
    }
}

//! @brief
void
Frame::setStateIconified(StateAction sa)
{
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

//! @brief
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
Frame::setStateTitle(StateAction sa, Client *client, const std::wstring &title)
{
    if (sa == STATE_SET) {
        client->getTitle()->setUser(title);

    } else if (sa == STATE_UNSET) {
        client->getTitle()->setUser(L"");
        client->getXClientName();
    } else {
        if (client->getTitle()->isUserSet()) {
            client->getTitle()->setUser(L"");
        } else {
            client->getTitle()->setUser(title);
        }
    }

    // Set PEKWM_TITLE atom to preserve title on client between sessions.
    AtomUtil::setString(client->getWindow(),
                        PekwmAtoms::instance()->getAtom(PEKWM_TITLE),
                        Util::to_mb_str(client->getTitle()->getUser()));
    

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

// STATE actions end

//! @brief
void
Frame::getMaxBounds(int &max_x,int &max_r, int &max_y, int &max_b)
{
    int f_r, f_b;
    int x, y, h, w, r, b;

    f_r = getRX();
    f_b = getBY();

    list<Frame*>::iterator it = _frame_list.begin();
    for (; it != _frame_list.end(); ++it) {
        if (! (*it)->isMapped()) {
            continue;
        }

        x = (*it)->getX();
        y = (*it)->getY();
        h = (*it)->getHeight();
        w = (*it)->getWidth();
        r = (*it)->getRX();
        b = (*it)->getBY();

        // update max borders when other frame border lies between
        // this border and prior max border (originally screen/head edge)
        if ((r >= max_x) && (r <= _gm.x) && ! ((y >= f_b) || (b <= _gm.y))) {
            max_x = r;
        }
        if ((x <= max_r) && (x >= f_r) && ! ((y >= f_b) || (b <= _gm.y))) {
            max_r = x;
        }
        if ((b >= max_y) && (b <= _gm.y) && ! ((x >= f_r) || (r <= _gm.x))) {
            max_y = b;
        }
        if ((y <= max_b) && (y >= f_b) && ! ((x >= f_r) || (r <= _gm.x))) {
            max_b = y;
        }
    }
}

//! @brief
void
Frame::growDirection(uint direction)
{
    Geometry head;
    _scr->getHeadInfoWithEdge(getNearestHead(), head);

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

    downSize((direction != DIRECTION_LEFT), (direction != DIRECTION_UP));

    moveResize(_gm.x, _gm.y, _gm.width, _gm.height);
}

//! @brief Closes the frame and all clients in it
void
Frame::close(void)
{
    list<PWinObj*>::iterator it(_child_list.begin());
    for (; it != _child_list.end(); ++it) {
        static_cast<Client*>(*it)->close();
    }
}

//! @brief Reads autoprops for the active client.
//! @param type Defaults to APPLY_ON_RELOAD
void
Frame::readAutoprops(uint type)
{
    if ((type != APPLY_ON_RELOAD) && (type != APPLY_ON_WORKSPACE))
        return;

    _class_hint->title = _client->getTitle()->getReal();
    AutoProperty *data =
        AutoProperties::instance()->findAutoProperty(_class_hint, _workspace, type);
    _class_hint->title = L"";

    if (! data) {
        return;
    }

    // Set the correct group of the window
    _class_hint->group = data->group_name;

    if (_class_hint == _client->getClassHint() &&
            (_client->getTransientWindow() &&
             ! data->isApplyOn(APPLY_ON_TRANSIENT))) {
        return;
    }

    if (data->isMask(AP_STICKY) && _sticky != data->sticky) {
        stick();
    }
    if (data->isMask(AP_SHADED) && (isShaded() != data->shaded)) {
        setShaded(STATE_UNSET);
    }
    if (data->isMask(AP_MAXIMIZED_HORIZONTAL) &&
            (_maximized_horz != data->maximized_horizontal)) {
        setStateMaximized(STATE_TOGGLE, true, false, false);
    }
    if (data->isMask(AP_MAXIMIZED_VERTICAL) &&
            (_maximized_vert != data->maximized_vertical)) {
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
        // In order to avoid eternal recursion, the workspace is only set here
        // and then actually called PDecor::setWorkspace from Frame::setWorkspace
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

//! @brief Figure out how large the frame is in cells.
void
Frame::calcSizeInCells(uint &width, uint &height)
{
    const XSizeHints *hints = _client->getXSizeHints();

    if (hints->flags&PResizeInc) {
        width = (getChildWidth() - hints->base_width) / hints->width_inc;
        height = (getChildHeight() - hints->base_height) / hints->height_inc;
    } else {
        width = _gm.width;
        height = _gm.height;
    }
}

//! @brief Calculates position based on current gravity
void
Frame::calcGravityPosition(int gravity, int x, int y, int &g_x, int &g_y)
{
    switch (gravity) {
    case NorthEastGravity: // outside border corner
        g_x = x - _gm.x;
        g_y = y;
        break;
    case SouthWestGravity: // outside border corner
        g_x = x;
        g_y = y - _gm.y;
        break;
    case SouthEastGravity: // outside border corner
        g_x = x - _gm.x;
        g_y = y - _gm.y;
        break;

    case NorthGravity: // outside border center
        g_x = x - (_gm.x / 2);
        g_y = y;
        break;
    case SouthGravity: // outside border center
        g_x = x - (_gm.x / 2);
        g_y = y - _gm.y;
        break;
    case WestGravity: // outside border center
        g_x = x;
        g_y = y - (_gm.y / 2);
        break;
    case EastGravity: // outside border center
        g_x = x - _gm.x;
        g_y = y - (_gm.y / 2);
        break;

    case CenterGravity: // center of window
        g_x = x - (_gm.x / 2);
        g_y = y - (_gm.y / 2);
        break;
    case StaticGravity: // client top left
        g_x = x - (_gm.width - getChildWidth());
        g_y = y - (_gm.height - getChildHeight());
        break;
    case NorthWestGravity: // outside border corner
    default:
        g_x = x;
        g_y = y;
        break;
    }
}

//! @brief Makes Frame conform to Clients width and height inc
void
Frame::downSize(bool keep_x, bool keep_y)
{
    XSizeHints *size_hint = _client->getXSizeHints(); // convenience

    // conform to width_inc
    if (size_hint->flags&PResizeInc) {
        int o_r = getRX();
        int b_x = (size_hint->flags&PBaseSize)
                  ? size_hint->base_width
                  : (size_hint->flags&PMinSize) ? size_hint->min_width : 0;

        _gm.width -= (getChildWidth() - b_x) % size_hint->width_inc;
        if (! keep_x) {
            _gm.x = o_r - _gm.width;
        }
    }

    // conform to height_inc
    if (size_hint->flags&PResizeInc) {
        int o_b = getBY();
        int b_y = (size_hint->flags&PBaseSize)
                  ? size_hint->base_height
                  : (size_hint->flags&PMinSize) ? size_hint->min_height : 0;

        _gm.height -= (getChildHeight() - b_y) % size_hint->height_inc;
        if (! keep_y) {
            _gm.y = o_b - _gm.height;
        }
    }
}

// Below this Client message handling is done

//! @brief Handle XConfgiureRequestEvents
//! @todo Should we send a ConfigureRequest back to the Client if ignoring?
void
Frame::handleConfigureRequest(XConfigureRequestEvent *ev, Client *client)
{
    if (client != _client) {
        return; // only handle the active client's events
    }

    // Handled geometry, this is handled seperatley due to fullscreen
    // detection
    handleConfigureRequestGeometry(ev, client);

    // update the stacking
    if (! client->isCfgDeny(CFG_DENY_STACKING)) {
        if (ev->value_mask&CWStackMode) {
            if (ev->value_mask&CWSibling) {
                switch(ev->detail) {
                case Above:
                    Workspaces::instance()->stackAbove(this, ev->above);
                    break;
                case Below:
                    Workspaces::instance()->stackBelow(this, ev->above);
                    break;
                case TopIf:
                case BottomIf:
                    // FIXME: What does occlude mean?
                    break;
                }
            } else {
                switch(ev->detail) { // FIXME: Is this broken?
                case Above:
                    raise();
                    break;
                case Below:
                    lower();
                    break;
                case TopIf:
                case BottomIf:
                    // FIXME: Why does the manual say that it should care about siblings
                    // even if we don't have any specified?
                    break;
                }
            }
        }
    }
}

/**
 * Handle size and position part of configure request, detects
 * fullscreen mode if detection is enabled.
 */
void
Frame::handleConfigureRequestGeometry(XConfigureRequestEvent *ev, Client *client)
{
    // Look for fullscreen requests
    long all_geometry = CWX|CWY|CWWidth|CWHeight;
    bool is_fullscreen = false;
    if (Config::instance()->isFullscreenDetect()
        && ! client->isCfgDeny(CFG_DENY_SIZE)
        && ! client->isCfgDeny(CFG_DENY_POSITION)
        && ((ev->value_mask&all_geometry) == static_cast<unsigned>(all_geometry))) {

        Geometry gm_request(ev->x, ev->y, ev->width, ev->height);
        if (gm_request == _scr->getScreenGeometry()
             || gm_request == _scr->getHeadGeometry(_scr->getNearestHead(ev->x, ev->y))) {

            is_fullscreen = true;
            setStateFullscreen(STATE_SET);
        }
    }

    if (! is_fullscreen) {
        // Remove fullscreen state if client changes it size
        if (Config::instance()->isFullscreenDetect()) {
            setStateFullscreen(STATE_UNSET);
        }

        if (! client->isCfgDeny(CFG_DENY_SIZE)
                && (ev->value_mask & (CWWidth|CWHeight)) ) {

            resizeChild(ev->width, ev->height);
            _client->setShaped(setShape());
        }

        if (! client->isCfgDeny(CFG_DENY_POSITION)
               && (ev->value_mask & (CWX|CWY)) ) {

            calcGravityPosition(_client->getXSizeHints()->win_gravity,
                                ev->x, ev->y, _gm.x, _gm.y);
            move(_gm.x, _gm.y);
        }
    }
}

//! @brief
void
Frame::handleClientMessage(XClientMessageEvent *ev, Client *client)
{
    EwmhAtoms *ewmh = EwmhAtoms::instance(); // convenience

    StateAction sa;

    if (ev->message_type == ewmh->getAtom(STATE)) {
        if (ev->data.l[0]== NET_WM_STATE_REMOVE) {
            sa = STATE_UNSET;
        } else if (ev->data.l[0]== NET_WM_STATE_ADD) {
            sa = STATE_SET;
        } else if (ev->data.l[0]== NET_WM_STATE_TOGGLE) {
            sa = STATE_TOGGLE;
        } else {
#ifdef DEBUG
            cerr << __FILE__ << "@" << __LINE__ << ": "
                 << "None of StateAdd, StateRemove or StateToggle." << endl;
#endif // DEBUG
            return;
        }

#define IS_STATE(S) ((ev->data.l[1] == long(ewmh->getAtom(S))) || (ev->data.l[2] == long(ewmh->getAtom(S))))

        // actions that only is going to be applied on the active client
        if (client == _client) {
            // there is no modal support in pekwm yet
// 			if (IS_STATE(STATE_MODAL)) {
// 				is_modal=true;
// 			}
            if (IS_STATE(STATE_STICKY)) {
                setStateSticky(sa);
            }
            if (IS_STATE(STATE_MAXIMIZED_HORZ)
                    && ! client->isCfgDeny(CFG_DENY_STATE_MAXIMIZED_HORZ)) {
                setStateMaximized(sa, true, false, false);
            }
            if (IS_STATE(STATE_MAXIMIZED_VERT)
                    && ! client->isCfgDeny(CFG_DENY_STATE_MAXIMIZED_VERT)) {
                setStateMaximized(sa, false, true, false);
            }
            if (IS_STATE(STATE_SHADED)) {
                setShaded(sa);
            }
            if (IS_STATE(STATE_HIDDEN)
                    && ! client->isCfgDeny(CFG_DENY_STATE_HIDDEN)) {
                setStateIconified(sa);
            }
            if (IS_STATE(STATE_FULLSCREEN)
                    && ! client->isCfgDeny(CFG_DENY_STATE_FULLSCREEN)) {
                setStateFullscreen(sa);
            }
            if (IS_STATE(STATE_ABOVE)
                    && ! client->isCfgDeny(CFG_DENY_STATE_ABOVE)) {
                setStateAlwaysOnTop(sa);
            }
            if (IS_STATE(STATE_BELOW)
                    && ! client->isCfgDeny(CFG_DENY_STATE_BELOW)) {
                setStateAlwaysBelow(sa);
            }
        }

        if (IS_STATE(STATE_SKIP_TASKBAR)) {
            client->setStateSkip(sa, SKIP_TASKBAR);
        }
        if (IS_STATE(STATE_SKIP_PAGER)) {
            client->setStateSkip(sa, SKIP_PAGER);
        }

        client->updateEwmhStates();

    } else if (ev->message_type == ewmh->getAtom(NET_ACTIVE_WINDOW)) {
        if (! client->isCfgDeny(CFG_DENY_ACTIVE_WINDOW)) {
            // Active child if it's not the active child
            if (client != _client) {
                activateChild(client);
	        }

            // If we aren't mapped we check if we make sure we're on the right
            // workspace and then map the window.
            if (! _mapped) {
                if (_workspace != Workspaces::instance()->getActive()) {
                    Workspaces::instance()->setWorkspace(_workspace, false);
                }
                mapWindow();
            }
            // Seems as if raising the window is implied in activating it
            raise();
            giveInputFocus();
        }
    } else if (ev->message_type == ewmh->getAtom(NET_CLOSE_WINDOW)) {
        client->close();
    } else if (ev->message_type == ewmh->getAtom(NET_WM_DESKTOP)) {
        if (client == _client) {
            setWorkspace(ev->data.l[0]);
        }
    } else if (ev->message_type == IcccmAtoms::instance()->getAtom(WM_CHANGE_STATE) &&
               (ev->format == 32) && (ev->data.l[0] == IconicState)) {
        if (client == _client) {
            iconify();
        }
    }
#undef IS_STATE
}


//! @brief
void
Frame::handlePropertyChange(XPropertyEvent *ev, Client *client)
{
    EwmhAtoms *ewmh = EwmhAtoms::instance(); // convenience

    if (ev->atom == ewmh->getAtom(NET_WM_DESKTOP)) {
        if (client == _client) {
            long workspace;

            if (AtomUtil::getLong(client->getWindow(),
                                  ewmh->getAtom(NET_WM_DESKTOP), workspace)) {
                if (workspace != signed(_workspace))
                    setWorkspace(workspace);
            }
        }
    } else if (ev->atom == ewmh->getAtom(NET_WM_STRUT)) {
        client->getStrutHint();
    } else if (ev->atom == ewmh->getAtom(NET_WM_NAME)
               || ev->atom == XA_WM_NAME) {
        handleTitleChange(client);
    } else if (ev->atom == XA_WM_NORMAL_HINTS) {
        client->getWMNormalHints();
    } else if (ev->atom == XA_WM_TRANSIENT_FOR) {
        client->getTransientForHint();
    }
}

/**
 * Handle title change, find decoration rules based on changed title
 * and update if changed.
 */
void
Frame::handleTitleChange(Client *client)
{
    // Update title
    client->getXClientName();

    bool require_render = true;
    if (client == _client) {
        string new_decor_name(getClientDecorName(client));
        if (new_decor_name != _decor_name) {
            require_render = ! setDecor(new_decor_name);
        }
    }

    // Render title if decoration was not updated
    if (require_render) {
        renderTitle();
    }
}
