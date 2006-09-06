//
// Client.cc for pekwm
// Copyright (C) 2002-2006 Claes Nästén <me{@}pekdon{.}net>
//
// client.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#include "../config.h"
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
#include "Viewport.hh"
#include "WindowManager.hh"

#ifdef MENUS
#include "PMenu.hh"
#include "WORefMenu.hh"
#endif // MENUS

#include <cstdio>

extern "C" {
#include <X11/Xatom.h>
#ifdef HAVE_SHAPE
#include <X11/extensions/shape.h>
#endif // HAVE_SHAPE
}

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

using std::vector;
using std::list;
using std::find;
using std::string;

list<Client*> Client::_client_list = list<Client*>();
list<uint> Client::_clientid_list = list<uint>();

Client::Client(WindowManager *w, Window new_client, bool is_new)
    : PWinObj(w->getScreen()->getDpy()),
      _wm(w), _id(0), _size(NULL),
      _transient(None), _strut(NULL),
      _class_hint(NULL),
      _alive(false), _marked(false),
      _send_focus_message(false), _send_close_message(false),
      _cfg_request_lock(false),
#ifdef HAVE_SHAPE
      _shaped(false),
#endif // HAVE_SHAPE
      _extended_net_name(false)
{
    // Construct the client
    PScreen::instance()->grabServer();

    // PWinObj attributes
    _window = new_client;
    _type = WO_CLIENT;

    // Get unique Client id
    _id = findClientID();
    _title.setId(_id);
    _title.infoAdd(PDecor::TitleItem::INFO_ID);

    if (!validate()) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Couldn't validate client." << endl;
#endif // DEBUG
        PScreen::instance()->ungrabServer(true);
        return;
    }

    XWindowAttributes attr;
    if (!XGetWindowAttributes(_dpy, _window, &attr)) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Client died." << endl;
#endif // DEBUG
        PScreen::instance()->ungrabServer(true);
        return;
    }
    _gm.x = attr.x;
    _gm.y = attr.y;
    _gm.width = attr.width;
    _gm.height = attr.height;

    _cmap = attr.colormap;
    _size = XAllocSizeHints();

    // get trans window and the window attributes
    getTransientForHint();

    XSetWindowAttributes sattr;
    sattr.event_mask =
        PropertyChangeMask|StructureNotifyMask|FocusChangeMask;
    sattr.do_not_propagate_mask =
        ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;

    // We don't want these masks to be propagated down to the frame
    XChangeWindowAttributes(_dpy, _window, CWEventMask|CWDontPropagate, &sattr);

#ifdef HAVE_SHAPE
    if (PScreen::instance()->hasExtensionShape()) {
        XShapeSelectInput(_dpy, _window, ShapeNotifyMask);
    }
#endif // HAVE_SHAPE

    XAddToSaveSet(_dpy, _window);
    XSetWindowBorderWidth(_dpy, _window, 0);

    // Load the Class hint before loading the autoprops and
    // getting client title as we search for TitleRule in the autoprops
    _class_hint = new ClassHint();
    readClassRoleHints();

    getWMNormalHints();
    getXClientName();
    getXIconName();

    // cyclic dependency, getting the name requires quiering autoprops
    _class_hint->title = _title.getReal();

    // Get Autoproperties before EWMH as we need the cfg_deny property.
    AutoProperty *ap = readAutoprops(_wm->isStartup()
                                     ? APPLY_ON_NEW : APPLY_ON_START);

    readMwmHints(); // read atoms
    readEwmhHints();
    readPekwmHints();
    getWMProtocols();

    XWMHints* hints = XGetWMHints(_dpy, _window);
    if (hints != NULL) {
        // get the input focus mode
        if (hints->flags&InputHint) { // FIXME: More logic needed
// 			if (hints->input) {
// 				_focusable = true;
// 			} else {
// 				_focusable = false;
// 			}
        }

        // get initial state of the window
        if (hints->flags&StateHint) {
            setWmState(hints->initial_state);
        } else {
            setWmState(NormalState);
        }

        XFree(hints);
    }

    // are we iconified
    if (getWmState() == IconicState) {
        _iconified = true;
    }

    // grab keybindings and mousebutton actions
    KeyGrabber::instance()->grabKeys(_window);
    grabButtons();

    // don't want to spread the giveInputFocuses
    bool do_focus = is_new ? Config::instance()->isFocusNew() : false;
    bool do_autogroup = true;

    // If pekwm allready is running, check against autoproperty then
    // tagged frame. If starting up, check if it has a FRAME_ID and if not
    // use autoproperties.
    if (_wm->isStartup()) {
        // Check for tagged frame
        Frame *frame = Frame::getTagFrame();
        if (frame && frame->isMapped()) {
            _parent = frame;
            frame->addChild(this);

            if (! Frame::getTagBehind()) {
                frame->activateChild(this);
                do_focus = frame->isFocused();
            }
        }

        // Not startup, pekwm (re)start it is.
    } else {
        long id;
        if (AtomUtil::getLong(_window,
                              PekwmAtoms::instance()->getAtom(PEKWM_FRAME_ID),
                              id)) {
            do_autogroup = false;
            _parent = _wm->findFrameFromId(id);
            if (_parent) {
                Frame *frame = static_cast<Frame*>(_parent);
                frame->addChild(this);
                frame->activateChild(this);
                do_focus = frame->isFocused();
            }
        }
    }


    // Apply Autoproperties again to override EWMH state. It's done twice as
    // we need the cfg_deny property when reading the EWMH state.
    if (ap) {
        applyAutoprops(ap);

        if (do_autogroup && !_parent && (ap->group_size >= 0)) {
            Frame* frame = _wm->findGroup(ap);
            if (frame) {
                frame->addChild(this);

                if (ap->group_behind == false) {
                    frame->activateChild(this);
                    do_focus = frame->isFocused();
                }
                if (ap->group_raise) {
                    frame->raise();
                }
            }
        }
    }

    // Tell the world about our state
    updateEwmhStates();

    PScreen::instance()->ungrabServer(true);

    // if we don't have a frame allready, create a new one
    if (_parent == NULL) {
        _parent = new Frame(_wm, this, ap);
    }

    // make sure the window is mapped, this is done after it has been
    // added to the decor/frame as otherwise IsViewable state won't
    // be correct and we don't know wheter or not to place the window
    PWinObj::mapWindow();

    // Let us hear what autoproperties has to say about focusing
    if (is_new && (ap != NULL) && ap->isMask(AP_FOCUS_NEW)) {
        do_focus = ap->focus_new;
    }

    // Only can give input focus to mapped windows
    if (_parent->isMapped()) {
        // Ordinary focus
        if (do_focus) {
            _parent->giveInputFocus();

            // Check if we are transient, and if we want to focus
        } else if ((_transient != None)
                   && Config::instance()->isFocusNewChild()) {
            Client *trans_for = _wm->findClientFromWindow(_transient);
            if (trans_for && trans_for->isFocused()) {
                _parent->giveInputFocus();
            }
        }
    }

    _alive = true;

    // Finished creating the client, so now adding it to the client list.
    _wo_list.push_back(this);
    _client_list.push_back(this);
}

//! @brief Client destructor
Client::~Client(void)
{
    // Remove from lists
    _wo_list.remove(this);
    _client_list.remove(this);

    _clientid_list.push_back(_id);

    PScreen::instance()->grabServer();

#ifdef MENUS
    WORefMenu *wo_ref_menu;

    wo_ref_menu = static_cast<WORefMenu*>(_wm->getMenu("WINDOW"));
    if (this == wo_ref_menu->getWORef()) {
        wo_ref_menu->setWORef(NULL);
        wo_ref_menu->unmapAll();
    }

    wo_ref_menu = static_cast<WORefMenu*>(_wm->getMenu("DECORMENU"));
    if (this == wo_ref_menu->getWORef()) {
        wo_ref_menu->setWORef(NULL);
        wo_ref_menu->unmapAll();
    }
#endif // MENUS

    // removes gravity and moves it back to root if we are alive
    bool focus = false;
    if (_parent && (_parent->getType() == PWinObj::WO_FRAME)) {
        focus = _parent->isFocused();
        static_cast<PDecor*>(_parent)->removeChild(this);
    }

    // Focus the parent if we had focus before
    if (focus && (_transient != None)) {
        Client *trans_cli = _wm->findClientFromWindow(_transient);
        if ((trans_cli != NULL) &&
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
    if (_size)
        XFree(_size);

    removeStrutHint();

    if (_class_hint)
        delete _class_hint;

    PScreen::instance()->ungrabServer(true);
}

// START - PWinObj interface.

//! @brief Maps the window.
void
Client::mapWindow(void)
{
    if (_mapped)
        return;

    if (_iconified) {
        _iconified = false;
        setWmState(NormalState);
        updateEwmhStates();
    }

    if(!_transient) // unmap our transient windows if we have any
        _wm->findTransientsToMapOrUnmap(_window, false);

    XSelectInput(_dpy, _window, NoEventMask);
    PWinObj::mapWindow();
    XSelectInput(_dpy, _window,
                 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}


//! @brief Sets the client to WithdrawnState and unmaps it.
void
Client::unmapWindow(void)
{
    if (!_mapped)
        return;

    if (_iconified) { // set the state of the window
        setWmState(IconicState);
        updateEwmhStates();
    }

    XSelectInput(_dpy, _window, NoEventMask);
    PWinObj::unmapWindow();
    XSelectInput(_dpy, _window,
                 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}

//! @brief Iconifies the client and adds it to the iconmenu
void
Client::iconify(void)
{
    if (_iconified)
        return;
    _iconified = true;

    if (!_transient)
        _wm->findTransientsToMapOrUnmap(_window, true);

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
Client::move(int x, int y, bool do_virtual)
{
    bool request = ((_gm.x != x) || (_gm.y != y));

    _gm.x = x;
    _gm.y = y;

    if (request) {
        configureRequestSend();
    }
}

//! @brief
void
Client::moveVirtual(int x, int y)
{
    AtomUtil::setPosition(_window,
                          PekwmAtoms::instance()->getAtom(PEKWM_FRAME_VPOS),
                          x, y);
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
            AtomUtil::setLong(_window, EwmhAtoms::instance()->getAtom(NET_WM_DESKTOP),
                              NET_WM_STICKY_WINDOW);
        } else {
            AtomUtil::setLong(_window, EwmhAtoms::instance()->getAtom(NET_WM_DESKTOP),
                              _workspace);
        }
    }
}

//! @brief Gives the Client input focus.
bool
Client::giveInputFocus(void)
{
    return PWinObj::giveInputFocus();
}

//! @brief Reparents and sets _parent member, filtering unmap events
void
Client::reparent(PWinObj *parent, int x, int y)
{
    XSelectInput(_dpy, _window, NoEventMask);
    PWinObj::reparent(parent, x, y);
    XSelectInput(_dpy, _window,
                 PropertyChangeMask|StructureNotifyMask|FocusChangeMask);
}

//! @brief
ActionEvent*
Client::handleUnmapEvent(XUnmapEvent *ev)
{
    if ((ev->window != ev->event) && (ev->send_event != true)) {
        return NULL;
    }
#ifdef DEBUG
    cerr << __FILE__ << "@" << __LINE__ << ": "
         << "Client(" << this << ")::handleUnmapEvent(" << ev << ")" << endl
         << " *** unmapping client, window: " << _window << endl;
#endif // DEBUG

    // FIXME: Listen mask should change as this doesn't work?
    //_alive = false;
    delete this;

    return NULL;
}

// END - PWinObj interface.

//! @brief Checks if the window has any Destroy or Unmap notifys.
bool
Client::validate(void)
{
    XSync(_dpy, false);

    XEvent ev;
    if (XCheckTypedWindowEvent(_dpy, _window, DestroyNotify, &ev) ||
            XCheckTypedWindowEvent(_dpy, _window, UnmapNotify, &ev)) {
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

    list<ActionEvent> *actions =
        Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_CHILD_FRAME);
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
    if (ActionUtil::needToggle(sa, _state.cfg_deny&deny) == false) {
        return;
    }

    if (_state.cfg_deny&deny) {
        _state.cfg_deny &= ~deny;
    } else {
        _state.cfg_deny |= deny;
    }
}

void
Client::readClassRoleHints(void)
{
    // class hint
    XClassHint class_hint;
    if (XGetClassHint(_dpy, _window, &class_hint)) {
        _class_hint->h_name = class_hint.res_name;
        _class_hint->h_class = class_hint.res_class;
        XFree(class_hint.res_name);
        XFree(class_hint.res_class);
    }

    // wm window role
    _class_hint->h_role = "";
    AtomUtil::getString(_window, IcccmAtoms::instance()->getAtom(WM_WINDOW_ROLE),
                        _class_hint->h_role);
}

//! @brief Loads the Clients state from EWMH atoms.
void
Client::readEwmhHints(void)
{
    EwmhAtoms *ewmh = EwmhAtoms::instance(); // convenience

    // which workspace do we belong to?
    long workspace = -1;
    AtomUtil::getLong(_window, ewmh->getAtom(NET_WM_DESKTOP), workspace);
    if (workspace < 0) {
        _workspace = Workspaces::instance()->getActive();
        AtomUtil::setLong(_window, ewmh->getAtom(NET_WM_DESKTOP), _workspace);
    } else {
        _workspace = workspace;
    }

    // try to figure out what kind of window we are and alter it acordingly
    int items;
    Atom *atoms = NULL;

    bool found_window_type = false;
    atoms = (Atom*)
            AtomUtil::getEwmhPropData(_window, ewmh->getAtom(WINDOW_TYPE),
                                      XA_ATOM, items);
    if (atoms) {
        for (int i = 0; i < items; ++i) {
            found_window_type = true;

            if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_DESKTOP)) {
                // desktop windows, make it the same size as the screen and place
                // it below all windows withouth any decorations, also make it sticky
                _gm.x = _gm.y = 0;
                _gm.width = PScreen::instance()->getWidth();
                _gm.height = PScreen::instance()->getHeight();
                setTitlebar(false);
                setBorder(false);
                _sticky = true;
                _state.placed = true;
                _state.skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_SNAP;
                _layer = LAYER_DESKTOP;
                break;
            } else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_DOCK)) {
                setTitlebar(false);
                setBorder(false);
                _sticky = true;
                _layer = LAYER_DOCK;
                break;
            } else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_TOOLBAR)) {
                setTitlebar(false);
                setBorder(false);
                _state.skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE;
                break;
            } else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_MENU)) {
                _layer = LAYER_MENU;
                _state.skip = SKIP_MENUS|SKIP_FOCUS_TOGGLE|SKIP_SNAP;
                break;
// 			} else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_UTILITY)) {
// 			}
// 			} else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_DIALOG)) {
// 			}
            } else if (atoms[i] == ewmh->getAtom(WINDOW_TYPE_SPLASH)) {
                setTitlebar(false);
                setBorder(false);
            } else {
                found_window_type = false;
            }
        }

        XFree(atoms);
    }

    if (!found_window_type) {
        Atom type = ewmh->getAtom(WINDOW_TYPE_NORMAL);
        AtomUtil::setAtom(_window, ewmh->getAtom(WINDOW_TYPE), type);
    }

    // The _NET_WM_STATE overides the _NET_WM_TYPE
    NetWMStates win_states;
    if (getEwmhStates(win_states) == true) {
        if (win_states.hidden) _iconified = true;
        if (win_states.shaded) _state.shaded = true;
        if (win_states.max_vert) _state.maximized_vert = true;
        if (win_states.max_horz) _state.maximized_horz = true;
        if (win_states.skip_taskbar) _state.skip_taskbar = true;
        if (win_states.skip_pager) _state.skip_pager = true;
        if (win_states.sticky) _sticky = true;
        if (win_states.above) _layer = LAYER_BELOW;
        if (win_states.below) _layer = LAYER_ABOVE_DOCK;
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

    if (mwm_hints != NULL) {

        if (mwm_hints->flags&MWM_HINTS_FUNCTIONS) {
            if (mwm_hints->functions&MWM_FUNC_ALL != 0) {
                if (mwm_hints->functions&MWM_FUNC_RESIZE == 0)
                    _actions.resize = false;
                if (mwm_hints->functions&MWM_FUNC_MOVE == 0)
                    _actions.move = false;
                if (mwm_hints->functions&MWM_FUNC_ICONIFY == 0)
                    _actions.minimize = false;
                if (mwm_hints->functions&MWM_FUNC_CLOSE == 0)
                    _actions.close = false;
                if (mwm_hints->functions&MWM_FUNC_MAXIMIZE == 0) {
                    _actions.maximize_vert = true;
                    _actions.maximize_horz = true;
                }
            }
        }

        if (mwm_hints->flags&MWM_HINTS_DECORATIONS) {
            if (!mwm_hints->decorations&MWM_DECOR_ALL) {
                setTitlebar(mwm_hints->decorations&MWM_DECOR_TITLE);
                setBorder(mwm_hints->decorations&MWM_DECOR_BORDER);
            }
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

    PekwmAtoms *atoms = PekwmAtoms::instance();

    // Virtual position overides real position
    AtomUtil::getPosition(_window, atoms->getAtom(PEKWM_FRAME_VPOS),
                          _gm.x, _gm.y);

    // Get decor state
    if (AtomUtil::getLong(_window, atoms->getAtom(PEKWM_FRAME_DECOR),
                          value)) {
        _state.decor = value;
    }
    // Get skip state
    if (AtomUtil::getLong(_window, atoms->getAtom(PEKWM_FRAME_SKIP),
                          value)) {
        _state.skip = value;
    }

    // Get custom title
    if (AtomUtil::getString(_window, atoms->getAtom(PEKWM_TITLE), str)) {
        _title.setUser(str);
    }
}

//! @brief Tries to find a AutoProp for the current client.
//! @param type Defaults to 0
AutoProperty*
Client::readAutoprops(uint type)
{
    AutoProperty *data =
        AutoProperties::instance()->findAutoProperty(_class_hint, _workspace, type);

    if ((data == NULL) ||
            (_transient
             ? !data->isApplyOn(APPLY_ON_TRANSIENT|APPLY_ON_TRANSIENT_ONLY)
             : data->isApplyOn(APPLY_ON_TRANSIENT_ONLY))) {
        return NULL;
    }

    applyAutoprops(data);

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

//! @brief Tries to get the NET_WM name, else fall back to XA_WM_NAME
void
Client::getXClientName(void)
{
    string title;

    // read X11 property
    XTextProperty text_property;
    if ((XGetWMName(_dpy, _window, &text_property) == False) ||
            (text_property.value == NULL) || (text_property.nitems == 0)){
        return;
    }

    if (text_property.encoding == XA_STRING) {
        title = (const char*) text_property.value;

    } else {
        char **list;
        int num;

        XmbTextPropertyToTextList(_dpy, &text_property, &list, &num);
        if ((num > 0) && (*list != NULL)) {
            title = *list;
            XFreeStringList(list);
        }
    }

    XFree(text_property.value);

    // Mirror it on the visible
    _title.setCustom("");
    _title.setCount(titleFindID(title));
    _title.setReal(title);

    // Apply title rules and find unique name, doesn't apply on
    // user-set titles
    if (titleApplyRule(title)) {
        _title.setCustom(title);
    }
}

//! @brief Searches for an TitleRule and if found, applies it
//! @param title Title to apply rule on.
//! @return true if rule was applied, else false.
bool
Client::titleApplyRule(std::string &title)
{
    _class_hint->title = title;
    TitleProperty *data =
        AutoProperties::instance()->findTitleProperty (_class_hint);
    _class_hint->title = "";

    if (data) {
        return data->getTitleRule ().ed_s (title);
    } else {
        return false;
    }
}

//! @brief Searches for a unique ID within Clients having the same title
//! @param title Title of client to find ID for.
//! @return Number of clients with that id.
uint
Client::titleFindID(std::string &title)
{
    // Do not search for unique IDs if it is not enabled.
    if (Config::instance()->getClientUniqueName() == false) {
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

//! @brief Tries to get Client's icon name, and puts it in _icon_name.
void
Client::getXIconName(void)
{
    _icon_name = "";

    XTextProperty text_property;
    if (!XGetWMIconName(_dpy, _window, &text_property) ||
            !text_property.value || !text_property.nitems)
        return;

    if (text_property.encoding == XA_STRING) {
        _icon_name = (const char*) text_property.value;
    } else {
        char **list;
        int num;

        XmbTextPropertyToTextList(_dpy, &text_property, &list, &num);
        if (num && *list) {
            _icon_name = *list;
            XFreeStringList(list);
        }
    }

    XFree (text_property.value);
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
                    IcccmAtoms::instance()->getAtom(WM_STATE),
                    IcccmAtoms::instance()->getAtom(WM_STATE),
                    32, PropModeReplace, (uchar*) data, 2);
}

// If we can't find a _wm->wm_state we're going to have to assume
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
        XGetWindowProperty(_dpy, _window, IcccmAtoms::instance()->getAtom(WM_STATE),
                           0L, 2L, False, IcccmAtoms::instance()->getAtom(WM_STATE),
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

    AtomUtil::setLong(_window,
                      PekwmAtoms::instance()->getAtom(PEKWM_FRAME_SKIP),
                      _state.skip);
}

//! @brief Sends an WM_DELETE message to the client, else kills it.
void
Client::close(void)
{
    IcccmAtoms *icccm = IcccmAtoms::instance(); // convenience

    if (_send_close_message) {
        sendXMessage(_window, icccm->getAtom(WM_PROTOCOLS), NoEventMask,
                     icccm->getAtom(WM_DELETE_WINDOW), CurrentTime);
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

//! @brief Sets the positoin based on P or U position.
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

//! @brief Get the size closest to the ResizeInc incremeter
//! @param r_w Pointer to put the new width in
//! @param r_h Pointer to put the new height in
//! @param w Width to calculate from
//! @param h Height to calculate from
bool
Client::getIncSize(uint *r_w, uint *r_h, uint w, uint h)
{
    int basex, basey;

    if (_size->flags&PResizeInc) {
        basex = (_size->flags&PBaseSize)
                ? _size->base_width
                : (_size->flags&PMinSize) ? _size->min_width : 0;

        basey = (_size->flags&PBaseSize)
                ? _size->base_height
                : (_size->flags&PMinSize) ? _size->min_height : 0;

        *r_w = w - ((w - basex) % _size->width_inc);
        *r_h = h - ((h - basey) % _size->height_inc);

        return true;
    }

    return false;
}

//! @brief Gets a MwmHint structure from a window. Doesn't free memory.
Client::MwmHints*
Client::getMwmHints(Window win)
{
    Atom real_type; int real_format;
    ulong items_read, items_left;
    MwmHints *data = NULL;
    uchar *udata;

    int status =
        XGetWindowProperty(_dpy, win, _wm->getMwmHintsAtom(),
                           0L, 20L, False, _wm->getMwmHintsAtom(),
                           &real_type, &real_format,
                           &items_read, &items_left,
                           &udata);

    if (status == Success) {
        if (items_read < MWM_HINTS_NUM) {
            XFree(udata);
            udata = NULL;
        }
    } else {
        udata = NULL;
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
             AtomUtil::getEwmhPropData(_window, EwmhAtoms::instance()->getAtom(STATE),
                                       XA_ATOM, num);

    if (states != NULL) {
        EwmhAtoms *ewmh = EwmhAtoms::instance(); // convenience

        for (int i = 0; i < num; ++i) {
            if (states[i] == ewmh->getAtom(STATE_MODAL)) {
                win_states.modal = true;
            } else if (states[i] == ewmh->getAtom(STATE_STICKY)) {
                win_states.sticky = true;
            } else if (states[i] == ewmh->getAtom(STATE_MAXIMIZED_VERT)
                       && !isCfgDeny(CFG_DENY_STATE_MAXIMIZED_VERT)) {
                win_states.max_vert = true;
            } else if (states[i] == ewmh->getAtom(STATE_MAXIMIZED_HORZ)
                       && !isCfgDeny(CFG_DENY_STATE_MAXIMIZED_HORZ)) {
                win_states.max_horz = true;
            } else if (states[i] == ewmh->getAtom(STATE_SHADED)) {
                win_states.shaded = true;
            } else if (states[i] == ewmh->getAtom(STATE_SKIP_TASKBAR)) {
                win_states.skip_taskbar = true;
            } else if (states[i] == ewmh->getAtom(STATE_SKIP_PAGER)) {
                win_states.skip_pager = true;
            } else if (states[i] == ewmh->getAtom(STATE_HIDDEN)
                       && !isCfgDeny(CFG_DENY_STATE_HIDDEN)) {
                win_states.hidden = true;
            } else if (states[i] == ewmh->getAtom(STATE_FULLSCREEN)
                       && !isCfgDeny(CFG_DENY_STATE_FULLSCREEN)) {
                win_states.fullscreen = true;
            } else if (states[i] == ewmh->getAtom(STATE_ABOVE)
                       && !isCfgDeny(CFG_DENY_STATE_ABOVE)) {
                win_states.above = true;
            } else if (states[i] == ewmh->getAtom(STATE_BELOW)
                       && !isCfgDeny(CFG_DENY_STATE_BELOW)) {
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
    EwmhAtoms *ewmh = EwmhAtoms::instance();

    list<Atom> states;

    if (false) // we don't yet support modal state
        states.push_back(ewmh->getAtom(STATE_MODAL));
    if (_sticky)
        states.push_back(ewmh->getAtom(STATE_STICKY));
    if (_state.maximized_vert)
        states.push_back(ewmh->getAtom(STATE_MAXIMIZED_VERT));
    if (_state.maximized_horz)
        states.push_back(ewmh->getAtom(STATE_MAXIMIZED_HORZ));
    if (_state.shaded)
        states.push_back(ewmh->getAtom(STATE_SHADED));
    if (_state.skip_taskbar)
        states.push_back(ewmh->getAtom(STATE_SKIP_TASKBAR));
    if (_state.skip_pager)
        states.push_back(ewmh->getAtom(STATE_SKIP_PAGER));
    if (_iconified)
        states.push_back(ewmh->getAtom(STATE_HIDDEN));
    if (_state.fullscreen)
        states.push_back(ewmh->getAtom(STATE_FULLSCREEN));
    if (_layer == LAYER_ABOVE_DOCK)
        states.push_back(ewmh->getAtom(STATE_ABOVE));
    if (_layer == LAYER_BELOW)
        states.push_back(ewmh->getAtom(STATE_BELOW));

    Atom *atoms = new Atom[(states.size() > 0) ? states.size() : 1];
    if (states.size() > 0) {
        copy(states.begin(), states.end(), atoms);
    }
    AtomUtil::setAtoms(_window, ewmh->getAtom(STATE), atoms, states.size());
    delete [] atoms;
}

//! @brief
void
Client::getWMNormalHints(void)
{
    long dummy;
    XGetWMNormalHints(_dpy, _window, _size, &dummy);
}

//! @brief
void
Client::getWMProtocols(void)
{
    int count;
    Atom *protocols;

    if (XGetWMProtocols(_dpy, _window, &protocols, &count) != 0) {
        IcccmAtoms *icccm = IcccmAtoms::instance(); // convenience

        for (int i = 0; i < count; ++i) {
            if (protocols[i] == icccm->getAtom(WM_TAKE_FOCUS)) {
                _send_focus_message = true;
            } else if (protocols[i] == icccm->getAtom(WM_DELETE_WINDOW)) {
                _send_close_message = true;
            }
        }

        XFree(protocols);
    }
}

//! @brief
void
Client::getTransientForHint(void)
{
    if (!_transient)
        XGetTransientForHint(_dpy, _window, &_transient);
}

//! @brief
void
Client::getStrutHint(void)
{
    int num = 0;
    CARD32 *strut = NULL;

    strut = (CARD32*)
            AtomUtil::getEwmhPropData(_window,
                                      EwmhAtoms::instance()->getAtom(NET_WM_STRUT),
                                      XA_CARDINAL, num);

    if (strut != NULL) {
        if (_strut != NULL) {
            PScreen::instance()->removeStrut(_strut);
        } else {
            _strut = new Strut();
        }

        *_strut = strut;
        PScreen::instance()->addStrut(_strut);

        XFree(strut);

    } else if (_strut != NULL) {
        PScreen::instance()->removeStrut(_strut);
        delete _strut;
        _strut = NULL;
    }

    Workspaces::instance()->getActiveViewport()->hintSetWorkarea();
}

//! @brief
void
Client::removeStrutHint(void)
{
    if (_strut == NULL)
        return;

    PScreen::instance()->removeStrut(_strut);
    delete _strut;
    _strut = NULL;

    Workspaces::instance()->getActiveViewport()->hintSetWorkarea();
}

