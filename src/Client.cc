//
// Client.cc for pekwm
// Copyright © 2002-2009 Claes Nästén <me{@}pekdon{.}net>
//
// client.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cstdio>
#include <iostream>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#ifdef HAVE_SHAPE
#include <X11/extensions/shape.h>
#endif // HAVE_SHAPE
}

#include "PWinObj.hh"
#include "PDecor.hh" // PDecor::TitleItem
#include "Client.hh"
#include "PScreen.hh"
#include "Config.hh"
#include "KeyGrabber.hh"
#include "Theme.hh"
#include "AutoProperties.hh"
#include "Frame.hh"
#include "Workspaces.hh"
#include "WindowManager.hh"
#include "PTexturePlain.hh"
#include "PImageIcon.hh"
#include "TextureHandler.hh"

#ifdef MENUS
#include "PMenu.hh"
#include "WORefMenu.hh"
#endif // MENUS

using std::cerr;
using std::endl;
using std::find;
using std::list;
using std::string;
using std::vector;
using std::wstring;

list<Client*> Client::_client_list = list<Client*>();
vector<uint> Client::_clientid_list = vector<uint>();

Client::Client(Window new_client, bool is_new)
    : PWinObj(WindowManager::instance()->getScreen()->getDpy()),
      _id(0), _size(0),
      _transient(None), _strut(0), _icon(0),
      _pid(0), _is_remote(false), _class_hint(0),
      _alive(false), _marked(false),
      _send_focus_message(false), _send_close_message(false),
      _wm_hints_input(true), _cfg_request_lock(false),
      _shaped(false), _extended_net_name(false)
{
    // PWinObj attributes, required by validate etc.
    _window = new_client;
    _type = WO_CLIENT;

    // Construct the client
    PScreen::instance()->grabServer();
    if (! validate() || ! getAndUpdateWindowAttributes()) {
        PScreen::instance()->ungrabServer(true);
        return;
    }

    // Get unique Client id
    _id = findClientID();
    _title.setId(_id);
    _title.infoAdd(PDecor::TitleItem::INFO_ID);

    if (PScreen::instance()->hasExtensionShape()) {
#ifdef HAVE_SHAPE
        XShapeSelectInput(_dpy, _window, ShapeNotifyMask);
#endif // HAVE_SHAPE
    }

    XAddToSaveSet(_dpy, _window);
    XSetWindowBorderWidth(_dpy, _window, 0);

    // Load the Class hint before loading the autoprops and
    // getting client title as we search for TitleRule in the autoprops
    _class_hint = new ClassHint();
    readClassRoleHints();

    getWMNormalHints();
    readName();
    readIconName();

    // cyclic dependency, getting the name requires quiering autoprops
    _class_hint->title = _title.getReal();

    // Get Autoproperties before EWMH as we need the cfg_deny
    // property, however the _transient hint needs to be setup to
    // avoid auto-grouping to be to greedy.
    getTransientForHint();
    AutoProperty *ap = readAutoprops(WindowManager::instance()->isStartup()
                                     ? APPLY_ON_NEW : APPLY_ON_START);

    readHints();
    findOrCreateFrame(ap);
    setInitialState();

    // Grab keybindings and mousebutton actions
    KeyGrabber::instance()->grabKeys(_window);
    grabButtons();

    // Tell the world about our state
    updateEwmhStates();

    PScreen::instance()->ungrabServer(true);

    setMappedStateAndFocus(is_new, ap);

    _alive = true;

    // Finished creating the client, so now adding it to the client list.
    woListAdd(this);
    _wo_map[_window] = this;
    _client_list.push_back(this);
}

//! @brief Client destructor
Client::~Client(void)
{
    // Remove from lists
    _wo_map.erase(_window);
    woListRemove(this);
    _client_list.remove(this);

    returnClientID(_id);

    PScreen::instance()->grabServer();

    // removes gravity and moves it back to root if we are alive
    bool focus = false;
    if (_parent && (_parent->getType() == PWinObj::WO_FRAME)) {
        focus = _parent->isFocused();
        static_cast<PDecor*>(_parent)->removeChild(this);
    }

    // Focus the parent if we had focus before
    if (focus && (_transient != None)) {
        Client *trans_cli = findClientFromWindow(_transient);
        if (trans_cli &&
                (((Frame*) trans_cli->getParent())->getActiveChild() == trans_cli)) {
            trans_cli->getParent()->giveInputFocus();
        }
    }

    // Clean up if the client still is alive, it'll be dead all times
    // except when we exit pekwm
    if (_alive) {
        XUngrabButton(_dpy, AnyButton, AnyModifier, _window);
        KeyGrabber::instance()->ungrabKeys(_window);
        XRemoveFromSaveSet(_dpy, _window);
        PWinObj::mapWindow();
    }

    // free names and size hint
    if (_size) {
        XFree(_size);
    }

    removeStrutHint();

    if (_class_hint) {
        delete _class_hint;
    }

    if (_icon) {
        TextureHandler::instance()->returnTexture(_icon);
        _icon = 0;
    }

    PScreen::instance()->ungrabServer(true);
}

/**
 * Read basic window attributes including geometry and update the
 * window attributes being listened to. Returns false if the client
 * disappears during the check.
 */
bool
Client::getAndUpdateWindowAttributes(void)
{
    XWindowAttributes attr;
    if (! XGetWindowAttributes(_dpy, _window, &attr)) {
        return false;
    }
    _gm.x = attr.x;
    _gm.y = attr.y;
    _gm.width = attr.width;
    _gm.height = attr.height;

    _cmap = attr.colormap;
    _size = XAllocSizeHints();

    XSetWindowAttributes sattr;
    sattr.event_mask =
        PropertyChangeMask|StructureNotifyMask|FocusChangeMask;
    sattr.do_not_propagate_mask =
        ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;

    // We don't want these masks to be propagated down to the frame
    XChangeWindowAttributes(_dpy, _window, CWEventMask|CWDontPropagate, &sattr);

    return true;
}

/**
 * Find frame for client based on tagging, hints and
 * autoproperties. Create a new one if not found and add the client.
 */
void
Client::findOrCreateFrame(AutoProperty *autoproperty)
{
    if (! _parent) {
        findTaggedFrame();
    }
    if (! _parent) {
        findPreviousFrame();
    }

    // Apply Autoproperties again to override EWMH state. It's done twice as
    // we need the cfg_deny property when reading the EWMH state.
    if (autoproperty != 0) {
        applyAutoprops(autoproperty);
        if (! _parent) {
            findAutoGroupFrame(autoproperty);
        }
    }

    // if we don't have a frame already, create a new one
    if (! _parent) {
        _parent = new Frame(this, autoproperty);
    }
}

/**
 * Find from client from for the currently tagged client.
 */
bool
Client::findTaggedFrame(void)
{
    if (! WindowManager::instance()->isStartup()) {
        return false;
    }

    // Check for tagged frame
    Frame *frame = Frame::getTagFrame();
    if (frame && frame->isMapped()) {
        _parent = frame;
        frame->addChild(this);

        if (! Frame::getTagBehind()) {
            frame->activateChild(this);
        }
    }

    return _parent != 0;
}

/**
 * Find frame for client on PEKWM_FRAME_ID hint.
 */
bool
Client::findPreviousFrame(void)
{
    if (WindowManager::instance()->isStartup()) {
        return false;
    }

    long id;
    if (AtomUtil::getLong(_window, Atoms::getAtom(PEKWM_FRAME_ID), id)) {
        _parent = Frame::findFrameFromID(id);
        if (_parent) {
            Frame *frame = static_cast<Frame*>(_parent);
            frame->addChildOrdered(this);
            if (getPekwmFrameActive()) {
                frame->activateChild(this);
            }
        }
    }

    return _parent != 0;
}

/**
 * Find frame for client based on autoproperties.
 */
bool
Client::findAutoGroupFrame(AutoProperty *autoproperty)
{
    if (autoproperty->group_size < 0) {
        return false;
    }

    Frame* frame = WindowManager::instance()->findGroup(autoproperty);
    if (frame) {
        frame->addChild(this);

        if (! autoproperty->group_behind) {
            frame->activateChild(this);
        }
        if (autoproperty->group_raise) {
            frame->raise();
        }
    }

    return _parent != 0;
}

/**
 * Get the client state and set iconified and mapped flags.
 */
void
Client::setInitialState(void)
{
    // Set state either specified in hint
    ulong initial_state = readWmHints();
    if (getWmState() == IconicState) {
        _iconified = true;
    }

    if (_iconified || initial_state == IconicState) {
        _iconified = true;
        _mapped = true;
        unmapWindow();
    } else {
        setWmState(initial_state);
    }
}

/**
 * Ensure the Client is (un) mapped and give input focus if requested.
 */
void
Client::setMappedStateAndFocus(bool is_new, AutoProperty *autoproperty)
{
    // Make sure the window is mapped, this is done after it has been
    // added to the decor/frame as otherwise IsViewable state won't
    // be correct and we don't know whether or not to place the window
    if (! _iconified) {
        PWinObj::mapWindow();
    }

    // Let us hear what autoproperties has to say about focusing
    bool do_focus = is_new ? Config::instance()->isFocusNew() : false;
    if (is_new && autoproperty && autoproperty->isMask(AP_FOCUS_NEW)) {
        do_focus = autoproperty->focus_new;
    }

    // Only can give input focus to mapped windows
    if (_parent->isMapped()) {
        // Ordinary focus
        if (do_focus) {
            _parent->giveInputFocus();

            // Check if we are transient, and if we want to focus
        } else if ((_transient != None)
                   && Config::instance()->isFocusNewChild()) {
            Client *trans_for = findClientFromWindow(_transient);
            if (trans_for && trans_for->isFocused()) {
                _parent->giveInputFocus();
            }
        }
    }
}

// START - PWinObj interface.

//! @brief Maps the window.
void
Client::mapWindow(void)
{
    if (_mapped) {
        return;
    }

    if (_iconified) {
        _iconified = false;
        setWmState(NormalState);
        updateEwmhStates();
    }

    if(! _transient) {
        // Unmap our transient windows if we have any
        WindowManager::instance()->findTransientsToMapOrUnmap(_window, false);
    }

    XSelectInput(_dpy, _window, NoEventMask);
    PWinObj::mapWindow();
    XSelectInput(_dpy, _window,
                 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}


//! @brief Sets the client to WithdrawnState and unmaps it.
void
Client::unmapWindow(void)
{
    if (! _mapped) {
        return;
    }

    if (_iconified) {
        // Set the state of the window
        setWmState(IconicState);
        updateEwmhStates();
    }

    XSelectInput(_dpy, _window, NoEventMask);
    PWinObj::unmapWindow();
    XSelectInput(_dpy, _window, PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}

//! @brief Iconifies the client and adds it to the iconmenu
void
Client::iconify(void)
{
    if (_iconified) {
        return;
    }

    _iconified = true;
    if (! _transient) {
        WindowManager::instance()->findTransientsToMapOrUnmap(_window, true);
    }

    unmapWindow();
}

//! @brief Toggle client sticky state
void
Client::stick(void)
{
    PWinObj::stick();

    updateEwmhStates();
}

//! @brief Update the position variables.
void
Client::move(int x, int y)
{
    bool request = ((_gm.x != x) || (_gm.y != y));

    _gm.x = x;
    _gm.y = y;

    if (request) {
        configureRequestSend();
    }
}

//! @brief Resizes the client window to specified size.
void
Client::resize(uint width, uint height)
{
    bool request = ((_gm.width != width) || (_gm.height != height));

    PWinObj::resize(width, height);

    if (request) {
        configureRequestSend();
    }
}

//! @brief Move and resizes the client window to specified size.
void
Client::moveResize(int x, int y, uint width, uint height)
{
    bool request = ((_gm.x != x) || (_gm.y != y) || (_gm.width != width) || (_gm.height != height));

    _gm.x = x;
    _gm.y = y;

    PWinObj::resize(width, height);

    if (request) {
        configureRequestSend();
    }
}

//! @brief Sets the workspce and updates the _NET_WM_DESKTOP hint.
void
Client::setWorkspace(uint workspace)
{
    if (workspace != NET_WM_STICKY_WINDOW) {
        if (workspace >= Workspaces::instance()->size()) {
            workspace = Workspaces::instance()->size() - 1;
        }
        _workspace = workspace;

        if (_sticky) {
            AtomUtil::setLong(_window, Atoms::getAtom(NET_WM_DESKTOP), NET_WM_STICKY_WINDOW);
        } else {
            AtomUtil::setLong(_window, Atoms::getAtom(NET_WM_DESKTOP), _workspace);
        }
    }
}

//! @brief Gives the Client input focus.
void
Client::giveInputFocus(void)
{
    if (_wm_hints_input) {
        PWinObj::giveInputFocus();
    }

    sendTakeFocusMessage();
}

//! @brief Reparents and sets _parent member, filtering unmap events
void
Client::reparent(PWinObj *parent, int x, int y)
{
    XSelectInput(_dpy, _window, NoEventMask);
    PWinObj::reparent(parent, x, y);
    _gm.x = parent->getX() + x;
    _gm.y = parent->getY() + y;
    XSelectInput(_dpy, _window,
                 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}

//! @brief
ActionEvent*
Client::handleUnmapEvent(XUnmapEvent *ev)
{
    if ((ev->window != ev->event) && (ev->send_event != true)) {
        return 0;
    }

    // ICCCM 4.1.4 advices the window manager to trigger the transition to
    // Withdrawn state on real and synthetic UnmapNotify events.
    setWmState(WithdrawnState);

    // Extended Window Manager Hints 1.3 specifies that a window manager
    // should remove the _NET_WM_STATE property when a window is withdrawn.
    AtomUtil::unsetProperty(_window, Atoms::getAtom(STATE));

#ifdef DEBUG
    cerr << __FILE__ << "@" << __LINE__ << ": "
         << "Client(" << this << ")::handleUnmapEvent(" << ev << ")" << endl
         << " *** unmapping client, window: " << _window << endl;
#endif // DEBUG

    // FIXME: Listen mask should change as this doesn't work?
    _alive = false;
    delete this;

    return 0;
}

//!@brief
ActionEvent*
Client::handleMapRequest(XMapRequestEvent *ev)
{
       if (_parent && dynamic_cast<PDecor *>(_parent)) {
           dynamic_cast<PDecor*>(_parent)->deiconify();
       }
       return 0;
}

// END - PWinObj interface.

//! @brief Finds the Client which holds the Window w.
//! @param win Window to search for.
//! @return Pointer to the client if found, else 0
Client*
Client::findClient(Window win)
{
    // Validate input window.
    if ((win == None) || (win == PScreen::instance()->getRoot())) {
        return 0;
    }

    list<Client*>::iterator it(_client_list.begin());
    for (; it != _client_list.end(); ++it) {
        if (win == (*it)->getWindow()
            || ((*it)->getParent() && (*((*it)->getParent()) == win))) {
            return (*it);
        }
    }

    return 0;
}

//! @brief Finds the Client of Window win.
//! @param win Window to search for.
Client*
Client::findClientFromWindow(Window win)
{
    // Validate input window.
    if (! win || win == PScreen::instance()->getRoot()) {
        return 0;
    }

    list<Client*>::iterator it(_client_list.begin());
    for(; it != _client_list.end(); ++it) {
        if (win == (*it)->getWindow()) {
            return (*it);
        }
    }

    return 0;
}

//! @brief Finds Client with equal ClassHint.
//! @param class_hint ClassHint to search for.
//! @return Client if found, else 0.
Client*
Client::findClientFromHint(const ClassHint *class_hint)
{
    list<Client*>::iterator it(_client_list.begin());
    for (; it != _client_list.end(); ++it) {
        if (*class_hint == *(*it)->getClassHint()) {
            return *it;
        }
    }

    return 0;
}

//! @brief Finds Client with id.
//! @param id ID to search for.
//! @return Client if found, else 0.
Client*
Client::findClientFromID(uint id)
{
    list<Client*>::iterator it(_client_list.begin());
    for (; it != _client_list.end(); ++it) {
        if ((*it)->getClientID() == id) {
            return *it;
        }
    }

    return 0;
}

//! @brief Checks if the window has any Destroy or Unmap notifys.
bool
Client::validate(void)
{
    XSync(_dpy, false);

    XEvent ev;
    if (XCheckTypedWindowEvent(_dpy, _window, DestroyNotify, &ev)
        || XCheckTypedWindowEvent(_dpy, _window, UnmapNotify, &ev)) {
        XPutBackEvent(_dpy, &ev);
        return false;
    }

    return true;
}

//! @brief Checks if the window has attribute IsViewable set
bool
Client::isViewable(void)
{
    XWindowAttributes attr;
    XGetWindowAttributes(_dpy, _window, &attr);

    return (attr.map_state == IsViewable);
}

//! @brief Grabs all the mouse button actions on the client.
void
Client::grabButtons(void)
{
    // Make sure we don't have any buttons grabbed.
    XUngrabButton(_dpy, AnyButton, AnyModifier, _window);

    list<ActionEvent> *actions = Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_CHILD_FRAME);
    list<ActionEvent>::iterator it(actions->begin());
    for (; it != actions->end(); ++it) {
        if ((it->type == MOUSE_EVENT_PRESS) || (it->type == MOUSE_EVENT_RELEASE)) {
            // No need to grab mod less events, replied with the frame
            if ((it->mod == 0) || (it->mod == MOD_ANY)) {
                continue;
            }

            grabButton(it->sym, it->mod,
                       ButtonPressMask|ButtonReleaseMask,
                       _window, None);
        } else if (it->type == MOUSE_EVENT_MOTION) {
            // FIXME: Add support for MOD_ANY
            grabButton(it->sym, it->mod,
                       ButtonPressMask|ButtonReleaseMask|ButtonMotionMask,
                       _window, None);
        }
    }
}

//! @brief Sets CfgDeny state on Client
void
Client::setStateCfgDeny(StateAction sa, uint deny)
{
    if (! ActionUtil::needToggle(sa, _state.cfg_deny&deny)) {
        return;
    }

    if (_state.cfg_deny&deny) {
        _state.cfg_deny &= ~deny;
    } else {
        _state.cfg_deny |= deny;
    }
}

/**
 * Read "all" hints required during creation of Client.
 */
void
Client::readHints(void)
{
    readMwmHints(); // read atoms
    readEwmhHints();
    readPekwmHints();
    readIcon();
    readClientPid();
    readClientRemote();
    getWMProtocols();
}

/**
 * Read WM Hints, return the initial state of the window.
 */
ulong
Client::readWmHints(void)
{
    ulong initial_state = NormalState;
    XWMHints* hints = XGetWMHints(_dpy, _window);
    if (hints) {
        // get the input focus mode
        if (hints->flags&InputHint) { // FIXME: More logic needed
            _wm_hints_input = hints->input;
        }

        // Get initial state of the window
        if (hints->flags&StateHint) {
            initial_state = hints->initial_state;
        }

        XFree(hints);
    }
    return initial_state;
}

/**
 * Read the WM_CLASS hint and set information on the _class_hint used
 * in matching auto properties.
 */
void
Client::readClassRoleHints(void)
{
    // class hint
    XClassHint class_hint;
    if (XGetClassHint(_dpy, _window, &class_hint)) {
        _class_hint->h_name = Util::to_wide_str(class_hint.res_name);
        _class_hint->h_class = Util::to_wide_str(class_hint.res_class);
        XFree(class_hint.res_name);
        XFree(class_hint.res_class);
    }

    // wm window role
    string role;
    AtomUtil::getString(_window, Atoms::getAtom(WM_WINDOW_ROLE), role);

    _class_hint->h_role = Util::to_wide_str(role);
}

//! @brief Loads the Clients state from EWMH atoms.
void
Client::readEwmhHints(void)
{
    // which workspace do we belong to?
    long workspace = -1;
    AtomUtil::getLong(_window, Atoms::getAtom(NET_WM_DESKTOP), workspace);
    if (workspace < 0) {
        _workspace = Workspaces::instance()->getActive();
        AtomUtil::setLong(_window, Atoms::getAtom(NET_WM_DESKTOP), _workspace);
    } else {
        _workspace = workspace;
    }

    // try to figure out what kind of window we are and alter it accordingly
    int items;
    Atom *atoms = 0;

    AtomName window_type = WINDOW_TYPE;
    atoms = (Atom*) AtomUtil::getEwmhPropData(_window, Atoms::getAtom(WINDOW_TYPE), XA_ATOM, items);
    if (atoms) {
      for (int i = 0; window_type == WINDOW_TYPE && i < items; ++i) {
        if (atoms[i] == Atoms::getAtom(WINDOW_TYPE_DESKTOP)) {
          window_type = WINDOW_TYPE_DESKTOP;
        } else if (atoms[i] == Atoms::getAtom(WINDOW_TYPE_DOCK)) {
          window_type = WINDOW_TYPE_DOCK;
        } else if (atoms[i] == Atoms::getAtom(WINDOW_TYPE_TOOLBAR)) {
          window_type = WINDOW_TYPE_TOOLBAR;
        } else if (atoms[i] == Atoms::getAtom(WINDOW_TYPE_MENU)) {
          window_type = WINDOW_TYPE_MENU;
        } else if (atoms[i] == Atoms::getAtom(WINDOW_TYPE_UTILITY)) {
          window_type = WINDOW_TYPE_UTILITY;
        } else if (atoms[i] == Atoms::getAtom(WINDOW_TYPE_DIALOG)) {
          window_type = WINDOW_TYPE_DIALOG;
        } else if (atoms[i] == Atoms::getAtom(WINDOW_TYPE_SPLASH)) {
          window_type = WINDOW_TYPE_SPLASH;
        }
      }

      XFree(atoms);
    }

    // Set window type to WINDOW_TYPE_NORMAL if it did not match
    if (window_type == WINDOW_TYPE) {
        window_type = WINDOW_TYPE_NORMAL;
        AtomUtil::setAtom(_window, Atoms::getAtom(WINDOW_TYPE), Atoms::getAtom(WINDOW_TYPE_NORMAL));
    }

    // Apply autoproperties for window type
    AutoProperty *auto_property = AutoProperties::instance()->findWindowTypeProperty(window_type);
    if (auto_property) {
        applyAutoprops(auto_property);
    }

    // The _NET_WM_STATE overrides the _NET_WM_TYPE
    NetWMStates win_states;
    if (getEwmhStates(win_states)) {
        if (win_states.hidden) _iconified = true;
        if (win_states.shaded) _state.shaded = true;
        if (win_states.max_vert) _state.maximized_vert = true;
        if (win_states.max_horz) _state.maximized_horz = true;
        if (win_states.skip_taskbar) _state.skip |= SKIP_TASKBAR;
        if (win_states.skip_pager) _state.skip |= SKIP_PAGER;
        if (win_states.sticky) _sticky = true;
        if (win_states.above) _layer = LAYER_ABOVE_DOCK;
        if (win_states.below) _layer = LAYER_BELOW;
        if (win_states.fullscreen) _state.fullscreen = true;
    }

    // check if we have a strut
    getStrutHint();
}

//! @brief Loads the Clients state from MWM atoms.
void
Client::readMwmHints(void)
{
    MwmHints *mwm_hints = getMwmHints(_window);

    if (mwm_hints) {
        if (mwm_hints->flags&MWM_HINTS_FUNCTIONS) {
            bool state = ! (mwm_hints->functions&MWM_FUNC_ALL);

            _actions.resize   = (mwm_hints->functions&MWM_FUNC_RESIZE)  ? state : ! state;
            _actions.move     = (mwm_hints->functions&MWM_FUNC_MOVE)    ? state : ! state;
            _actions.minimize = (mwm_hints->functions&MWM_FUNC_ICONIFY) ? state : ! state;
            _actions.close    = (mwm_hints->functions&MWM_FUNC_CLOSE)   ? state : ! state;
            _actions.maximize_vert = (mwm_hints->functions&MWM_FUNC_MAXIMIZE) ? state : ! state;
            _actions.maximize_horz = (mwm_hints->functions&MWM_FUNC_MAXIMIZE) ? state : ! state;
        }

        // Check decoration flags
        if (mwm_hints->flags & MWM_HINTS_DECORATIONS
            && ! (mwm_hints->decorations & MWM_DECOR_ALL)) {
          if (! (mwm_hints->decorations & MWM_DECOR_TITLE)) {
            setTitlebar(false);
          }
          if (! (mwm_hints->decorations & MWM_DECOR_BORDER)) {
            setBorder(false);
          }

          // Do not handle HANDLE, MENU, ICONFIY or MAXIMIZE. Maybe
          // one should set the allowed actions for the client based
          // on this but that might be annoying so ignoring these.
        }

        XFree(mwm_hints);
    }
}

//! @brief Reads non-standard pekwm hints
void
Client::readPekwmHints(void)
{
    long value;
    string str;

    // Get decor state
    if (AtomUtil::getLong(_window, Atoms::getAtom(PEKWM_FRAME_DECOR), value)) {
        _state.decor = value;
    }
    // Get skip state
    if (AtomUtil::getLong(_window, Atoms::getAtom(PEKWM_FRAME_SKIP), value)) {
        _state.skip = value;
    }

    // Get custom title
    if (AtomUtil::getString(_window, Atoms::getAtom(PEKWM_TITLE), str)) {
        _title.setUser(Util::to_wide_str(str));
    }

    _state.initial_frame_order = getPekwmFrameOrder();
}

//! @brief Read _NET_WM_ICON from client window.
void
Client::readIcon(void)
{
    PImageIcon *image = new PImageIcon(_dpy);
    if (image->loadFromWindow(_window)) {
        if (! _icon) {
            _icon = new PTextureImage(_dpy);
            TextureHandler::instance()->referenceTexture(_icon);
        }

        _icon->setImage(image);
    } else {
        if (image) {
            delete image;
        }

        if (_icon) {
            TextureHandler::instance()->returnTexture(_icon);
            _icon = 0;
        }
    }
}

//! @brief Tries to find a AutoProp for the current client.
//! @param type Defaults to 0
//! @return AutoProperty if any is found, else 0.
AutoProperty*
Client::readAutoprops(uint type)
{
    AutoProperty *data =
        AutoProperties::instance()->findAutoProperty(_class_hint, _workspace, type);

    if (data) {
        // Make sure transient state matches
        if (_transient
                ? data->isApplyOn(APPLY_ON_TRANSIENT|APPLY_ON_TRANSIENT_ONLY)
                : ! data->isApplyOn(APPLY_ON_TRANSIENT_ONLY)) {
            applyAutoprops(data);
        } else {
            data = 0;
        }
    }

    return data;
}

//! @brief Applies AutoPropery to this Client.
void
Client::applyAutoprops(AutoProperty *ap)
{
    // Set the correct group of the window
    _class_hint->group = ap->group_name;

    // We only apply grouping if it's a new client or if we are restarting
    // and have APPLY_ON_START set
    if (ap->isMask(AP_STICKY))
        _sticky = ap->sticky;
    if (ap->isMask(AP_SHADED))
        _state.shaded = ap->shaded;
    if (ap->isMask(AP_MAXIMIZED_VERTICAL))
        _state.maximized_vert = ap->maximized_vertical;
    if (ap->isMask(AP_MAXIMIZED_HORIZONTAL))
        _state.maximized_horz = ap->maximized_horizontal;
    if (ap->isMask(AP_FULLSCREEN))
        _state.fullscreen = ap->fullscreen;
    if (ap->isMask(AP_ICONIFIED))
        _iconified = ap->iconified;
    if (ap->isMask(AP_TITLEBAR))
        setTitlebar(ap->titlebar);
    if (ap->isMask(AP_BORDER))
        setBorder(ap->border);
    if (ap->isMask(AP_LAYER) && (ap->layer <= LAYER_MENU))
        _layer = ap->layer;
    if (ap->isMask(AP_SKIP))
        _state.skip = ap->skip;
    if (ap->isMask(AP_FOCUSABLE))
        _focusable = ap->focusable;
    if (ap->isMask(AP_WORKSPACE)) {
        _workspace = ap->workspace;
    }
    if (ap->isMask(AP_CFG_DENY)) {
        _state.cfg_deny = ap->cfg_deny;
    }
}

/**
 * Read _NET_WM_PID.
 */
void
Client::readClientPid(void)
{
    AtomUtil::getLong(_window, Atoms::getAtom(NET_WM_PID), _pid);
}

/**
 * Read WM_CLIENT_MACHINE and check against local hostname and set
 * _is_remote if it does not match.
 */
void
Client::readClientRemote(void)
{
    string client_machine;
    if (AtomUtil::getTextProperty(_window, XA_WM_CLIENT_MACHINE, client_machine)) {
        _is_remote = Util::getHostname() != client_machine;
    }
}

//! @brief Finds free Client ID.
//! @return First free Client ID.
uint
Client::findClientID(void)
{
    uint id = 0;    

    if (_clientid_list.size()) {
        // Check for used Frame IDs
        id = _clientid_list.back();
        _clientid_list.pop_back();
    } else {
        // No free, get next number (Client is not in list when this is called.)
        id = _client_list.size() + 1;
    }

    return id;
}

//! @brief Returns Client ID to used client id list.
//! @param id ID to return.
void
Client::returnClientID(uint id)
{
    vector<uint>::iterator it(_clientid_list.begin());
    for (; it != _clientid_list.end() && id < *it; ++it)
        ;
    _clientid_list.insert(it, id);
}

/**
 * Tries to get the NET_WM name, else fall back to WM_NAME
 */
void
Client::readName(void)
{
    // Read title, bail out if it fails.
    wstring title;
    if (! AtomUtil::getUtf8String(_window, Atoms::getAtom(NET_WM_NAME), title)) {
        string mb_title;
        if (! AtomUtil::getTextProperty(_window, XA_WM_NAME, mb_title)) {
            return;
        }
        title = Util::to_wide_str(mb_title);
    }

    // Mirror it on the visible
    _title.setCustom(L"");
    _title.setCount(titleFindID(title));
    _title.setReal(title);

    // Apply title rules and find unique name, doesn't apply on
    // user-set titles
    if (titleApplyRule(title)) {
        _title.setCustom(title);
        AtomUtil::setUtf8String(_window, Atoms::getAtom(NET_WM_VISIBLE_NAME), title);
    } else {
        AtomUtil::unsetProperty(_window, Atoms::getAtom(NET_WM_VISIBLE_NAME));
    }
}

//! @brief Searches for an TitleRule and if found, applies it
//! @param title Title to apply rule on.
//! @return true if rule was applied, else false.
bool
Client::titleApplyRule(std::wstring &title)
{
    _class_hint->title = title;
    TitleProperty *data = AutoProperties::instance()->findTitleProperty(_class_hint);

    if (data) {
        return data->getTitleRule().ed_s(title);
    } else {
        return false;
    }
}

//! @brief Searches for a unique ID within Clients having the same title
//! @param title Title of client to find ID for.
//! @return Number of clients with that id.
uint
Client::titleFindID(std::wstring &title)
{
    // Do not search for unique IDs if it is not enabled.
    if (! Config::instance()->getClientUniqueName()) {
        return 0;
    }

    uint id_found = 0;
    list<uint> ids_used;

    list<Client*>::iterator it = _client_list.begin();
    for (; it != _client_list.end(); ++it) {
        if (*it != this) {
            if ((*it)->getTitle()->getReal() == title) {
                ids_used.push_back((*it)->getTitle()->getCount());
            }
        }
    }

    // more than one client having this name
    if (ids_used.size() > 0) {
        ids_used.sort();

        list<uint>::iterator ui_it( ids_used.begin());
        for (uint i = 0; ui_it != ids_used.end(); ++i, ++ui_it) {
            if (i < *ui_it) {
                id_found = i;
                break;
            }
        }

        if (ui_it == ids_used.end()) {
            id_found = ids_used.size();
        }
    }

    return id_found;
}

/**
 * Get the clients icon name to be displayed when the client is
 * iconified.
 */
void
Client::readIconName(void)
{
    wstring icon_name;

    if (! AtomUtil::getUtf8String(_window, Atoms::getAtom(NET_WM_ICON_NAME), icon_name)) {
        string mb_icon_name;
        if (AtomUtil::getTextProperty(_window, XA_WM_ICON_NAME, mb_icon_name)) {
            icon_name = Util::to_wide_str(mb_icon_name);
        }
    }

    // Set real name
    _icon_name.setReal(icon_name);
    _icon_name.setCustom(icon_name);

    if (_icon_name.getVisible() == _icon_name.getReal()) {
        AtomUtil::unsetProperty(_window, Atoms::getAtom(NET_WM_VISIBLE_ICON_NAME));
    } else {
        AtomUtil::setUtf8String(_window, Atoms::getAtom(NET_WM_VISIBLE_ICON_NAME), icon_name);
    }
}

//! @brief Sets the WM_STATE of the client to state
//! @param state State to set.
void
Client::setWmState(ulong state)
{
    ulong data[2];

    data[0] = state;
    data[1] = None; // No Icon

    XChangeProperty(_dpy, _window,
                    Atoms::getAtom(WM_STATE),
                    Atoms::getAtom(WM_STATE),
                    32, PropModeReplace, (uchar*) data, 2);
}

// If we can't find a wm_state we're going to have to assume
// Withdrawn. This is not exactly optimal, since we can't really
// distinguish between the case where no WM has run yet and when the
// state was explicitly removed (Clients are allowed to either set the
// atom to Withdrawn or just remove it... yuck.)
long
Client::getWmState(void)
{
    Atom real_type;
    int real_format;
    long *data, state = WithdrawnState;
    ulong items_read, items_left;
    uchar *udata;

    int status =
        XGetWindowProperty(_dpy, _window, Atoms::getAtom(WM_STATE),
                           0L, 2L, False, Atoms::getAtom(WM_STATE),
                           &real_type, &real_format, &items_read, &items_left,
                           &udata);
    if ((status  == Success) && items_read) {
        data = reinterpret_cast<long*>(udata);
        state = *data;
        XFree(udata);
    }

    return state;
}

//! @brief Send XConfigureEvent, letting the client know about changes
void
Client::configureRequestSend(void)
{
    if (_cfg_request_lock) {
      return;
    }

    XConfigureEvent e;

    e.type = ConfigureNotify;
    e.event = _window;
    e.window = _window;
    e.x = _gm.x;
    e.y = _gm.y;
    e.width = _gm.width;
    e.height = _gm.height;
    e.border_width = 0;
    e.above = None;
    e.override_redirect = False;

    XSendEvent(_dpy, _window, false, StructureNotifyMask, (XEvent *) &e);
}

//! @brief Send a TAKE_FOCUS client message to the client (if requested by it).
void Client::sendTakeFocusMessage(void)
{
    if (_send_focus_message) {
        {
            XEvent ev;
            XChangeProperty(_dpy, RootWindow(_dpy, DefaultScreen(_dpy)), XA_PRIMARY, XA_STRING, 8, PropModeAppend, 0, 0);
            XWindowEvent(_dpy, RootWindow(_dpy, DefaultScreen(_dpy)), PropertyChangeMask, &ev);
            PScreen::instance()->setLastEventTime(ev.xproperty.time);
        }
        sendXMessage(_window,
                     Atoms::getAtom(WM_PROTOCOLS), NoEventMask,
                     Atoms::getAtom(WM_TAKE_FOCUS),
                     PScreen::instance()->getLastEventTime());
    }
}

//! @brief Grabs a button on the window win
//! Grabs the button button, with the mod mod and mask mask on the window win
//! and cursor curs with "all" possible extra modifiers
void
Client::grabButton(int button, int mod, int mask, Window win, Cursor curs)
{
    uint num_lock = PScreen::instance()->getNumLock();
    uint scroll_lock = PScreen::instance()->getScrollLock();

    XGrabButton(_dpy, button, mod,
                win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
    XGrabButton(_dpy, button, mod|LockMask,
                win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);

    if (num_lock) {
        XGrabButton(_dpy, button, mod|num_lock,
                    win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
        XGrabButton(_dpy, button, mod|num_lock|LockMask,
                    win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
    }
    if (scroll_lock) {
        XGrabButton(_dpy, button, mod|scroll_lock,
                    win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
        XGrabButton(_dpy, button, mod|scroll_lock|LockMask,
                    win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
    }
    if (num_lock && scroll_lock) {
        XGrabButton(_dpy, button, mod|num_lock|scroll_lock,
                    win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
        XGrabButton(_dpy, button, mod|num_lock|scroll_lock|LockMask,
                    win, true, mask, GrabModeAsync, GrabModeAsync, None, curs);
    }
}

//! @brief Toggles the clients always on top state
void
Client::alwaysOnTop(bool top)
{
    _layer = top ? LAYER_ONTOP : LAYER_NORMAL;
    updateEwmhStates();
}

//! @brief Toggles the clients always below state.
void
Client::alwaysBelow(bool below)
{
    _layer = below ? LAYER_BELOW : LAYER_NORMAL;
    updateEwmhStates();
}

//! @brief Sets the skip state, and updates the _PEKWM_FRAME_SKIP atom
void
Client::setSkip(uint skip)
{
    _state.skip = skip;
    AtomUtil::setLong(_window, Atoms::getAtom(PEKWM_FRAME_SKIP), _state.skip);
}

/**
 * Set demands attention state, this should be unset when client
 * recieves focus. This is ignored if client has focus.
 */
void
Client::setStateDemandsAttention(StateAction sa, bool attention)
{
    // FIXME: Demands attention state.
}

//! @brief Sends an WM_DELETE message to the client, else kills it.
void
Client::close(void)
{
    if (_send_close_message) {
        sendXMessage(_window, Atoms::getAtom(WM_PROTOCOLS), NoEventMask,
                     Atoms::getAtom(WM_DELETE_WINDOW), CurrentTime);
    } else {
        kill();
    }
}

//! @brief Kills the client using XKillClient
void
Client::kill(void)
{
    XKillClient(_dpy, _window);
}

//! @brief Sets the position based on P or U position.
//! @return Returns true on success, else false.
bool
Client::setPUPosition(void)
{
    if ((_size->flags&PPosition) || (_size->flags&USPosition)) {
        if (_gm.x == 0) {
            _gm.x = _size->x;
        }
        if (_gm.y == 0) {
            _gm.y = _size->y;
        }
        return true;
    }
    return false;
}


//! @brief Get the closest size confirming to the aspect ratio and ResizeInc (if applicable)
//! @param r_w Pointer to put the new width in
//! @param r_h Pointer to put the new height in
//! @param w Width to calculate from
//! @param h Height to calculate from
//! @return true if width/height have changed
bool
Client::getAspectSize(uint *r_w, uint *r_h, uint w, uint h)
{
    // see ICCCM 4.1.2.3 for PAspect and {min,max}_aspect
    if (_size->flags & PAspect && Config::instance()->isHonourAspectRatio()) {
        // shorthand
        const uint amin_x = _size->min_aspect.x;
        const uint amin_y = _size->min_aspect.y;
        const uint amax_x = _size->max_aspect.x;
        const uint amax_y = _size->max_aspect.y;

        uint base_w = 0, base_h = 0;

        // If PBaseSize is specified, base_{width,height} should be subtracted
        // before checking the aspect ratio (c.f. ICCCM). The additional checks avoid
        // underflows in w and h. Keep in mind that _size->base_{width,height} are
        // guaranteed to be non-negative by getWMNormalHints().
        if (_size->flags & PBaseSize) {
            if (static_cast<uint>(_size->base_width) < w) {
                base_w = _size->base_width;
                w -= base_w;
            }
            if (static_cast<uint>(_size->base_height) < h) {
                base_h = _size->base_height;
                h -= base_h;
            }
        }

        double tmp;

        // We have to ensure: min_aspect.x/min_aspect.y <= w/h <= max_aspect.x/max_aspect.y

        // How do we calculate the new r_w, r_h?
        // Consider the plane spanned by width and height. The points with one specific
        // aspect ratio form a line and (w,h) is a point. So, we simply calculate the
        // point on the line that is closest to (w, h) under the Euclidean metric.

        // Lesson learned doing this: It is good to look at a different implementation
        // (thanks fluxbox!) and then let a friend go over your own calculation to
        // tell you what your mistake is (thanks Robert!). ;-)

        // Check if w/h is less than amin_x/amin_y
        if (w * amin_y < h * amin_x) {
            tmp = ((double)(w * amin_x + h * amin_y)) /
                  ((double)(amin_x * amin_x + amin_y * amin_y));

            w = static_cast<uint>(amin_x * tmp) + base_w;
            h = static_cast<uint>(amin_y * tmp) + base_h;

        // Check if w/h is greater than amax_x/amax_y
        } else if (w * amax_y > amax_x * h) {
            tmp = ((double)(w * amax_x + h * amax_y)) /
                  ((double)(amax_x * amax_x + amax_y * amax_y));

            w = static_cast<uint>(amax_x * tmp) + base_w;
            h = static_cast<uint>(amax_y * tmp) + base_h;
        }

        getIncSize(r_w, r_h, w, h, false);
        return true;
    }
    return getIncSize(r_w, r_h, w, h);
}

//! @brief Get the size closest to the ResizeInc incrementer
//! @param r_w Pointer to put the new width in
//! @param r_h Pointer to put the new height in
//! @param w Width to calculate from
//! @param h Height to calculate from
//! @param incr If true, increase w,h to fulfil PResizeInc (instead of decreasing them)
bool
Client::getIncSize(uint *r_w, uint *r_h, uint w, uint h, bool incr)
{
    uint basex, basey;

    if (_size->flags&PResizeInc) {
        basex = (_size->flags&PBaseSize)
                ? _size->base_width
                : (_size->flags&PMinSize) ? _size->min_width : 0;

        basey = (_size->flags&PBaseSize)
                ? _size->base_height
                : (_size->flags&PMinSize) ? _size->min_height : 0;

        if (w-basex < 0 || h-basey<0) {
            basex=basey=0;
        }

        uint dw = (w - basex) % _size->width_inc;
        uint dh = (h - basey) % _size->height_inc;

        *r_w = w - dw + ((incr && dw)?_size->width_inc:0);
        *r_h = h - dh + ((incr && dh)?_size->height_inc:0);
        return true;
    }

    *r_w = w;
    *r_h = h;
    return false;
}

//! @brief Gets a MwmHint structure from a window. Doesn't free memory.
Client::MwmHints*
Client::getMwmHints(Window win)
{
    Atom real_type; int real_format;
    ulong items_read, items_left;
    MwmHints *data = 0;
    uchar *udata;

    Atom hints_atom = Atoms::getAtom(MOTIF_WM_HINTS);

    int status = XGetWindowProperty(_dpy, win, hints_atom, 0L, 20L, False, hints_atom,
                                    &real_type, &real_format, &items_read, &items_left, &udata);

    if (status == Success) {
        if (items_read < MWM_HINTS_NUM) {
            XFree(udata);
            udata = 0;
        }
    } else {
        udata = 0;
    }

    if (udata) {
        data = reinterpret_cast<MwmHints*>(udata);
    }

    return data;
}

// This happens when a window is iconified and destroys itself. An
// Unmap event wouldn't happen in that case because the window is
// already unmapped.
void
Client::handleDestroyEvent(XDestroyWindowEvent *e)
{
    _alive = false;
    delete this;
}

void
Client::handleColormapChange(XColormapEvent *e)
{
    if (e->c_new) {
        _cmap = e->colormap;
        XInstallColormap(_dpy, _cmap);
    }
}

int
Client::sendXMessage(Window window, Atom atom, long mask,
                     long v1, long v2, long v3, long v4, long v5)
{
    XEvent e;

    e.type = e.xclient.type = ClientMessage;
    e.xclient.display = _dpy;
    e.xclient.window = window;
    e.xclient.format = 32;
    e.xclient.message_type = atom;

    e.xclient.data.l[0] = v1;
    e.xclient.data.l[1] = v2;
    e.xclient.data.l[2] = v3;
    e.xclient.data.l[3] = v4;
    e.xclient.data.l[4] = v5;

    return XSendEvent(_dpy, window, false, mask, &e);
}

//! @brief
bool
Client::getEwmhStates(NetWMStates &win_states)
{
    int num = 0;
    Atom *states;
    states = (Atom*)
             AtomUtil::getEwmhPropData(_window, Atoms::getAtom(STATE),
                                       XA_ATOM, num);

    if (states) {
        for (int i = 0; i < num; ++i) {
            if (states[i] == Atoms::getAtom(STATE_MODAL)) {
                win_states.modal = true;
            } else if (states[i] == Atoms::getAtom(STATE_STICKY)) {
                win_states.sticky = true;
            } else if (states[i] == Atoms::getAtom(STATE_MAXIMIZED_VERT)
                       && ! isCfgDeny(CFG_DENY_STATE_MAXIMIZED_VERT)) {
                win_states.max_vert = true;
            } else if (states[i] == Atoms::getAtom(STATE_MAXIMIZED_HORZ)
                       && ! isCfgDeny(CFG_DENY_STATE_MAXIMIZED_HORZ)) {
                win_states.max_horz = true;
            } else if (states[i] == Atoms::getAtom(STATE_SHADED)) {
                win_states.shaded = true;
            } else if (states[i] == Atoms::getAtom(STATE_SKIP_TASKBAR)) {
                win_states.skip_taskbar = true;
            } else if (states[i] == Atoms::getAtom(STATE_SKIP_PAGER)) {
                win_states.skip_pager = true;
            } else if (states[i] == Atoms::getAtom(STATE_DEMANDS_ATTENTION)) {
                win_states.demands_attention = true;
            } else if (states[i] == Atoms::getAtom(STATE_HIDDEN)
                       && ! isCfgDeny(CFG_DENY_STATE_HIDDEN)) {
                win_states.hidden = true;
            } else if (states[i] == Atoms::getAtom(STATE_FULLSCREEN)
                       && ! isCfgDeny(CFG_DENY_STATE_FULLSCREEN)) {
                win_states.fullscreen = true;
            } else if (states[i] == Atoms::getAtom(STATE_ABOVE)
                       && ! isCfgDeny(CFG_DENY_STATE_ABOVE)) {
                win_states.above = true;
            } else if (states[i] == Atoms::getAtom(STATE_BELOW)
                       && ! isCfgDeny(CFG_DENY_STATE_BELOW)) {
                win_states.below = true;
            }
        }

        XFree(states);

        return true;
    } else {
        return false;
    }
}

//! @brief Tells the world about our states, such as shaded etc.
void
Client::updateEwmhStates(void)
{
    list<Atom> states;

    if (false) // we don't yet support modal state
        states.push_back(Atoms::getAtom(STATE_MODAL));
    if (_sticky)
        states.push_back(Atoms::getAtom(STATE_STICKY));
    if (_state.maximized_vert)
        states.push_back(Atoms::getAtom(STATE_MAXIMIZED_VERT));
    if (_state.maximized_horz)
        states.push_back(Atoms::getAtom(STATE_MAXIMIZED_HORZ));
    if (_state.shaded)
        states.push_back(Atoms::getAtom(STATE_SHADED));
    if (isSkip(SKIP_TASKBAR))
        states.push_back(Atoms::getAtom(STATE_SKIP_TASKBAR));
    if (isSkip(SKIP_PAGER))
        states.push_back(Atoms::getAtom(STATE_SKIP_PAGER));
    if (_iconified)
        states.push_back(Atoms::getAtom(STATE_HIDDEN));
    if (_state.fullscreen)
        states.push_back(Atoms::getAtom(STATE_FULLSCREEN));
    if (_layer == LAYER_ABOVE_DOCK)
        states.push_back(Atoms::getAtom(STATE_ABOVE));
    if (_layer == LAYER_BELOW)
        states.push_back(Atoms::getAtom(STATE_BELOW));

    Atom *atoms = new Atom[(states.size() > 0) ? states.size() : 1];
    if (states.size() > 0) {
        copy(states.begin(), states.end(), atoms);
    }
    AtomUtil::setAtoms(_window, Atoms::getAtom(STATE), atoms, states.size());
    delete [] atoms;
}

//! @brief
void
Client::getWMNormalHints(void)
{
    long dummy;
    XGetWMNormalHints(_dpy, _window, _size, &dummy);

    // let's do some sanity checking
    if (_size->flags & PBaseSize) {
        if (_size->base_width<0 || _size->base_height<0) {
            _size->base_width = _size->base_height = 0;
            _size->flags &= ~PBaseSize;
        }
    }
    if (_size->flags & PAspect) {
        if (_size->min_aspect.x < 0 || _size->min_aspect.y < 0 ||
            _size->max_aspect.x < 0 || _size->max_aspect.y < 0) {

            _size->min_aspect.x = _size->min_aspect.y = 0;
            _size->max_aspect.x = _size->max_aspect.y = 0;
            _size->flags &= ~PAspect;
        }
    }
    if (_size->flags & PResizeInc) {
        if (_size->width_inc <= 0 || _size->height_inc <= 0) {
            _size->width_inc = 0;
            _size->height_inc = 0;
            _size->flags &= ~PBaseSize;
        }
    }

}

//! @brief
void
Client::getWMProtocols(void)
{
    int count;
    Atom *protocols;

    if (XGetWMProtocols(_dpy, _window, &protocols, &count) != 0) {
        for (int i = 0; i < count; ++i) {
            if (protocols[i] == Atoms::getAtom(WM_TAKE_FOCUS)) {
                _send_focus_message = true;
            } else if (protocols[i] == Atoms::getAtom(WM_DELETE_WINDOW)) {
                _send_close_message = true;
            }
        }

        XFree(protocols);
    }
}

/**
 * Read WM_TRANSIENT_FOR hint.
 */
void
Client::getTransientForHint(void)
{
    if (_transient == None) {
        XGetTransientForHint(_dpy, _window, &_transient);
    }
}

//! @brief
void
Client::getStrutHint(void)
{
    int num = 0;
    long *strut = static_cast<long*>(AtomUtil::getEwmhPropData(_window,
                                                               Atoms::getAtom(NET_WM_STRUT),
                                                               XA_CARDINAL, num));
    if (strut) {
        if (_strut) {
            PScreen::instance()->removeStrut(_strut);
        } else {
            _strut = new Strut();
        }

        *_strut = strut;
        PScreen::instance()->addStrut(_strut);

        XFree(strut);

    } else if (_strut) {
        PScreen::instance()->removeStrut(_strut);
        delete _strut;
        _strut = 0;
    }
}

//! @brief
void
Client::removeStrutHint(void)
{
    if (! _strut)
        return;

    PScreen::instance()->removeStrut(_strut);
    delete _strut;
    _strut = 0;
}

/**
 * Get _PEKWM_FRAME_ORDER hint from client, return < 0 on failure.
 */
long
Client::getPekwmFrameOrder(void)
{
    long num = -1;
    AtomUtil::getLong(_window, Atoms::getAtom(PEKWM_FRAME_ORDER), num);
    return num;
}

/**
 * Update _PEKWM_FRAME_ORDER hint on client window.
 */
void
Client::setPekwmFrameOrder(long num)
{
    AtomUtil::setLong(_window, Atoms::getAtom(PEKWM_FRAME_ORDER), num);
}

/**
 * Get _PEKWM_FRAME_ACTIVE hint from client window, return true if
 * client is treated as active.
 */
bool
Client::getPekwmFrameActive(void)
{
    long act = 0;
    return (AtomUtil::getLong(_window, Atoms::getAtom(PEKWM_FRAME_ACTIVE), act)
            && act == 1);
}

/**
 * Set _PEKWM_FRAME_ACTIVE hint on client window.
 */
void
Client::setPekwmFrameActive(bool act)
{
    AtomUtil::setLong(_window, Atoms::getAtom(PEKWM_FRAME_ACTIVE), act ? 1 : 0);
}
