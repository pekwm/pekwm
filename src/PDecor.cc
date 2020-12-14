//
// PDecor.cc for pekwm
// Copyright (C) 2004-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <functional>
#include <algorithm>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
}

#include "Debug.hh"
#include "Compat.hh"
#include "Config.hh"
#include "PWinObj.hh"
#include "PFont.hh"
#include "PDecor.hh"
#include "x11.hh"
#include "PTexture.hh"
#include "PTexturePlain.hh" // PTextureSolid
#include "ActionHandler.hh"
#include "StatusWindow.hh"
#include "KeyGrabber.hh"
#include "Theme.hh"
#include "Workspaces.hh"

using std::find;
using std::map;
using std::string;
using std::vector;

// PDecor::Button

//! @brief PDecor::Button constructor
PDecor::Button::Button(PWinObj *parent, Theme::PDecorButtonData *data, uint width, uint height)
  : PWinObj(true),
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

    _bg = X11::createPixmap(_gm.width, _gm.height);

    setBackgroundPixmap(_bg);
    setState(_state);
}

//! @brief PDecor::Button destructor
PDecor::Button::~Button(void)
{
    XDestroyWindow(X11::getDpy(), _window);
    X11::freePixmap(_bg);
}

//! @brief Searches the PDecorButtonData for an action matching ev
ActionEvent*
PDecor::Button::findAction(XButtonEvent *ev)
{
    vector<ActionEvent>::iterator it(_data->begin());
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
        if (_data->setShape()) {
            // Get shape mask
            bool need_free;
            Pixmap shape = _data->getTexture(state)->getMask(0, 0, need_free);
            if (shape != None) {
                X11::shapeSetMask(_window, ShapeBounding, shape);
                if (need_free) {
                    X11::freePixmap(shape);
                }
            } else {
                XRectangle rect = {0 /* x */, 0 /* y */, static_cast<short uint>(_gm.width), static_cast<short uint>(_gm.height) };
                X11::shapeSetRect(_window, &rect);
            }
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

vector<PDecor*> PDecor::_pdecors;

//! @brief PDecor constructor
//! @param dpy Display
//! @param theme Theme
//! @param decor_name String, if not DEFAULT_DECOR_NAME sets _decor_name_saved
PDecor::PDecor(Theme *theme,
               const std::string &decor_name, const Window child_window)
    : PWinObj(true),
      _theme(theme), _decor_name(decor_name),
      _child(0), _button(0), _button_press_win(None),
      _pointer_x(0), _pointer_y(0),
      _click_x(0), _click_y(0),
      _decor_cfg_child_move_overloaded(false),
      _decor_cfg_bpr_replay_pointer(false),
      _decor_cfg_bpr_al_child(MOUSE_ACTION_LIST_CHILD_OTHER),
      _decor_cfg_bpr_al_title(MOUSE_ACTION_LIST_TITLE_OTHER),
      _maximized_vert(false), _maximized_horz(false),
      _fullscreen(false), _skip(0), _data(0), 
      _border(true), _titlebar(true), _shaded(false), _attention(0),
      _need_shape(false),
      _real_height(1),
      _title_wo(true),
      _title_active(0), _titles_left(0), _titles_right(1)
{
    if (_decor_name.empty()) {
        _decor_name = getDecorName();
    }
    if (_decor_name != PDecor::DEFAULT_DECOR_NAME) {
        _decor_name_saved = _decor_name;
    }

    // we be reset in loadDecor later on, inlines using the _data used before
    // loadDecor needs this though
    _data = _theme->getPDecorData(_decor_name);
    if (! _data) {
        _data = _theme->getPDecorData(DEFAULT_DECOR_NAME);
    }

    CreateWindowParams window_params;
    createParentWindow(window_params, child_window);
    createTitle(window_params);
    createBorder(window_params);

    // sets buttons etc up
    loadDecor();

    // map title and border windows
    XMapSubwindows(X11::getDpy(), _window);

    _pdecors.push_back(this);
}

/**
 * Create window attributes
 */
void
PDecor::createParentWindow(CreateWindowParams &params, Window child_window)
{
    params.mask = CWOverrideRedirect|CWEventMask|CWBorderPixel|CWBackPixel;
    params.depth = CopyFromParent;
    params.visual = CopyFromParent;
    params.attr.override_redirect = True;
    params.attr.border_pixel = X11::getBlackPixel();
    params.attr.background_pixel = X11::getBlackPixel();

    if (child_window != None) {
        XWindowAttributes attr;
        if (X11::getWindowAttributes(child_window, &attr)
              && 32 != X11::getDepth()
              && attr.depth == 32) {
            params.mask |= CWColormap;
            params.depth = attr.depth;
            params.visual = attr.visual;
            params.attr.colormap = XCreateColormap(X11::getDpy(),
                                                   X11::getRoot(),
                                                   params.visual, AllocNone);
        }
    }
    params.attr.event_mask = ButtonPressMask|ButtonReleaseMask|
        ButtonMotionMask|EnterWindowMask|SubstructureRedirectMask|
        SubstructureNotifyMask;
    _window = XCreateWindow(X11::getDpy(), X11::getRoot(),
                            _gm.x, _gm.y, _gm.width, _gm.height, 0,
                            params.depth, InputOutput, params.visual,
                            params.mask, &params.attr);

    if (params.mask & CWColormap) {
        XFreeColormap(X11::getDpy(), params.attr.colormap);
        params.depth = X11::getDepth();
        params.visual = X11::getVisual();
        params.attr.colormap = X11::getColormap();
    }
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
    _title_wo.mapWindow();
}

/**
 * Create border windows
 */
void
PDecor::createBorder(CreateWindowParams &params)
{
    params.attr.event_mask = ButtonPressMask|ButtonReleaseMask|
        ButtonMotionMask|EnterWindowMask;

    for (uint i = 0; i < BORDER_NO_POS; ++i) {
        params.attr.cursor = X11::getCursor(CursorType(i));

        _border_win[i] =
            XCreateWindow(X11::getDpy(), _window, -1, -1, 1, 1, 0,
                          params.depth, InputOutput, params.visual,
                          params.mask|CWCursor, &params.attr);
        addChildWindow(_border_win[i]);
    }
}

//! @brief PDecor destructor
PDecor::~PDecor(void)
{
    _pdecors.erase(std::remove(_pdecors.begin(), _pdecors.end(), this), _pdecors.end());

    while (! _children.empty()) {
        removeChild(_children.back(), false); // Don't call delete this.
    }

    // Make things look smoother, buttons will be noticed as deleted
    // otherwise. Using X call directly to avoid re-drawing and other
    // special features not required when removing the window.
    X11::unmapWindow(_window);

    // free buttons
    unloadDecor();

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
        for_each(_children.begin(), _children.end(),
                 std::mem_fn(&PWinObj::mapWindow));
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
            for_each(_children.begin(), _children.end(),
                     std::mem_fn(&PWinObj::iconify));
        } else {
            for_each(_children.begin(), _children.end(),
                     std::mem_fn(&PWinObj::unmapWindow));
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

    // Set and apply shape on window, all parts of the border can now
    // be shaped.
    setBorderShape();
    applyBorderShape();

    renderTitle();
    renderBorder();
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
        // _child and not all members of _children, because activateChild.*()
        // does it itself.
        alignChild(_child);
    }

    // Place and resize title and border
    resizeTitle();
    placeBorder();

    // Apply shape on window, all parts of the border can now be shaped.
    setBorderShape();
    applyBorderShape();

    renderTitle();
    renderBorder();
}

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

void
PDecor::setWorkspace(uint workspace)
{
    if (workspace != NET_WM_STICKY_WINDOW) {
        if (workspace >= Workspaces::size()) {
            LOG("this == " << this << " workspace == " << workspace
                << " >= number of workspaces == " << Workspaces::size());
            workspace = Workspaces::size() - 1;
        }
        _workspace = workspace;
    }

    vector<PWinObj*>::const_iterator it(_children.begin());
    for (; it != _children.end(); ++it) {
        (*it)->setWorkspace(workspace);
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
        LOG("this == " << this << " - reverting to root");
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
    vector<ActionEvent> *actions = 0;

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
    vector<ActionEvent> *actions = 0;
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
        LOG("this == decor (" << this << ")");
        return;
    }

    vector<PWinObj*>::const_iterator it(decor->begin());
    for (; it != decor->end(); ++it) {
        addChild(*it);
        (*it)->setWorkspace(_workspace);
    }

    decor->_children.clear();
    delete decor;
}

//! @return True on decor change, False if decor unchanged
bool
PDecor::updateDecor(void)
{
    string name = getDecorName();
    if (! _decor_name_saved.empty()) {
        _decor_name_saved = name;
        return false;
    }

    if (_decor_name == name) {
        return false;
    }

    _decor_name = name;
    loadDecor();
    return true;
}

//! @brief Sets override decor name
void
PDecor::setDecorOverride(StateAction sa, const std::string &name)
{
    if (sa == STATE_SET ||
            (sa == STATE_TOGGLE && _decor_name_saved.empty())) {
        if (_decor_name_saved.empty()) {
            _decor_name_saved = _decor_name;
        }
        if (_decor_name != name) {
            _decor_name = name;
            loadDecor();
        }
    } else if (! _decor_name_saved.empty()) {
        _decor_name = _decor_name_saved;
        _decor_name_saved.clear();
        loadDecor();
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
    LOG_IF(!_data, "_data == 0");

    // Load decor.
    vector<Theme::PDecorButtonData*>::const_iterator b_it(_data->buttonBegin());
    for (; b_it != _data->buttonEnd(); ++b_it) {
        uint width = std::max(static_cast<uint>(1), (*b_it)->getWidth() ? (*b_it)->getWidth() : getTitleHeight());
        uint height = std::max(static_cast<uint>(1), (*b_it)->getHeight() ? (*b_it)->getHeight() : getTitleHeight());

        _buttons.push_back(new PDecor::Button(&_title_wo, *b_it, width, height));
        _buttons.back()->mapWindow();
        addChildWindow(_buttons.back()->getWindow());
    }

    if (! _data->getTitleHeight() && ! _data->isTitleHeightAdapt()) {
        _title_wo.unmapWindow();
    } else {
        _title_wo.mapWindow();
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
    vector<PWinObj*>::const_iterator c_it(_children.begin());
    for (; c_it != _children.end(); ++c_it) {
        alignChild(*c_it);
    }

    // Make sure it gets rendered correctly.
    _focused = ! _focused;
    setFocused(! _focused);

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

    vector<PDecor::Button*>::const_iterator it(_buttons.begin());
    for (; it != _buttons.end(); ++it) {
        removeChildWindow((*it)->getWindow());
        delete *it;
    }
    _buttons.clear();
}

PDecor::Button*
PDecor::findButton(Window win)
{
    vector<PDecor::Button*>::const_iterator it(_buttons.begin());
    for (; it != _buttons.end(); ++it) {
        if (**it == win) {
            return *it;
        }
    }

    return 0;
}

PWinObj*
PDecor::getChildFromPos(int x)
{
    if (_children.empty() || _children.size() != _titles.size()) {
        return 0;
    }
    if (_children.size() == 1)
        return _children.front();

    if (x < static_cast<int>(_titles_left)) {
        return _children.front();
    } else if (x > static_cast<int>(_title_wo.getWidth() - _titles_right)) {
        return _children.back();
    }

    PTexture *t_sep = _data->getTextureSeparator(getFocusedState(false));
    uint sepw = t_sep?t_sep->getWidth():0;

    uint pos = _titles_left, xx = x;
    for (uint i = 0; i < _titles.size(); ++i) {
        if (xx >= pos && xx <= pos + _titles[i]->getWidth() + sepw) {
            return _children[i];
        }
        pos += _titles[i]->getWidth() + sepw;
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
    if (! updateDecor() && _child) {
        resizeChild(_child->getWidth(), _child->getHeight());
    }
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
    if (! updateDecor() && _child) {
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
PDecor::addChild(PWinObj *child, vector<PWinObj*>::iterator *it)
{
    child->reparent(this, borderLeft(), borderTop() + getTitleHeight());
    if (it == 0) {
        _children.push_back(child);
    } else {
        _children.insert(*it, child);
    }

    updatedChildOrder();

    // Sync focused state if it is the first child, the child will be
    // activated later on. If there are children here already fit the
    // child into the decor.
    if (_children.size() == 1) {
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

    vector<PWinObj*>::iterator it(find(_children.begin(), _children.end(), child));
    if (it == _children.end()) {
        LOG("this == " << this << " child == " << child
            << " - child not in _child_list, bailing out");
        return;
    }

    it = _children.erase(it);

    if (! _children.empty()) {
        if (_child == child) {
            if (it == _children.end()) {
                --it;
            }

            activateChild(*it);
        }

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

void
PDecor::getDecorInfo(wchar_t *buf, uint size)
{
    swprintf(buf, size, L"%dx%d+%d+%d", _gm.width, _gm.height, _gm.x, _gm.y);
}

void
PDecor::activateChildNum(uint num)
{
    if (num >= _children.size()) {
        LOG("this == " << this << " num == " << num
             << " >= _children.size() == " << _children.size());
        return;
    }

    activateChild(_children[num]);
}

void
PDecor::activateChildRel(int off)
{
    vector<PWinObj*>::size_type cur=0, size = _children.size();
    for (; cur < size; ++cur) {
        if (_child == _children[cur]) {
            break;
        }
    }

    if (cur == size) {
        _child = _children.front();
        cur = 0;
    }

    off = (off+signed(cur))%signed(size);
    if (off < 0) {
        off += size;
    }
    activateChild(_children[off]);
}

//! @brief Moves the current child off steps
//! @param off - for left and + for right
void
PDecor::moveChildRel(int off)
{
    vector<PWinObj*>::size_type idx=0, size = _children.size();
    for (; idx < size; ++idx) {
        if (_child == _children[idx]) {
            break;
        }
    }

    if (idx == size) {
        if (_children.empty()) {
            ERR("_children is empty! off == " << off << " Please report.");
            return;
        }
        _child = _children.front();
        idx = 0;
    }

    off = (off+idx)%size;
    if (off < 0) {
        off += size;
    }

    _children.erase(std::remove(_children.begin(), _children.end(), _child), _children.end());
    _children.insert(_children.begin()+off, _child);
    updatedChildOrder();
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

    if (! X11::grabPointer(X11::getRoot(), ButtonMotionMask|ButtonReleaseMask, CURSOR_MOVE)) {
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

    if (Config::instance()->isShowStatusWindow()) {
        sw->drawGeometry(_gm, center_on_root);
        sw->mapWindowRaised();
        sw->drawGeometry(_gm, center_on_root);
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
                XMoveWindow(X11::getDpy(), _window, _gm.x, _gm.y);
            }

            edge = doMoveEdgeFind(e.xmotion.x_root, e.xmotion.y_root);
            if (edge != SCREEN_EDGE_NO) {
                doMoveEdgeAction(&e.xmotion, edge);
            }

            if (Config::instance()->isShowStatusWindow()) {
                sw->drawGeometry(_gm, center_on_root);
            }

            break;
        case ButtonRelease:
            exit = true;
            break;
        }
    }

    move(_gm.x, _gm.y); // update child

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

    if (! X11::grabPointer(X11::getRoot(), NoEventMask, CURSOR_MOVE)) {
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
    vector<Action>::iterator it;

    bool center_on_root = Config::instance()->isShowStatusWindowOnRoot();    
    if (Config::instance()->isShowStatusWindow()) {
        sw->drawGeometry(_gm, center_on_root);
        sw->mapWindowRaised();
        sw->drawGeometry(_gm, center_on_root);
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

                if (Config::instance()->isShowStatusWindow()) {
                    sw->drawGeometry(_gm, center_on_root);
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

/**
 * Returns the name of the decor for the current titlebar/border state.
 */
std::string
PDecor::getDecorName(void)
{
    if (_attention) {
        return DEFAULT_DECOR_NAME_ATTENTION;
    }
    if (_titlebar && _border) {
        return DEFAULT_DECOR_NAME;
    }
    if (_border) {
        return DEFAULT_DECOR_NAME_TITLEBARLESS;
    }
    return DEFAULT_DECOR_NAME_BORDERLESS;
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

    if (_data->getTitleWidthMin()) {
        resizeTitle();
        applyBorderShape(); // update title shape
    } else {
        calcTabsWidth();
    }

    PTexture *t_sep = _data->getTextureSeparator(getFocusedState(false));
    Pixmap title_bg = X11::createPixmap(_title_wo.getWidth(), _title_wo.getHeight());
    // Render main background on pixmap
    _data->getTextureMain(getFocusedState(false))->render(title_bg, 0, 0,
                                                          _title_wo.getWidth(), _title_wo.getHeight());

    PFont *font;
    bool sel; // Current tab selected flag
    uint x = _titles_left; // Position
    uint pad_horiz =  _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT); // Amount of horizontal padding

    uint size = _titles.size();
    for (uint i = 0; i < size; ++i) {
        sel = (_title_active == i);

        // render tab
        _data->getTextureTab(getFocusedState(sel))->render(title_bg, x, 0,
                                                           _titles[i]->getWidth(),
                                                           _title_wo.getHeight());

        font = getFont(getFocusedState(sel));
        font->setColor(_data->getFontColor(getFocusedState(sel)));

        PFont::TrimType trim = PFont::FONT_TRIM_MIDDLE;
        if (_titles[i]->isCustom() || _titles[i]->isUserSet()) {
            trim = PFont::FONT_TRIM_END;
        }

        font->draw(title_bg,
                   x + _data->getPad(PAD_LEFT), // X position
                   _data->getPad(PAD_UP), // Y position
                   _titles[i]->getVisible(), 0, // Text and max chars
                   _titles[i]->getWidth() - pad_horiz, // Available width
                   trim); // Type of trim

        // move to next tab (or separator if any)
        x += _titles[i]->getWidth();

        // draw separator
        if (size > 1 && i < size - 1) {
            t_sep->render(title_bg, x, 0, 0, 0);
            x += t_sep->getWidth();
        }
    }

    _title_wo.setBackgroundPixmap(title_bg);
    _title_wo.clear();
    X11::freePixmap(title_bg);
}

void
PDecor::renderButtons(void)
{
    vector<PDecor::Button*>::const_iterator it(_buttons.begin());
    for (; it != _buttons.end(); ++it) {
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

    PTexture *tex;
    FocusedState state = getFocusedState(false);
    uint width, height;
    Pixmap pix = None;

    for (int i=0; i < BORDER_NO_POS; ++i) {
        tex = _data->getBorderTexture(state, static_cast<BorderPosition>(i));

        // Solid texture, get the color and set as bg, no need to render pixmap
        if (tex->getType() == PTexture::TYPE_SOLID) {
            XSetWindowBackground(X11::getDpy(), _border_win[i],
                                 static_cast<PTextureSolid*>(tex)->getColor()->pixel);
        } else {
            // not a solid texture, get pixmap, render, set as bg
            getBorderSize(static_cast<BorderPosition>(i), width, height);

            if (width > 0 && height > 0) {
                pix = X11::createPixmap(width, height);
                tex->render(pix, 0, 0, width, height);
                XSetWindowBackgroundPixmap(X11::getDpy(), _border_win[i], pix);
                X11::freePixmap(pix);
            }
        }

        XClearWindow(X11::getDpy(), _border_win[i]);
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
    PTexture *tex;
    FocusedState state = getFocusedState(false);

    XRectangle rect = {0, 0, 0, 0};

    for (int i=0; i < BORDER_NO_POS; ++i) {
        // Get the size of the border at position
        getBorderSize(static_cast<BorderPosition>(i), width, height);

        tex = _data->getBorderTexture(state, static_cast<BorderPosition>(i));
        pix = tex->getMask(width, height, do_free);

        if (pix != None) {
            _need_shape = true;
            X11::shapeSetMask(_border_win[i], ShapeBounding, pix);
            if (do_free) {
                X11::freePixmap(pix);
            }
        } else {
            rect.width = width;
            rect.height = height;
            X11::shapeSetRect(_border_win[i], &rect);
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

    vector<PDecor::Button*>::const_iterator it(_buttons.begin());
    for (; it != _buttons.end(); ++it) {
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

#ifdef HAVE_SHAPE
/**
 * Apply shaping on the window based on the shape of the client and
 * the borders.
 */
void
PDecor::applyBorderShape(int kind)
{
    bool client_shape = _child?_child->hasShapeRegion(kind):false;
    XRectangle rect_square;
    rect_square.x = 0;
    rect_square.y = 0;
    rect_square.width = _gm.width;
    rect_square.height = _gm.height;

    if ((_need_shape && kind == ShapeBounding) || client_shape) {
        // if we have tab min == 0 then we have full width title and place the
        // border ontop, else we put the border under the title
        uint bt_off = (_data->getTitleWidthMin() > 0) ? getTitleHeight() : 0;

        Window shape;
        shape = XCreateSimpleWindow(X11::getDpy(), X11::getRoot(),
                                    0, 0, _gm.width, _gm.height, 0, 0, 0);

        if (_child && ! _shaded) {
            X11::shapeCombine(shape, kind,
                              borderLeft(), borderTop() + getTitleHeight(),
                              _child->getWindow(), ShapeSet);
        }

        // Apply border shape. Need to be carefull wheter or not to include it.
        if (_border
                && ! (_shaded && bt_off) // Shaded in non-full-width mode removes border.
                && ! client_shape) { // Shaped clients should appear bordeless.
            // top
            if (borderTop() > 0) {
                X11::shapeCombine(shape, kind, 0, bt_off,
                                  _border_win[BORDER_TOP_LEFT], ShapeUnion);

                X11::shapeCombine(shape, kind, borderTopLeft(), bt_off,
                                  _border_win[BORDER_TOP], ShapeUnion);

                X11::shapeCombine(shape, kind, _gm.width - borderTopRight(),
                                  bt_off, _border_win[BORDER_TOP_RIGHT], ShapeUnion);
            }

            bool use_bt_off = bt_off || borderTop();
            // Left border
            if (borderLeft() > 0) {
                X11::shapeCombine(shape, kind, 0,
                                  use_bt_off ? bt_off + borderTopLeftHeight() : getTitleHeight(),
                                  _border_win[BORDER_LEFT], ShapeUnion);
            }

            // Right border
            if (borderRight() > 0) {
                X11::shapeCombine(shape, kind, _gm.width - borderRight(),
                                  use_bt_off ? bt_off + borderTopRightHeight() : getTitleHeight(),
                                  _border_win[BORDER_RIGHT], ShapeUnion);
            }

            // bottom
            if (borderBottom() > 0) {
                X11::shapeCombine(shape, kind,
                                  0, _gm.height - borderBottomLeftHeight(),
                                  _border_win[BORDER_BOTTOM_LEFT], ShapeUnion);

                X11::shapeCombine(shape, kind,
                                  borderBottomLeft(), _gm.height - borderBottom(),
                                  _border_win[BORDER_BOTTOM], ShapeUnion);

                X11::shapeCombine(shape, kind,
                                  _gm.width - borderBottomRight(),
                                  _gm.height - borderBottomRightHeight(),
                                  _border_win[BORDER_BOTTOM_RIGHT],
                                  ShapeUnion);
            }
        }
        if (_titlebar) {
            // apply title shape
            X11::shapeCombine(shape, kind, _title_wo.getX(), _title_wo.getY(),
                              _title_wo.getWindow(), ShapeUnion);
        }

        // The borders might extend beyond the window area (causing
        // artifacts under xcompmgr), so we cut the shape down to the
        // original window size.
        X11::shapeIntersectRect(shape, &rect_square);

        // Apply the shape mask to the window
        X11::shapeCombine(_window, kind, 0, 0, shape, ShapeSet);

        XDestroyWindow(X11::getDpy(), shape);
    } else {
        // Reinstate default region
        X11::shapeSetMask(_window, kind, None);
    }
}
#endif // HAVE_SHAPE

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
        LOG("this == " << this << " - invalid border position");
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
            vector<PDecor::TitleItem*>::const_iterator it(_titles.begin());
            for (; it != _titles.end(); ++it) {
                width = font->getWidth((*it)->getVisible());
                if (width > width_max) {
                    width_max = width;
                }
            }

            width = width_max + _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT);
            width *= _titles.size();

        } else {
            // Asymetric mode, get individual widths
            vector<PDecor::TitleItem*>::const_iterator it(_titles.begin());
            for (; it != _titles.end(); ++it) {
                width += font->getWidth((*it)->getVisible())
                    + _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT);
            }
        }

        // Add width of separators and buttons
        width += (_titles.size() - 1)
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
    if (! _titles.size()) {
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
    uint sep_width = (_titles.size() - 1)
        * _data->getTextureSeparator(getFocusedState(false))->getWidth();
    if (width_avail > sep_width) {
        width_avail -= sep_width;
    }

    tab_width = width_avail / _titles.size();
    off = width_avail % _titles.size();
}

//! @brief Calculate tab width symetric, sets up _titles widths.
void
PDecor::calcTabsWidthSymetric(void)
{
    int off;
    uint width_avail, tab_width;
    calcTabsGetAvailAndTabWidth(width_avail, tab_width, off);

    // Assign width to elements
    vector<PDecor::TitleItem*>::const_iterator it(_titles.begin());
    for (; it != _titles.end(); ++it) {
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
    vector<PDecor::TitleItem*>::const_iterator it(_titles.begin());
    for (; it != _titles.end(); ++it) {
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
    uint tabs_left = _titles.size();
    vector<PDecor::TitleItem*>::const_iterator it(_titles.begin());
    for (; it != _titles.end(); ++it) {
        if ((*it)->getWidth() < tab_width) {
            // 3. Add width of tabs requiring less space to the average.
            tabs_left--;
            width_avail -= (*it)->getWidth();
        }
    }

    // 4. Re-assign width equally to tabs using more than average
    tab_width = width_avail / tabs_left;
    uint off = width_avail % tabs_left;
    for (it = _titles.begin(); it != _titles.end(); ++it) {
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
