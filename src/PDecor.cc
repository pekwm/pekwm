//
// PDecor.cc for pekwm
// Copyright © 2004-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

#include "Compat.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PFont.hh"
#include "PDecor.hh"
#include "x11.hh"
#include "PTexture.hh"
#include "PTexturePlain.hh" // PTextureSolid
#include "ActionHandler.hh"
#include "ScreenResources.hh"
#include "StatusWindow.hh"
#include "KeyGrabber.hh"
#include "Theme.hh"
#include "PixmapHandler.hh"
#include "Workspaces.hh"

using std::cerr;
using std::endl;
using std::find;
using std::list;
using std::map;
using std::mem_fun;
using std::string;
using std::vector;

// PDecor::Button

//! @brief PDecor::Button constructor
PDecor::Button::Button(PWinObj *parent, Theme::PDecorButtonData *data, uint width, uint height)
  : PWinObj(),
    _data(data), _state(BUTTON_STATE_UNFOCUSED),
    _left(_data->isLeft())
{
    _parent = parent;

    _gm.width = width;
    _gm.height = height;

    XSetWindowAttributes attr;
    attr.event_mask = EnterWindowMask|LeaveWindowMask;
    attr.override_redirect = True;
    _window =
        XCreateWindow(X11::getDpy(), _parent->getWindow(),
                      -_gm.width, -_gm.height, _gm.width, _gm.height, 0,
                      CopyFromParent, InputOutput, CopyFromParent,
                      CWEventMask|CWOverrideRedirect, &attr);

    _bg = ScreenResources::instance()->getPixmapHandler()->getPixmap(_gm.width, _gm.height, X11::getDepth());

    setBackgroundPixmap(_bg);
    setState(_state);
}

//! @brief PDecor::Button destructor
PDecor::Button::~Button(void)
{
    XDestroyWindow(X11::getDpy(), _window);
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_bg);
}

//! @brief Searches the PDecorButtonData for an action matching ev
ActionEvent*
PDecor::Button::findAction(XButtonEvent *ev)
{
    list<ActionEvent>::iterator it(_data->begin());
    for (; it != _data->end(); ++it) {
        if (it->mod == ev->state && it->sym == ev->button)
            return &*it;
    }
    return 0;
}

//! @brief Sets the state of the button
void
PDecor::Button::setState(ButtonState state)
{
    if (state == BUTTON_STATE_NO) {
        return;
    }

    // Only update if we don't hoover, we want to be able to turn back
    if (state != BUTTON_STATE_HOVER) {
        _state = state;
    }

    PTexture *texture = _data->getTexture(state);
    if (texture) {
        texture->render(_bg, 0, 0, _gm.width, _gm.height);

#ifdef HAVE_SHAPE
        // Get shape mask
        bool need_free;
        Pixmap shape = _data->getTexture(state)->getMask(0, 0, need_free);
        if (shape != None) {
            X11::shapeSetMask(_window, shape);
            if (need_free) {
                ScreenResources::instance()->getPixmapHandler()->returnPixmap(shape);
            }
        } else {
            XRectangle rect = {0 /* x */, 0 /* y */, _gm.width, _gm.height };
            X11::shapeSetRect(_window, &rect);
        }
#endif // HAVE_SHAPE
        clear();
    }
}

//! @brief Update visible version of title
void
PDecor::TitleItem::updateVisible(void) {
    // Start with empty string
    _visible = L"";

    // Add client info to title
    if ((_info != 0)
        && ((_info != INFO_ID) || Config::instance()->isShowClientID())) {
        _visible.append(L"[");

        if (infoIs(INFO_ID) && Config::instance()->isShowClientID()) {
            _visible.append(Util::to_wide_str(Util::to_string<uint>(_id)));
        }
        if (infoIs(INFO_MARKED)) {
            _visible.append(L"M");
        }

        _visible.append(L"] ");
    }

    // Add title
    if (_user.size() > 0) {
        _visible.append(_user);
    } else if (_custom.size() > 0) {
        _visible.append(_custom);
    } else {
        _visible.append(_real);
    }

    // Add client number to title
    if (_count > 0) {
      _visible.append(Util::to_wide_str(Config::instance()->getClientUniqueNamePre()));
      _visible.append(Util::to_wide_str(Util::to_string(_count)));
      _visible.append(Util::to_wide_str(Config::instance()->getClientUniqueNamePost()));
    }
}

// PDecor

const string PDecor::DEFAULT_DECOR_NAME = string("DEFAULT");
const string PDecor::DEFAULT_DECOR_NAME_BORDERLESS = string("BORDERLESS");
const string PDecor::DEFAULT_DECOR_NAME_TITLEBARLESS = string("TITLEBARLESS");
const string PDecor::DEFAULT_DECOR_NAME_ATTENTION = string("ATTENTION");

list<PDecor*> PDecor::_pdecor_list = list<PDecor*>();

//! @brief PDecor constructor
//! @param dpy Display
//! @param theme Theme
//! @param decor_name String, if not DEFAULT_DECOR_NAME sets _decor_name_override
PDecor::PDecor(Theme *theme,
               const std::string decor_name, const Window child_window)
    : PWinObj(),
      _theme(theme),_decor_name(decor_name),
      _child(0), _button(0), _button_press_win(None),
      _pointer_x(0), _pointer_y(0),
      _click_x(0), _click_y(0),
      _decor_cfg_keep_empty(false), _decor_cfg_child_move_overloaded(false),
      _decor_cfg_bpr_replay_pointer(false),
      _decor_cfg_bpr_al_child(MOUSE_ACTION_LIST_CHILD_OTHER),
      _decor_cfg_bpr_al_title(MOUSE_ACTION_LIST_TITLE_OTHER),
      _maximized_vert(false), _maximized_horz(false),
      _fullscreen(false), _skip(0), _data(0), 
      _border(true), _titlebar(true), _shaded(false),
      _need_shape(false), _need_client_shape(false),
      _dirty_resized(true), _real_height(1),
      _title_wo(), _title_bg(None),
      _title_active(0), _titles_left(0), _titles_right(1)
{
    if (_decor_name != PDecor::DEFAULT_DECOR_NAME) {
        _decor_name_override = _decor_name;
    }

    // we be reset in loadDecor later on, inlines using the _data used before
    // loadDecor needs this though
    _data = _theme->getPDecorData(_decor_name);
    if (! _data) {
        _data = _theme->getPDecorData(DEFAULT_DECOR_NAME);
    }

    CreateWindowParams window_params;
    getParentWindowAttributes(window_params, child_window);
    createParentWindow(window_params);
    if (window_params.mask & CWColormap) {
        window_params.depth = X11::getDepth();
        window_params.visual = X11::getVisual()->getXVisual();
        window_params.attr.colormap = X11::getColormap();
    }
    createTitle(window_params);
    createBorder(window_params);

    // sets buttons etc up
    loadDecor();

    // map title and border windows
    XMapSubwindows(X11::getDpy(), _window);

    _pdecor_list.push_back(this);
}

/**
 * Create window attributes
 */
void
PDecor::getParentWindowAttributes(CreateWindowParams &params,
                                  Window child_window)
{
    params.mask = CWOverrideRedirect|CWEventMask|CWBorderPixel|CWBackPixel;
    params.depth = CopyFromParent;
    params.visual = CopyFromParent;
    params.attr.override_redirect = True;
    params.attr.border_pixel = 0;
    params.attr.background_pixel = 0;

    if (child_window != None) {
        getChildWindowAttributes(params, child_window);
    }
}

/**
 * Get window attributes from child window
 */
void
PDecor::getChildWindowAttributes(CreateWindowParams &params,
                                 Window child_window)
{
    XWindowAttributes attr;
    Status status =  XGetWindowAttributes(X11::getDpy(), child_window, &attr);
    if (status != BadDrawable && status != BadWindow && attr.depth == 32) {
        params.mask |= CWColormap;
        params.depth = attr.depth;
        params.visual = attr.visual;
        params.attr.colormap = XCreateColormap(X11::getDpy(),
                                               X11::getRoot(),
                                               params.visual, AllocNone);
    }
}

/**
 * Create container window.
 */
void
PDecor::createParentWindow(CreateWindowParams &params)
{
    params.attr.event_mask = ButtonPressMask|ButtonReleaseMask|
        ButtonMotionMask|EnterWindowMask|SubstructureRedirectMask|
        SubstructureNotifyMask;
    _window = XCreateWindow(X11::getDpy(), X11::getRoot(),
                            _gm.x, _gm.y, _gm.width, _gm.height, 0,
                            params.depth, InputOutput, params.visual,
                            params.mask, &params.attr);
}

/**
 * Create title window.
 */
void
PDecor::createTitle(CreateWindowParams &params)
{
    params.attr.event_mask = ButtonPressMask|ButtonReleaseMask|
        ButtonMotionMask|EnterWindowMask;
    Window title = XCreateWindow(X11::getDpy(), _window,
                                 borderLeft(), borderTop(), 1, 1, 0,
                                 params.depth, InputOutput, params.visual,
                                 params.mask, &params.attr);
    _title_wo.setWindow(title);
    addChildWindow(_title_wo.getWindow());
}

/**
 * Create border windows
 */
void
PDecor::createBorder(CreateWindowParams &params)
{
    params.attr.event_mask = ButtonPressMask|ButtonReleaseMask|
        ButtonMotionMask|EnterWindowMask;

    ScreenResources *sr = ScreenResources::instance();
    for (uint i = 0; i < BORDER_NO_POS; ++i) {
        params.attr.cursor = sr->getCursor(ScreenResources::CursorType(i));

        _border_win[i] =
            XCreateWindow(X11::getDpy(), _window, -1, -1, 1, 1, 0,
                          params.depth, InputOutput, params.visual,
                          params.mask|CWCursor, &params.attr);
        _border_pos_map[BorderPosition(i)] = None;
        addChildWindow(_border_win[i]);
    }
}

//! @brief PDecor destructor
PDecor::~PDecor(void)
{
    _pdecor_list.remove(this);

    if (_child_list.size() > 0) {
        while (_child_list.size() != 0) {
            removeChild(_child_list.back(), false); // Don't call delete this.
        }
    }

    // Make things look smoother, buttons will be noticed as deleted
    // otherwise. Using X call directly to avoid re-drawing and other
    // special features not required when removing the window.
    X11::unmapWindow(_window);

    // free buttons
    unloadDecor();

    // free border pixmaps
    map<BorderPosition, Pixmap>::iterator it(_border_pos_map.begin());
    for (; it != _border_pos_map.end(); ++it) {
        if (it->second != None) {
            ScreenResources::instance()->getPixmapHandler()->returnPixmap(it->second);
        }
    }
    ScreenResources::instance()->getPixmapHandler()->returnPixmap(_title_bg);

    removeChildWindow(_title_wo.getWindow());
    XDestroyWindow(X11::getDpy(), _title_wo.getWindow());
    for (uint i = 0; i < BORDER_NO_POS; ++i) {
        removeChildWindow(_border_win[i]);
        XDestroyWindow(X11::getDpy(), _border_win[i]);
    }
    XDestroyWindow(X11::getDpy(), _window);
}

// START - PWinObj interface.

//! @brief Map decor and all children
void
PDecor::mapWindow(void)
{
    if (! _mapped) {
        PWinObj::mapWindow();
        for_each(_child_list.begin(), _child_list.end(),
                 mem_fun(&PWinObj::mapWindow));
    }
}

//! @brief Maps the window raised
void
PDecor::mapWindowRaised(void)
{
    if (_mapped) {
        return;
    }
    
    mapWindow();

    raise(); // XMapRaised wouldn't preserver layers
}

//! @brief Unmaps decor and all children
void
PDecor::unmapWindow(void)
{
    if (_mapped) {
        if (_iconified) {
            for_each(_child_list.begin(), _child_list.end(),
                     mem_fun(&PWinObj::iconify));
        } else {
            for_each(_child_list.begin(), _child_list.end(),
                     mem_fun(&PWinObj::unmapWindow));
        }
        PWinObj::unmapWindow();
    }
}

//! @brief Moves the decor
void
PDecor::move(int x, int y)
{
    // update real position
    PWinObj::move(x, y);
    if (_child && (_decor_cfg_child_move_overloaded)) {
        _child->move(x + borderLeft(), y + borderTop() + getTitleHeight());
    }
}

//! @brief Resizes the decor, and active child if any
void
PDecor::resize(uint width, uint height)
{
    // If shaded, don't resize to the size specified just update width
    // and set _real_height to height
    if (_shaded) {
        _real_height = height;

        height = getTitleHeight();
        // Shading in non full width title mode will make border go away
        if (! _data->getTitleWidthMin()) {
            height += borderTop() + borderBottom();
        }
    }

    PWinObj::resize(width, height);

    // Update size before moving and shaping the rest as shaping
    // depends on the child window
    if (_child) {
        _child->resize(getChildWidth(), getChildHeight());
    }

    // place / resize title and border
    resizeTitle();
    placeBorder();

    // render title and border
    _dirty_resized = true;

    // Set and apply shape on window, all parts of the border can now
    // be shaped.
    setBorderShape();
    applyBorderShape();

    renderTitle();
    renderBorder();

    _dirty_resized = false;
}

//! @brief Move and resize window.
void
PDecor::moveResize(int x, int y, uint width, uint height)
{
    // If shaded, don't resize to the size specified just update width
    // and set _real_height to height
    if (_shaded) {
        _real_height = height;

        height = getTitleHeight();
        // Shading in non full width title mode will make border go away
        if (! _data->getTitleWidthMin()) {
            height += borderTop() + borderBottom();
        }
    }

    PWinObj::moveResize(x, y, width, height);

    // Update size before moving and shaping the rest as shaping
    // depends on the child window
    if (_child) {
        _child->moveResize(x + borderLeft(), y + borderTop() + getTitleHeight(),
                          getChildWidth(), getChildHeight());

        // The client window may have its window gravity set to something different
        // than NorthWestGravity (see Xlib manual chapter 3.2.3). Therefore the 
        // X server may not move its top left corner along with the decoration.
        // We correct these cases by calling alignChild(). It is called only for 
        // _child and not all members of _child_list, because activateChild.*() 
        // does it itself.
        alignChild(_child);
    }

    // Place and resize title and border
    resizeTitle();
    placeBorder();

    // render title and border
    _dirty_resized = true;

    // Apply shape on window, all parts of the border can now be shaped.
    setBorderShape();
    applyBorderShape();

    renderTitle();
    renderBorder();

    _dirty_resized = false;
}

//! @brief
void
PDecor::resizeTitle(void)
{
    if (getTitleHeight()) { 
       _title_wo.resize(calcTitleWidth(), getTitleHeight());
        calcTabsWidth();
    }

    // place buttons, also updates title information
    placeButtons();
}

//! @brief Raises the window, taking _layer into account
void
PDecor::raise(void)
{
    Workspaces::raise(this);
    Workspaces::updateClientStackingList();
}

//! @brief Lowers the window, taking _layer into account
void
PDecor::lower(void)
{
    Workspaces::lower(this);
    Workspaces::updateClientStackingList();
}

//! @brief
void
PDecor::setFocused(bool focused)
{
    if (_focused != focused) { // save repaints
        PWinObj::setFocused(focused);

        renderTitle();
        renderButtons();

        renderBorder();
        setBorderShape();
        applyBorderShape();
    }
}

//! @brief
void
PDecor::setWorkspace(uint workspace)
{
    if (workspace != NET_WM_STICKY_WINDOW) {
        if (workspace >= Workspaces::size()) {
#ifdef DEBUG
            cerr << __FILE__ << "@" << __LINE__ << ": "
                 << "PDecor(" << this << ")::setWorkspace(" << workspace << ")"
                 << endl << " *** workspace > number of workspaces:"
                 << Workspaces::size () << endl;
#endif // DEBUG
            workspace = Workspaces::size() - 1;
        }
        _workspace = workspace;
    }

    list<PWinObj*>::iterator it (_child_list.begin ());
    for (; it != _child_list.end (); ++it) {
        (*it)->setWorkspace (workspace);
    }

    if (! _mapped && ! _iconified) {
        if (_sticky || (_workspace == Workspaces::getActive())) {
            mapWindow();
        }
    } else if (! _sticky && (_workspace != Workspaces::getActive())) {
        unmapWindow();
    }
}

//! @brief Gives decor input focus, fails if not mapped or not visible
void
PDecor::giveInputFocus(void)
{
    if (_mapped && _child) {
        _child->giveInputFocus();
    } else {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PDecor::giveInputFocus(" << this << ")" << endl << " *** reverting to root" << endl;
#endif // DEBUG
        PWinObj::getRootPWinObj()->giveInputFocus();
    }
}

/**
 * Handle button press events.
 */
ActionEvent*
PDecor::handleButtonPress(XButtonEvent *ev)
{
    ActionEvent *ae = 0;
    list<ActionEvent> *actions = 0;

    // Remove state modifiers from event
    X11::stripStateModifiers(&ev->state);
    X11::stripButtonModifiers(&ev->state);

    // Try to do something about frame buttons
    if (ev->subwindow != None && (_button = findButton(ev->subwindow)) != 0) {
        ae = handleButtonPressButton(ev, _button);

    } else {
        // Record position, used in the motion handler when doing a window move.
        _click_x = _gm.x;
        _click_y = _gm.y;
        _pointer_x = ev->x_root;
        _pointer_y = ev->y_root;

        // Allow us to get clicks from anywhere on the window.
        if (_decor_cfg_bpr_replay_pointer) {
            XAllowEvents(X11::getDpy(), ReplayPointer, CurrentTime);
        }

        if (ev->window == _child->getWindow()
            || (ev->state == 0 && ev->subwindow == _child->getWindow())) {
            // Clicks on the child window
            // NOTE: If the we're matching against the subwindow we need to make
            // sure that the state is 0, meaning we didn't have any modifier
            // because if we don't care about the modifier we'll get two actions
            // performed when using modifiers.
            _button_press_win = _child->getWindow();
            actions = Config::instance()->getMouseActionList(_decor_cfg_bpr_al_child);
            
        } else if (_title_wo == ev->window) {
            // Clicks on the decor title
            _button_press_win = ev->window;
            actions = Config::instance()->getMouseActionList(_decor_cfg_bpr_al_title);
            
        } else {
            // Clicks on the decor border, default case. Try both window and sub-window.
            uint pos = getBorderPosition(ev->window);
            if (pos != BORDER_NO_POS) {
                _button_press_win = ev->window;
                actions = Config::instance()->getBorderListFromPosition(pos);
            }
        }
    }

    if (! ae && actions) {
        ae = ActionHandler::findMouseAction(ev->button, ev->state, MOUSE_EVENT_PRESS, actions);
    }

    return ae;
}

/**
 * Handle button press events pressing decor buttons.
 */
ActionEvent*
PDecor::handleButtonPressButton(XButtonEvent *ev, PDecor::Button *button)
{
    // Keep track of pressed button.
    _button->setState(BUTTON_STATE_PRESSED);

    ActionEvent *ae = _button->findAction(ev);

    // if the button is used for resizing, we don't want to wait for release
    if (ae && ae->isOnlyAction(ACTION_RESIZE)) {
        _button->setState(_focused ? BUTTON_STATE_FOCUSED : BUTTON_STATE_UNFOCUSED);
        _button = 0;
    } else {
        ae = 0;
    }

    return ae;
}

/**
 * Handle button release events.
 */
ActionEvent*
PDecor::handleButtonRelease(XButtonEvent *ev)
{
    ActionEvent *ae = 0;
    list<ActionEvent> *actions = 0;
    MouseEventType mb = MOUSE_EVENT_RELEASE;

    // Remove state modifiers from event
    X11::stripStateModifiers(&ev->state);
    X11::stripButtonModifiers(&ev->state);

    // handle titlebar buttons
    if (_button) {
        ae = handleButtonReleaseButton(ev, _button);

    } else {
        // Allow us to get clicks from anywhere on the window.
        if (_decor_cfg_bpr_replay_pointer) {
            XAllowEvents(X11::getDpy(), ReplayPointer, CurrentTime);
        }

        // clicks on the child window
        if (ev->window == _child->getWindow()
            || (ev->state == 0 && ev->subwindow == _child->getWindow())) {
            // NOTE: If the we're matching against the subwindow we need to make
            // sure that the state is 0, meaning we didn't have any modifier
            // because if we don't care about the modifier we'll get two actions
            // performed when using modifiers.
            if (_button_press_win == _child->getWindow()) {
                actions = Config::instance()->getMouseActionList(_decor_cfg_bpr_al_child);
            }

        } else if (_title_wo == ev->window) {
            if (_button_press_win == ev->window) {
                // Handle clicks on the decor title, checking double clicks first.
                if (X11::isDoubleClick(ev->window, ev->button - 1, ev->time,
                                                       Config::instance()->getDoubleClickTime())) {
                    X11::setLastClickID(ev->window);
                    X11::setLastClickTime(ev->button - 1, 0);
                    mb = MOUSE_EVENT_DOUBLE;
                } else {
                    X11::setLastClickID(ev->window);
                    X11::setLastClickTime(ev->button - 1, ev->time);
                }

                actions = Config::instance()->getMouseActionList(_decor_cfg_bpr_al_title);
            }
        } else {
            // Clicks on the decor border, check subwindow then window.
            uint pos = getBorderPosition(ev->window);
            if (pos != BORDER_NO_POS && (_button_press_win == ev->window)) {
                actions = Config::instance()->getBorderListFromPosition(pos);
            }
        }
    }

    if (! ae && actions) {
        ae = ActionHandler::findMouseAction(ev->button, ev->state, mb, actions);
    }

    return ae;
}

/**
 * Handle button release events when button is in pressed state.
 */
ActionEvent*
PDecor::handleButtonReleaseButton(XButtonEvent *ev, PDecor::Button *button)
{
    // First restore the pressed buttons state
    _button->setState(_focused ? BUTTON_STATE_FOCUSED : BUTTON_STATE_UNFOCUSED);

    ActionEvent *ae = 0;

    // Then see if the button was released over ( to execute an action )
    if (*_button == ev->subwindow) {
        ae = _button->findAction(ev);

        // This is a little hack, resizing isn't wanted on both press and release
        if (ae && ae->isOnlyAction(ACTION_RESIZE)) {
            ae = 0;
        }
    }

    _button = 0;

    return ae;
}

//! @brief
ActionEvent*
PDecor::handleMotionEvent(XMotionEvent *ev)
{
    uint button = X11::getButtonFromState(ev->state);
    return ActionHandler::findMouseAction(button, ev->state,
                                          MOUSE_EVENT_MOTION,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_OTHER));
}

/**
 * Handle enter event, find action and toggle hoover state if enter
 * was on a button.
 */
ActionEvent*
PDecor::handleEnterEvent(XCrossingEvent *ev)
{
    PDecor::Button *button = findButton(ev->window);
    if (button) {
        button->setState(BUTTON_STATE_HOVER);
    }

    return ActionHandler::findMouseAction(BUTTON_ANY, ev->state, MOUSE_EVENT_ENTER,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_OTHER));
}

/**
 * Handle leave event, find action and toggle hoover state if leave
 * was from a button.
 */
ActionEvent*
PDecor::handleLeaveEvent(XCrossingEvent *ev)
{
    PDecor::Button *button = findButton(ev->window);
    if (button) {
        button->setState(button->getState());
    }

    return ActionHandler::findMouseAction(BUTTON_ANY, ev->state, MOUSE_EVENT_LEAVE,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_OTHER));
}

// END - PWinObj interface.

//! @brief Adds a children in another decor to this decor
void
PDecor::addDecor(PDecor *decor)
{
    if (this == decor) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PDecor(" << this << ")::addDecor(" << decor << ")" << " *** this == decor" << endl;
#endif // DEBUG
        return;
    }

    list<PWinObj*>::iterator it(decor->begin());
    for (; it != decor->end(); ++it) {
        addChild(*it);
        (*it)->setWorkspace(_workspace);
    }

    decor->_child_list.clear();
    delete decor;
}

//! @brief Sets preferred decor_name
//! @return True on change, False if unchanged
bool
PDecor::setDecor(const std::string &name)
{
    string new_name(_decor_name_override);
    if (new_name.size() == 0) {
        new_name = name;
    }

    if (_decor_name == new_name) {
        return false;
    }

    _decor_name = new_name;
    loadDecor();

    return true;
}

//! @brief Sets override decor name
void
PDecor::setDecorOverride(StateAction sa, const std::string &name)
{
    if ((sa == STATE_SET) ||
            ((sa == STATE_TOGGLE) && (_decor_name_override.size() == 0))) {
        _decor_name_override = name;
        setDecor("");

    } else if ((sa == STATE_UNSET) ||
               ((sa == STATE_TOGGLE) && (_decor_name_override.size() > 0))) {
        _decor_name_override = ""; // .clear() doesn't work with old g++
        updateDecorName();
    }
}

//! @brief Load and update decor.
void
PDecor::loadDecor(void)
{
    unloadDecor();

    // Get decordata with name.
    _data = _theme->getPDecorData(_decor_name);
    if (! _data) {
        _data = _theme->getPDecorData(DEFAULT_DECOR_NAME);
    }
    assert(_data);

    // Load decor.
    list<Theme::PDecorButtonData*>::iterator b_it(_data->buttonBegin());
    for (; b_it != _data->buttonEnd(); ++b_it) {
        uint width = std::max(static_cast<uint>(1), (*b_it)->getWidth() ? (*b_it)->getWidth() : getTitleHeight());
        uint height = std::max(static_cast<uint>(1), (*b_it)->getHeight() ? (*b_it)->getHeight() : getTitleHeight());

        _button_list.push_back(new PDecor::Button(&_title_wo, *b_it, width, height));
        _button_list.back()->mapWindow();
        addChildWindow(_button_list.back()->getWindow());
    }

    // Update title position.
    if (_data->getTitleWidthMin() == 0) {
        _title_wo.move(borderTopLeft(), borderTop());
        _need_shape = false;
    } else {
        _title_wo.move(0, 0);
        _need_shape = true;
    }

    // Update child positions.
    list<PWinObj*>::iterator c_it(_child_list.begin());
    for (; c_it != _child_list.end(); ++c_it) {
        alignChild(*c_it);
    }

    // Make sure it gets rendered correctly.
    _dirty_resized = true;
    _focused = ! _focused;
    setFocused(! _focused);
    _dirty_resized = false;

    // Update the dimension of the frame.
    if (_child) {
        resizeChild(_child->getWidth(), _child->getHeight());
    }

    // Child theme change.
    loadTheme();
}

//! @brief Frees resources used by PDecor.
void
PDecor::unloadDecor(void)
{
    // Set active button to 0 as it can not be valid after deleting
    // the current buttons.
    _button = 0;

    list<PDecor::Button*>::iterator it(_button_list.begin());
    for (; it != _button_list.end(); ++it) {
        removeChildWindow((*it)->getWindow());
        delete *it;
    }
    _button_list.clear();
}

//! @brief
PDecor::Button*
PDecor::findButton(Window win)
{
    list<PDecor::Button*>::iterator it(_button_list.begin());
    for (; it != _button_list.end(); ++it) {
        if (**it == win) {
            return *it;
        }
    }

    return 0;
}

//! @brief
PWinObj*
PDecor::getChildFromPos(int x)
{
    if (! _child_list.size() || (_child_list.size() != _title_list.size()))
        return 0;
    if (_child_list.size() == 1)
        return _child_list.front();

    if (x < static_cast<int>(_titles_left)) {
        return _child_list.front();
    } else if (x > static_cast<int>(_title_wo.getWidth() - _titles_right)) {
        return _child_list.back();
    }

    PTexture *t_sep = _data->getTextureSeparator(getFocusedState(false));
    uint sepw = t_sep?t_sep->getWidth():0;
    list<PWinObj*>::iterator c_it(_child_list.begin());
    list<PDecor::TitleItem*>::iterator t_it(_title_list.begin());

    uint pos = _titles_left, xx = x;
    for (uint i = 0; i < _title_list.size(); ++i, ++t_it, ++c_it) {
        if (xx >= pos && xx <= pos + (*t_it)->getWidth() + sepw) {
            return *c_it;
        }
        pos += (*t_it)->getWidth() + sepw;
    }

    return 0;
}

//! @brief Moves making the child be positioned at x y
void
PDecor::moveChild(int x, int y)
{
    move(x - borderLeft(), y - borderTop() - getTitleHeight());
}

//! @brief Resizes the decor, giving width x height space for the child
void
PDecor::resizeChild(uint width, uint height)
{
    resize(width + borderLeft() + borderRight(),
           height + borderTop() + borderBottom() + getTitleHeight());
}

//! @brief Sets border state of the decor
void
PDecor::setBorder(StateAction sa)
{
    if (! ActionUtil::needToggle(sa, _border)) {
        return;
    }

    // If we are going to remove the border, we need to check carefully
    // that we don't try to make the window disappear.
    if (! _border) {
        _border = true;
    } else if (! _shaded || _titlebar) {
        _border = false;
    }

    restackBorder();
    if (! updateDecorName() && _child)
        resizeChild(_child->getWidth(), _child->getHeight());
}

//! @brief Sets titlebar state of the decor
void
PDecor::setTitlebar(StateAction sa)
{
    if (! ActionUtil::needToggle(sa, _titlebar))
        return;

    // If we are going to remove the titlebar, we need to check carefully
    // that we don't try to make the window disappear.
    if (! _titlebar) {
        _title_wo.mapWindow();
        _titlebar = true;
    } else if (! _shaded || _border) {
        _title_wo.unmapWindow();
        _titlebar = false;
    }

    // If updateDecorName returns true, it already loaded decor stuff for us.
    if (! updateDecorName() && _child) {
        alignChild(_child);
        resizeChild(_child->getWidth(), _child->getHeight());
    }
}

//! @brief Calculate title height, 0 if titlebar is disabled.
uint
PDecor::getTitleHeight(void) const
{
    if (! _titlebar) {
        return 0;
    }

    if (_data->isTitleHeightAdapt()) {
        return getFont(getFocusedState(false))->getHeight()
              + _data->getPad(PAD_UP) + _data->getPad(PAD_DOWN);
    } else {
        return _data->getTitleHeight();
    }
}

//! @brief Adds a child to the decor, reparenting the window
void
PDecor::addChild(PWinObj *child, std::list<PWinObj*>::iterator *it)
{
    child->reparent(this, borderLeft(), borderTop() + getTitleHeight());
    if (it == 0) {
        _child_list.push_back(child);
    } else {
        _child_list.insert(*it, child);
    }

    updatedChildOrder();

    // Sync focused state if it is the first child, the child will be
    // activated later on. If there are children here already fit the
    // child into the decor.
    if (_child_list.size() == 1) {
        _focused = ! _focused;
        setFocused(! _focused);
    } else {
      alignChild(child);
      child->resize(getChildWidth(), getChildHeight());
    }
}

//! @brief Removes PWinObj from this PDecor.
//! @param child PWinObj to remove from the this PDecor.
//! @param do_delete Wheter to call delete this when empty. (Defaults to true)
void
PDecor::removeChild(PWinObj *child, bool do_delete)
{
    child->reparent(PWinObj::getRootPWinObj(),
                    _gm.x + borderLeft(),
                    _gm.y + borderTop() + getTitleHeight());

    list<PWinObj*>::iterator it(find(_child_list.begin(), _child_list.end(), child));
    if (it == _child_list.end()) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PDecor(" << this << ")::removeChild(" << child << ")" << endl
             << " *** child not in _child_list, bailing out." << endl;
#endif // DEBUG
        return;
    }

    it = _child_list.erase(it);

    if (_child_list.size() > 0) {
        if (_child == child) {
            if (it == _child_list.end()) {
                --it;
            }

            activateChild(*it);
        }

        updatedChildOrder();

    } else if (_decor_cfg_keep_empty) {
        updatedChildOrder();

    } else if (do_delete) {
        delete this; // no children, and we don't want empty PDecors
    }
}

//! @brief Activates the child, updating size and position
void
PDecor::activateChild(PWinObj *child)
{
    // sync state
    _focusable = child->isFocusable();

    alignChild(child); // place correct acording to border and title
    child->resize(getChildWidth(), getChildHeight());
    child->raise();
    _child = child;

    restackBorder();

    updatedChildOrder();
}

//! @brief
void
PDecor::getDecorInfo(wchar_t *buf, uint size)
{
    swprintf(buf, size, L"%dx%d+%d+%d", _gm.width, _gm.height, _gm.x, _gm.y);
}

//! @brief
void
PDecor::activateChildNum(uint num)
{
    if (num >= _child_list.size()) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PDecor(" << this << ")::activeChildNum(" << num << ")" << endl
             << " *** num > _child_list.size()" << endl;
#endif // DEBUG
        return;
    }

    list<PWinObj*>::iterator it(_child_list.begin());
    for (uint i = 0; i < num; ++i, ++it);

    activateChild(*it);
}

//! @brief
void
PDecor::activateChildRel(int off)
{
    PWinObj *child = getChildRel(off);
    if (! child) {
        child = _child_list.front();
    }
    activateChild(child);
}

//! @brief Moves the current child off steps
//! @param off - for left and + for right
void
PDecor::moveChildRel(int off)
{
    PWinObj *child = getChildRel(off);
    if (! child || (child == _child)) {
        return;
    }

    bool at_begin = (_child == _child_list.front());
    bool at_end = (_child == _child_list.back());

    // switch place
    _child_list.remove(_child);
    list<PWinObj*>::iterator it(find(_child_list.begin(), _child_list.end(), child));
    if (off > 0) {
        // when wrapping, we want to be able to insert at first place
        if (at_begin || (*it != _child_list.front())) {
            ++it;
        }

        _child_list.insert(it, _child);
    } else {
        if (! at_end && (*it == _child_list.back())) {
            ++it;
        }
        _child_list.insert(it, _child);
    }

    updatedChildOrder();
}

//! @brief
PWinObj*
PDecor::getChildRel(int off)
{
    if ((off == 0) || (_child_list.size() < 2)) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PDecor(" << this << ")::getChildRel(" << off << ")" << endl
             << " *** off == 0 or not enough children" << endl;
#endif // DEBUG
        return 0;
    }

    list<PWinObj*>::iterator it(find(_child_list.begin(), _child_list.end(), _child));
    // if no active child, use first
    if (it == _child_list.end()) {
        it = _child_list.begin();
    }

    int dir = (off > 0) ? 1 : -1;
    off = abs(off);

    for (int i = 0; i < off; ++i) {
        if (dir == 1) { // forward
            if (++it == _child_list.end()) {
                it = _child_list.begin();
            }

        } else { // backward
            if (it == _child_list.begin()) {
                it = _child_list.end();
            }
            --it;
        }
    }

    return *it;
}

//! @brief Do move of Decor with the mouse.
//! @param x_root X position of the pointer when event was triggered.
//! @param y_root Y position of the pointer when event was triggered.
void
PDecor::doMove(int x_root, int y_root)
{
    if (! allowMove()) {
        return;
    }

    StatusWindow *sw = StatusWindow::instance(); // convenience

    if (! X11::grabPointer(X11::getRoot(), ButtonMotionMask|ButtonReleaseMask,
                         ScreenResources::instance()->getCursor(ScreenResources::CURSOR_MOVE))) {
        return;
    }

    // Get relative position to root
    int x = x_root - _gm.x;
    int y = y_root - _gm.y;

    bool outline = ! Config::instance()->getOpaqueMove();
    bool center_on_root = Config::instance()->isShowStatusWindowOnRoot();
    EdgeType edge;

    // grab server, we don't want invert traces
    if (outline) {
        X11::grabServer();
    }

    wchar_t buf[128];
    getDecorInfo(buf, 128);

    if (Config::instance()->isShowStatusWindow()) {
        sw->draw(buf, true, center_on_root ? 0 : &_gm);
        sw->mapWindowRaised();
        sw->draw(buf, true, center_on_root ? 0 : &_gm);
    }

    Geometry last_gm(_gm);

    XEvent e;
    const long move_mask = ButtonPressMask|ButtonReleaseMask|PointerMotionMask;
    bool exit = false;
    while (! exit) {
        if (outline) {
            drawOutline(_gm);
        }
        XMaskEvent(X11::getDpy(), move_mask, &e);
        if (outline) {
            drawOutline(_gm); // clear
        }

        switch (e.type) {
        case MotionNotify:
            // Flush all pointer motion, no need to redraw and redraw.
            X11::removeMotionEvents();

            _gm.x = e.xmotion.x_root - x;
            _gm.y = e.xmotion.y_root - y;
            checkSnap();

            if (! outline && _gm != last_gm) {
                last_gm = _gm;
                move(_gm.x, _gm.y);
            }

            edge = doMoveEdgeFind(e.xmotion.x_root, e.xmotion.y_root);
            if (edge != SCREEN_EDGE_NO) {
                doMoveEdgeAction(&e.xmotion, edge);
            }

            getDecorInfo(buf, 128);
            if (Config::instance()->isShowStatusWindow()) {
                sw->draw(buf, true, center_on_root ? 0 : &_gm);
            }

            break;
        case ButtonRelease:
            exit = true;
            break;
        }
    }

    if (Config::instance()->isShowStatusWindow()) {
        sw->unmapWindow();
    }

    // ungrab the server
    if (outline) {
        move(_gm.x, _gm.y);
        X11::ungrabServer(true);
    }

    X11::ungrabPointer();
}

//! @brief Matches cordinates against screen edge
EdgeType
PDecor::doMoveEdgeFind(int x, int y)
{
    EdgeType edge = SCREEN_EDGE_NO;
    if (x <= signed(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_LEFT))) {
        edge = SCREEN_EDGE_LEFT;
    } else if (x >= signed(X11::getWidth() -
                           Config::instance()->getScreenEdgeSize(SCREEN_EDGE_RIGHT))) {
        edge = SCREEN_EDGE_RIGHT;
    } else if (y <= signed(Config::instance()->getScreenEdgeSize(SCREEN_EDGE_TOP))) {
        edge = SCREEN_EDGE_TOP;
    } else if (y >= signed(X11::getHeight() -
                           Config::instance()->getScreenEdgeSize(SCREEN_EDGE_BOTTOM))) {
        edge = SCREEN_EDGE_BOTTOM;
    }

    return edge;
}

//! @brief Finds and executes action if any
void
PDecor::doMoveEdgeAction(XMotionEvent *ev, EdgeType edge)
{
    ActionEvent *ae;

    uint button = X11::getButtonFromState(ev->state);
    ae = ActionHandler::findMouseAction(button, ev->state,
                                        MOUSE_EVENT_ENTER_MOVING,
                                        Config::instance()->getEdgeListFromPosition(edge));

    if (ae) {
        ActionPerformed ap(this, *ae);
        ap.type = ev->type;
        ap.event.motion = ev;

        ActionHandler::instance()->handleAction(ap);
    }
}

/**
 * Move and resize window with keybindings.
 */
void
PDecor::doKeyboardMoveResize(void)
{
    StatusWindow *sw = StatusWindow::instance(); // convenience

    if (! X11::grabPointer(X11::getRoot(), NoEventMask,
                         ScreenResources::instance()->getCursor(ScreenResources::CURSOR_MOVE))) {
        return;
    }
    if (! X11::grabKeyboard(X11::getRoot())) {
        X11::ungrabPointer();
        return;
    }

    Geometry old_gm = _gm; // backup geometry if we cancel
    bool outline = (! Config::instance()->getOpaqueMove() ||
                    ! Config::instance()->getOpaqueResize());
    ActionEvent *ae;
    list<Action>::iterator it;

    wchar_t buf[128];
    getDecorInfo(buf, 128);

    bool center_on_root = Config::instance()->isShowStatusWindowOnRoot();    
    if (Config::instance()->isShowStatusWindow()) {
        sw->draw(buf, true, center_on_root ? 0 : &_gm);
        sw->mapWindowRaised();
        sw->draw(buf, true, center_on_root ? 0 : &_gm);
    }

    if (outline) {
        X11::grabServer();
    }

    XEvent e;
    bool exit = false;
    while (! exit) {
        if (outline) {
            drawOutline(_gm);
        }
        XMaskEvent(X11::getDpy(), KeyPressMask, &e);
        if (outline) {
            drawOutline(_gm); // clear
        }

        if ((ae = KeyGrabber::instance()->findMoveResizeAction(&e.xkey)) != 0) {
            for (it = ae->action_list.begin(); it != ae->action_list.end(); ++it) {
                switch (it->getAction()) {
                case MOVE_HORIZONTAL:
                    _gm.x += it->getParamI(0);
                    if (! outline) {
                        move(_gm.x, _gm.y);
                    }
                    break;
                case MOVE_VERTICAL:
                    _gm.y += it->getParamI(0);
                    if (! outline) {
                        move(_gm.x, _gm.y);
                    }
                    break;
                case RESIZE_HORIZONTAL:
                    _gm.width += resizeHorzStep(it->getParamI(0));
                    if (! outline) {
                        resize(_gm.width, _gm.height);
                    }
                    break;
                case RESIZE_VERTICAL:
                    _gm.height += resizeVertStep(it->getParamI(0));
                    if (! outline) {
                        resize(_gm.width, _gm.height);
                    }
                    break;
                case MOVE_SNAP:
                    checkSnap();
                    if (! outline) {
                        move(_gm.x, _gm.y);
                    }
                    break;
                case MOVE_CANCEL:
                    _gm = old_gm; // restore position

                    if (! outline) {
                        move(_gm.x, _gm.y);
                        resize(_gm.width, _gm.height);
                    }

                    exit = true;
                    break;
                case MOVE_END:
                    if (outline) {
                        if ((_gm.x != old_gm.x) || (_gm.y != old_gm.y))
                            move(_gm.x, _gm.y);
                        if ((_gm.width != old_gm.width) || (_gm.height != old_gm.height))
                            resize(_gm.width, _gm.height);
                    }
                    exit = true;
                    break;
                default:
                    // do nothing
                    break;
                }

                getDecorInfo(buf, 128);
                if (Config::instance()->isShowStatusWindow()) {
                    sw->draw(buf, true, center_on_root ? 0 : &_gm);
                }
            }
        }
    }

    if (Config::instance()->isShowStatusWindow()) {
        sw->unmapWindow();
    }

    if (outline) {
        X11::ungrabServer(true);
    }

    X11::ungrabKeyboard();
    X11::ungrabPointer();
}

//! @brief Sets shaded state
void
PDecor::setShaded(StateAction sa)
{
    if (! ActionUtil::needToggle(sa, _shaded))
        return;

    // If we are going to shade the window, we need to check carefully
    // that we don't try to make the window disappear.
    if (_shaded) {
        _gm.height = _real_height;
        _shaded = false;
        // If we have a titlebar OR border (border will only be visible in
        // full-width mode only)
    } else if (_titlebar || (_border && ! _data->getTitleWidthMin())) {
        _real_height = _gm.height;
        _gm.height = getTitleHeight();
        // shading in non full width title mode will make border go away
        if (! _data->getTitleWidthMin()) {
            _gm.height += borderTop() + borderBottom();
        }
        _shaded = true;
    }

    setBorderShape();
    placeBorder();
    restackBorder();
    PWinObj::resize(_gm.width, _gm.height);
}

//! @brief Sets skip state.
void
PDecor::setSkip(uint skip)
{
    _skip = skip;
}

//! @brief Remove iconified state.
void PDecor::deiconify(void) {
    if (_iconified) {
        if (_workspace == Workspaces::getActive()) {
            mapWindow();
        }
        _iconified = false;
    }
}

//! @brief Renders and sets title background
void
PDecor::renderTitle(void)
{
    if (! getTitleHeight()) {
        return;
    }

    bool force_update = false;
    if (_data->getTitleWidthMin()) {
        uint width_before = _title_wo.getWidth();

        resizeTitle();
        applyBorderShape(); // update title shape

        if (width_before != _title_wo.getWidth()) {
            force_update = true;
        }
    } else {
        calcTabsWidth();
    }

    PFont *font = getFont(getFocusedState(false));
    PTexture *t_sep = _data->getTextureSeparator(getFocusedState(false));
    PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();

    // Get new title pixmap
    if (_dirty_resized || force_update) {
        pm->returnPixmap(_title_bg);
        _title_bg = pm->getPixmap(_title_wo.getWidth(), _title_wo.getHeight(), X11::getDepth());
        _title_wo.setBackgroundPixmap(_title_bg);
    }

    // Render main background on pixmap
    _data->getTextureMain(getFocusedState(false))->render(_title_bg, 0, 0,
                                                          _title_wo.getWidth(), _title_wo.getHeight());

    bool sel; // Current tab selected flag
    uint x = _titles_left; // Position
    uint pad_horiz =  _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT); // Amount of horizontal padding

    list<PDecor::TitleItem*>::iterator it(_title_list.begin());
    for (uint i = 0; it != _title_list.end(); ++i, ++it) {
        sel = (_title_active == i);

        // render tab
        _data->getTextureTab(getFocusedState(sel))->render(_title_bg, x, 0,
                                                           (*it)->getWidth(), _title_wo.getHeight());

        font = getFont(getFocusedState(sel));
        font->setColor(_data->getFontColor(getFocusedState(sel)));

        if ((*it)) {
            PFont::TrimType trim = PFont::FONT_TRIM_MIDDLE;
            if ((*it)->isCustom() || (*it)->isUserSet()) {
                trim = PFont::FONT_TRIM_END;
            }

            font->draw(_title_bg,
                       x + _data->getPad(PAD_LEFT), // X position
                       _data->getPad(PAD_UP), // Y position
                       (*it)->getVisible(), 0, // Text and max chars
                       (*it)->getWidth() - pad_horiz, // Available width
                       trim); // Type of trim
        }

        // move to next tab (or separator if any)
        x += (*it)->getWidth();

        // draw separator
        if ((_title_list.size() > 1) && (i < (_title_list.size() - 1))) {
            t_sep->render(_title_bg, x, 0, 0, 0);
            x += t_sep->getWidth();
        }
    }

    _title_wo.clear();
}

//! @brief
void
PDecor::renderButtons(void)
{
    list<PDecor::Button*>::iterator it(_button_list.begin());
    for (; it != _button_list.end(); ++it) {
        (*it)->setState(_focused ? BUTTON_STATE_FOCUSED : BUTTON_STATE_UNFOCUSED);
    }
}

//! @brief Renders the border of the decor.
void
PDecor::renderBorder(void)
{
    if (! _border) {
        return;
    }

    map<BorderPosition, Pixmap>::iterator it;
    PixmapHandler *pm = ScreenResources::instance()->getPixmapHandler();

    // Return pixmaps used for the border
    if (_dirty_resized) {
        for (it = _border_pos_map.begin(); it != _border_pos_map.end(); ++it) {
            pm->returnPixmap(it->second);
            it->second = None;
        }
    }

    PTexture *tex;
    FocusedState state = getFocusedState(false);
    uint width, height;

    for (it = _border_pos_map.begin(); it != _border_pos_map.end(); ++it) {
        tex = _data->getBorderTexture(state, it->first);

        // Solid texture, get the color and set as bg, no need to render pixmap
        if (tex->getType() == PTexture::TYPE_SOLID) {
            XSetWindowBackground(X11::getDpy(), _border_win[it->first],
                                 static_cast<PTextureSolid*>(tex)->getColor()->pixel);

        } else {
            // not a solid texture, get pixmap, render, set as bg
            getBorderSize(it->first, width, height);

            if ((width > 0) && (height > 0)) {
                if (_dirty_resized) {
                    it->second = pm->getPixmap(width, height,
                                               X11::getDepth());
                    XSetWindowBackgroundPixmap(X11::getDpy(), _border_win[it->first],
                                               it->second);
                }

                tex->render(it->second, 0, 0, width, height);
            }
        }

        XClearWindow(X11::getDpy(), _border_win[it->first]);
    }
}

//! @brief Sets shape on the border windows.
void
PDecor::setBorderShape(void)
{
#ifdef HAVE_SHAPE
    Pixmap pix;
    bool do_free;
    unsigned int width, height;
    FocusedState state = getFocusedState(false);

    XRectangle rect = {0, 0, 0, 0};

    map<BorderPosition, Pixmap>::iterator it(_border_pos_map.begin());
    for (; it != _border_pos_map.end(); ++it) {
        // Get the size of the border at position
        getBorderSize(it->first, width, height);

        // Get shape pixmap
        pix =  _data->getBorderTexture(state, it->first)->getMask(width, height,
                                                                  do_free);
        if (pix != None) {
            _need_shape = true;
            X11::shapeSetMask(_border_win[it->first], pix);
            if (do_free) {
                ScreenResources::instance()->getPixmapHandler()->returnPixmap(pix);
            }
        } else {
            rect.width = width;
            rect.height = height;
            X11::shapeSetRect(_border_win[it->first], &rect);
        }
    }
#endif // HAVE_SHAPE
}

//! @brief Finds the Head closest to x y from the center of the decor
uint
PDecor::getNearestHead(void)
{
    return X11::getNearestHead(_gm.x + (_gm.width / 2), _gm.y + (_gm.height / 2));
}

//! @brief
void
PDecor::checkSnap(void)
{
    if ((Config::instance()->getWOAttract() > 0) ||
            (Config::instance()->getWOResist() > 0)) {
        checkWOSnap();
    }
    if ((Config::instance()->getEdgeAttract() > 0) ||
            (Config::instance()->getEdgeResist() > 0)) {
        checkEdgeSnap();
    }
}

inline bool
isBetween(int x1, int x2, int t1, int t2)
{
    if (x1 > t1) {
        if (x1 < t2) {
            return true;
        }
    } else if (x2 > t1) {
        return true;
    }
    return false;
}

//! @brief
//! @todo PDecor/PWinObj doesn't have _skip property
void
PDecor::checkWOSnap(void)
{
    PDecor *decor;
    Geometry gm = _gm;

    int x = getRX();
    int y = getBY();

    int attract = Config::instance()->getWOAttract();
    int resist = Config::instance()->getWOResist();

    bool snapped;

    vector<PWinObj*>::reverse_iterator it = _wo_list.rbegin();
    for (; it != _wo_list.rend(); ++it) {
        if (((*it) == this) || ! (*it)->isMapped() || ((*it)->getType() != PWinObj::WO_FRAME)) {
            continue;
        }

        // Skip snapping, only valid on PDecor and up.
        decor = dynamic_cast<PDecor*>(*it);
        if (decor && decor->isSkip(SKIP_SNAP)) {
            continue;
        }

        snapped = false;

        // check snap
        if ((x >= ((*it)->getX() - attract)) && (x <= ((*it)->getX() + resist))) {
            if (isBetween(gm.y, y, (*it)->getY(), (*it)->getBY())) {
                _gm.x = (*it)->getX() - gm.width;
                snapped = true;
            }
        } else if ((gm.x >= signed((*it)->getRX() - resist)) &&
                   (gm.x <= signed((*it)->getRX() + attract))) {
            if (isBetween(gm.y, y, (*it)->getY(), (*it)->getBY())) {
                _gm.x = (*it)->getRX();
                snapped = true;
            }
        }

        if (y >= ((*it)->getY() - attract) && (y <= (*it)->getY() + resist)) {
            if (isBetween(gm.x, x, (*it)->getX(), (*it)->getRX())) {
                _gm.y = (*it)->getY() - gm.height;
                if (snapped)
                    break;
            }
        } else if ((gm.y >= signed((*it)->getBY() - resist)) &&
                   (gm.y <= signed((*it)->getBY() + attract))) {
            if (isBetween(gm.x, x, (*it)->getX(), (*it)->getRX())) {
                _gm.y = (*it)->getBY();
                if (snapped)
                    break;
            }
        }
    }
}

//! @brief Snaps decor agains head edges. Only updates _gm, no real move.
//! @todo Add support for checking for harbour and struts
void
PDecor::checkEdgeSnap(void)
{
    int attract = Config::instance()->getEdgeAttract();
    int resist = Config::instance()->getEdgeResist();

    Geometry head;
    X11::getHeadInfoWithEdge(X11::getNearestHead(_gm.x, _gm.y), head);

    if ((_gm.x >= (head.x - resist)) && (_gm.x <= (head.x + attract))) {
        _gm.x = head.x;
    } else if ((_gm.x + _gm.width) >= (head.x + head.width - attract) &&
               ((_gm.x + _gm.width) <= (head.x + head.width + resist))) {
        _gm.x = head.x + head.width - _gm.width;
    }
    if ((_gm.y >= (head.y - resist)) && (_gm.y <= (head.y + attract))) {
        _gm.y = head.y;
    } else if (((_gm.y + _gm.height) >= (head.y + head.height - attract)) &&
               ((_gm.y + _gm.height) <= (head.y + head.height + resist))) {
        _gm.y = head.y + head.height - _gm.height;
    }
}


//! @brief Set shapes of decor
//! @return true if shape is on, else false.
bool
PDecor::setShape(void)
{
#ifdef HAVE_SHAPE
    if (! _child) {
        return false;
    }

    int num;
    XRectangle *rect;

    rect = X11::shapeGetRects(_child->getWindow(), &num);
    // If there is more than one Rectangle it has an irregular shape.
    _need_client_shape = (num > 1);

    // Will set or unset shape depending on _need_shape and _need_client_shape.
    applyBorderShape();

    if (rect) {
        XFree(rect);
    }

    return (num > 1);
#else // !HAVE_SHAPE
    return false;
#endif // HAVE_SHAPE
}

/**
 * Move child into position with regards to title and border.
 */
void
PDecor::alignChild(PWinObj *child)
{
    if (child) {
        XMoveWindow(X11::getDpy(), child->getWindow(), borderLeft(), borderTop() + getTitleHeight());
    }
}

//! @brief Draws outline of the decor with geometry gm
void
PDecor::drawOutline(const Geometry &gm)
{
    XDrawRectangle(X11::getDpy(), X11::getRoot(),
                   _theme->getInvertGC(),
                   gm.x, gm.y, gm.width,
                   _shaded ? _gm.height : gm.height);
}

//! @brief Places decor buttons
void
PDecor::placeButtons(void)
{
    _titles_left = 0;
    _titles_right = 0;

    list<PDecor::Button*>::iterator it(_button_list.begin());
    for (; it != _button_list.end(); ++it) {
        if ((*it)->isLeft()) {
            (*it)->move(_titles_left, 0);
            _titles_left += (*it)->getWidth();
        } else {
            _titles_right += (*it)->getWidth();
            (*it)->move(_title_wo.getWidth() - _titles_right, 0);
        }
    }
}

//! @brief Places border windows
void
PDecor::placeBorder(void)
{
    // if we have tab min == 0 then we have full width title and place the
    // border ontop, else we put the border under the title
    uint bt_off = (_data->getTitleWidthMin() > 0) ? getTitleHeight() : 0;

    if (borderTop() > 0) {
        XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_TOP],
                          borderTopLeft(), bt_off,
                          _gm.width - borderTopLeft() - borderTopRight(), borderTop());

        XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_TOP_LEFT],
                          0, bt_off,
                          borderTopLeft(), borderTopLeftHeight());
        XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_TOP_RIGHT],
                          _gm.width - borderTopRight(), bt_off,
                          borderTopRight(), borderTopRightHeight());

        if (borderLeft()) {
            XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_LEFT],
                              0, borderTopLeftHeight() + bt_off,
                              borderLeft(), _gm.height - borderTopLeftHeight() - borderBottomLeftHeight());
        }

        if (borderRight()) {
            XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_RIGHT],
                              _gm.width - borderRight(), borderTopRightHeight() + bt_off,
                              borderRight(), _gm.height - borderTopRightHeight() - borderBottomRightHeight());
        }
    } else {
        if (borderLeft()) {
            XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_LEFT],
                              0, getTitleHeight(),
                              borderLeft(), _gm.height - getTitleHeight() - borderBottom());
        }

        if (borderRight()) {
            XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_RIGHT],
                              _gm.width - borderRight(), getTitleHeight(),
                              borderRight(), _gm.height - getTitleHeight() - borderBottom());
        }
    }

    if (borderBottom()) {
        XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_BOTTOM],
                          borderBottomLeft(), _gm.height - borderBottom(),
                          _gm.width - borderBottomLeft() - borderBottomRight(), borderBottom());
        XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_BOTTOM_LEFT],
                          0, _gm.height - borderBottomLeftHeight(),
                          borderBottomLeft(), borderBottomLeftHeight());
        XMoveResizeWindow(X11::getDpy(), _border_win[BORDER_BOTTOM_RIGHT],
                          _gm.width - borderBottomRight(), _gm.height - borderBottomRightHeight(),
                          borderBottomRight(), borderBottomRightHeight());
    }

    applyBorderShape();
}

/**
 * Apply shaping on the window based on the shape of the client and
 * the borders.
 */
void
PDecor::applyBorderShape(void)
{
#ifdef HAVE_SHAPE
    XRectangle rect_square;
    rect_square.x = 0;
    rect_square.y = 0;
    rect_square.width = _gm.width;
    rect_square.height = _gm.height;

    if (_need_shape || _need_client_shape) {
        // if we have tab min == 0 then we have full width title and place the
        // border ontop, else we put the border under the title
        uint bt_off = (_data->getTitleWidthMin() > 0) ? getTitleHeight() : 0;

        Window shape;
        shape = XCreateSimpleWindow(X11::getDpy(), X11::getRoot(),
                                    0, 0, _gm.width, _gm.height, 0, 0, 0);

        if (_child && ! _shaded) {
            X11::shapeCombine(shape, borderLeft(), borderTop() + getTitleHeight(),
                               _child->getWindow(), ShapeSet);
        }

        // Apply border shape. Need to be carefull wheter or not to include it.
        if (_border
                && ! (_shaded && bt_off) // Shaded in non-full-width mode removes border.
                && ! (_need_client_shape)) { // Shaped clients should appear bordeless.
            // top
            if (borderTop() > 0) {
                X11::shapeCombine(shape, 0, bt_off, _border_win[BORDER_TOP_LEFT],
                                  ShapeUnion);

                X11::shapeCombine(shape, borderTopLeft(), bt_off, _border_win[BORDER_TOP],
                                  ShapeUnion);

                X11::shapeCombine(shape, _gm.width - borderTopRight(), bt_off,
                                  _border_win[BORDER_TOP_RIGHT], ShapeUnion);
            }

            bool use_bt_off = bt_off || borderTop();
            // Left border
            if (borderLeft() > 0) {
                X11::shapeCombine(shape, 0, 
                                  use_bt_off ? bt_off + borderTopLeftHeight() : getTitleHeight(),
                                  _border_win[BORDER_LEFT], ShapeUnion);
            }

            // Right border
            if (borderRight() > 0) {
                X11::shapeCombine(shape, _gm.width - borderRight(), 
                                  use_bt_off ? bt_off + borderTopRightHeight() : getTitleHeight(),
                                  _border_win[BORDER_RIGHT], ShapeUnion);
            }

            // bottom
            if (borderBottom() > 0) {
                X11::shapeCombine(shape, 0, _gm.height - borderBottomLeftHeight(), 
                                  _border_win[BORDER_BOTTOM_LEFT], ShapeUnion);

                X11::shapeCombine(shape, borderBottomLeft(), _gm.height - borderBottom(),
                                  _border_win[BORDER_BOTTOM], ShapeUnion);

                X11::shapeCombine(shape, _gm.width - borderBottomRight(), 
                                  _gm.height - borderBottomRightHeight(),
                                  _border_win[BORDER_BOTTOM_RIGHT],
                                  ShapeUnion);
            }
        }
        if (_titlebar) {
            // apply title shape
            X11::shapeCombine(shape, _title_wo.getX(), _title_wo.getY(),
                              _title_wo.getWindow(), ShapeUnion);
        }

        // The borders might extend beyond the window area (causing
        // artifacts under xcompmgr), so we cut the shape down to the
        // original window size.
        X11::shapeIntersectRect(shape, &rect_square);

        // Apply the shape mask to the window
        X11::shapeCombine(_window, 0, 0, shape, ShapeSet);

        XDestroyWindow(X11::getDpy(), shape);
    } else {
        // No shaping is required, apply a square shape to remove
        // possible stale shaping.
        X11::shapeSetRect(_window, &rect_square);
    }
#endif // HAVE_SHAPE
}

//! @brief Restacks child, title and border windows.
void
PDecor::restackBorder(void)
{
    // List of windows, adding two possible for title and child window
    int extra = 0;
    Window windows[BORDER_NO_POS + 2];


    // Only put the Child over if not shaded.
    if (_child && ! _shaded) {
        windows[extra++] = _child->getWindow();
    }
    // Add title if any
    if (_titlebar) {
        windows[extra++] = _title_wo.getWindow();
    }

    // Add border windows
    for (int i = 0; i < BORDER_NO_POS; ++i) {
         windows[i + extra] = _border_win[i];
    }

    // Raise the top window so actual restacking is done.
    XRaiseWindow(X11::getDpy(), windows[0]);
    XRestackWindows(X11::getDpy(), windows, BORDER_NO_POS + extra);
}

//! @brief  Updates _decor_name to represent decor state
//! @return true if decor was changed
bool
PDecor::updateDecorName(void)
{
    string name;

    if (_titlebar && _border) {
        name = PDecor::DEFAULT_DECOR_NAME;
    } else if (_titlebar) {
        name = PDecor::DEFAULT_DECOR_NAME_BORDERLESS;
    } else if (_border) {
        name = PDecor::DEFAULT_DECOR_NAME_TITLEBARLESS;
    }

    return setDecor(name);
}

/**
 * Get size of border at position.
 *
 * @param pos Position to get size for.
 * @param width Width of border at position.
 * @param height Height of border at position.
 */
void
PDecor::getBorderSize(BorderPosition pos, uint &width, uint &height)
{
    FocusedState state = getFocusedState(false); // convenience

    switch (pos) {
    case BORDER_TOP_LEFT:
    case BORDER_TOP_RIGHT:
    case BORDER_BOTTOM_LEFT:
    case BORDER_BOTTOM_RIGHT:
        width = _data->getBorderTexture(state, pos)->getWidth();
        height = _data->getBorderTexture(state, pos)->getHeight();
        break;
    case BORDER_TOP:
        if ((borderTopLeft() + borderTopRight()) < _gm.width) {
            width = _gm.width - borderTopLeft() - borderTopRight();
        } else {
            width = 1;
        }
        height = _data->getBorderTexture(state, pos)->getHeight();
        break;
    case BORDER_BOTTOM:
        if ((borderBottomLeft() + borderBottomRight()) < _gm.width) {
            width = _gm.width - borderBottomLeft() - borderBottomRight();
        } else {
            width = 1;
        }
        height = _data->getBorderTexture(state, pos)->getHeight();
        break;
    case BORDER_LEFT:
        width = _data->getBorderTexture(state, pos)->getWidth();
        if ((borderTopLeftHeight() + borderBottomLeftHeight()) < _gm.height) {
            height = _gm.height - borderTopLeftHeight() - borderBottomLeftHeight();
        } else {
            height = 1;
        }
        break;
    case BORDER_RIGHT:
        width = _data->getBorderTexture(state, pos)->getWidth();
        if ((borderTopRightHeight() + borderBottomRightHeight()) < _gm.height) {
            height = _gm.height - borderTopRightHeight() - borderBottomRightHeight();
        } else {
            height = 1;
        }
        break;
    default:
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "PDecor(" << this << ")::getBorderSize()" << endl << " *** invalid border position" << endl;
#endif // DEBUG
        width = 1;
        height = 1;
        break;
    };
}

/**
 * Calculate width of title, if width min is 0 use the full available
 * size minus borders. Else get width from content.
 */
uint
PDecor::calcTitleWidth(void)
{
    uint width = 0;

    if (_data->getTitleWidthMin() == 0) {
        width = _gm.width;
        if (width > (borderTopLeft() + borderTopRight())) {
            width -= borderTopLeft() + borderTopRight();
        }

    } else {
        uint width_max = 0;
        // FIXME: what about selected tabs?
        PFont *font = getFont(getFocusedState(false));

        if (_data->isTitleWidthSymetric()) {
            // Symetric mode, get max tab width, multiply with number of tabs
            list<PDecor::TitleItem*>::iterator it(_title_list.begin());
            for (; it != _title_list.end(); ++it) {
                width = font->getWidth((*it)->getVisible());
                if (width > width_max) {
                    width_max = width;
                }
            }

            width = width_max + _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT);
            width *= _title_list.size();

        } else {
            // Asymetric mode, get individual widths
            list<PDecor::TitleItem*>::iterator it(_title_list.begin());
            for (; it != _title_list.end(); ++it) {
                width += font->getWidth((*it)->getVisible())
                    + _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT);
            }
        }

        // Add width of separators and buttons
        width += (_title_list.size() - 1)
                 * _data->getTextureSeparator(getFocusedState(false))->getWidth();
        width += _titles_left + _titles_right;

        // Validate sizes, make sure it is not less than width min
        // pixels and more than width max percent.
        if (width < static_cast<uint>(_data->getTitleWidthMin())) {
            width = _data->getTitleWidthMin();
        }
        if (width > (_gm.width * _data->getTitleWidthMax() / 100)) {
            width = _gm.width * _data->getTitleWidthMax() / 100;
        }
    }

    return width;
}

/**
 * Calculate tab width, wrapper to choose correct algorithm..
 */
void
PDecor::calcTabsWidth(void)
{
    if (! _title_list.size()) {
        return;
    }

    if (_data->isTitleWidthSymetric()) {
        calcTabsWidthSymetric();
    } else {
        calcTabsWidthAsymetric();
    }
}

/**
 * Get available tabs width and average width of a single tab.
 *
 * @param width_avail Number of pixels available in the titlebar.
 * @param tab_width Width in pixels to use for one tab.
 * @param off Number of pixels left with tab_width pixels wide tabs.
 */
void
PDecor::calcTabsGetAvailAndTabWidth(uint &width_avail, uint &tab_width, int &off)
{
    // Calculate width
    width_avail = _title_wo.getWidth();

    // Remove spacing if enough space is available
    if (width_avail > (_titles_left + _titles_right)) {
        width_avail -= _titles_left + _titles_right;
    }
 
    // Remove separators if enough space is available
    uint sep_width = (_title_list.size() - 1)
        * _data->getTextureSeparator(getFocusedState(false))->getWidth();
    if (width_avail > sep_width) {
        width_avail -= sep_width;
    }

    tab_width = width_avail / _title_list.size();
    off = width_avail % _title_list.size();
}

//! @brief Calculate tab width symetric, sets up _title_list widths.
void
PDecor::calcTabsWidthSymetric(void)
{
    int off;
    uint width_avail, tab_width;
    calcTabsGetAvailAndTabWidth(width_avail, tab_width, off);

    // Assign width to elements
    list<PDecor::TitleItem*>::iterator it(_title_list.begin());
    for (; it != _title_list.end(); ++it) {
        (*it)->setWidth(tab_width + ((off-- > 0) ? 1 : 0));
    }
}

/**
 * Calculate tab width asymmetrically. This is done with the following
 * priority:
 *
 *   1. Give tabs their required width, if it fits this is complete.
 *   2. Tabs did not fit, calculate average width.
 *   3. Add width of tabs requiring less space to average.
 *   4. Re-assign width equally to tabs using more than average.
 */
void
PDecor::calcTabsWidthAsymetric(void)
{
    int off;
    uint width, width_avail, tab_width;
    calcTabsGetAvailAndTabWidth(width_avail, tab_width, off);

    // Convenience
    PFont *font = getFont(getFocusedState(false));

    // 1. give tabs their required width.
    uint width_total = 0;
    list<PDecor::TitleItem*>::iterator it(_title_list.begin());
    for (; it != _title_list.end(); ++it) {
        // This should set the tab width to be only the size needed
        width = font->getWidth((*it)->getVisible().c_str())
            + _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT) + ((off-- > 0) ? 1 : 0);
        width_total += width;
        (*it)->setWidth(width);
    }

    if (width_total > width_avail) {
        calcTabsWidthAsymetricShrink(width_avail, tab_width);
    } else if (width_total < static_cast<uint>(_data->getTitleWidthMin())) {
        calcTabsWidthAsymetricGrow(width_avail, tab_width);
    }
}

/**
 * This is called to shrink the tabs that are over the tab_width in
 * order to make room for all
 */
void
PDecor::calcTabsWidthAsymetricShrink(uint width_avail, uint tab_width)
{
    // 2. Tabs did not fit
    uint tabs_left = _title_list.size();
    list<PDecor::TitleItem*>::iterator it(_title_list.begin());
    for (; it != _title_list.end(); ++it) {
        if ((*it)->getWidth() < tab_width) {
            // 3. Add width of tabs requiring less space to the average.
            tabs_left--;
            width_avail -= (*it)->getWidth();
        }
    }

    // 4. Re-assign width equally to tabs using more than average
    tab_width = width_avail / tabs_left;
    uint off = width_avail % tabs_left;
    for (it = _title_list.begin(); it != _title_list.end(); ++it) {
        if ((*it)->getWidth() >= tab_width) {
            (*it)->setWidth(tab_width + ((off-- > 0) ? 1 : 0));
        }
    }
}

/**
 * This is called when tabs in the decor are using less space than
 * width min, grow tabs that are smaller than the average.
 */
void
PDecor::calcTabsWidthAsymetricGrow(uint width_avail, uint tab_width)
{
    // FIXME: Implement growing of asymetric tabs.
}
