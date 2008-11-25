//
// ManagerWindows.cc for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Config.hh"
#include "ActionHandler.hh"
#include "Atoms.hh"
#include "ManagerWindows.hh"
#include "Util.hh"

#include <string>

using std::string;

// Static initializers
const string HintWO::WM_NAME = string("pekwm");
HintWO *HintWO::_instance = 0;

const unsigned long RootWO::EXPECTED_DESKTOP_NAMES_LENGTH = 256;

/**
 * Hint window constructor, creates window and sets supported
 * protocols.
 */
HintWO::HintWO(Display *dpy, Window root)
    : PWinObj(dpy)
{
    if (_instance) {
        throw string("trying to create HintWO which is already created.");
    }
    _instance = this;

    _type = WO_SCREEN_HINT;
    _layer = LAYER_NONE;
    _sticky = true; // Hack, do not map/unmap this window
    _iconified = true; // Hack, to be ignored when placing

    // Create window
    _window = XCreateSimpleWindow(_dpy, root, -200, -200, 5, 5, 0, 0, 0);

    // Remove override redirect from window
    XSetWindowAttributes attr;
    attr.override_redirect = True;
    XChangeWindowAttributes(_dpy, _window, CWOverrideRedirect, &attr);

    // Set hints not being updated
    AtomUtil::setString(_window, EwmhAtoms::instance()->getAtom(NET_WM_NAME), WM_NAME);
    AtomUtil::setWindow(_window, EwmhAtoms::instance()->getAtom(NET_SUPPORTING_WM_CHECK), _window);
}

/**
 * Hint WO destructor, destroy hint window.
 */
HintWO::~HintWO(void)
{
    XDestroyWindow(PScreen::instance()->getDpy(), _window);
    _instance = 0;
}

/**
 * Root window constructor, reads geometry and sets basic atoms.
 */
RootWO::RootWO(Display *dpy, Window root)
    : PWinObj(dpy)
{
    _type = WO_SCREEN_ROOT;
    _layer = LAYER_NONE;
    _mapped = true;

    _window = root;
    _gm.width = PScreen::instance()->getWidth();
    _gm.height = PScreen::instance()->getHeight();

    // Convenience
    EwmhAtoms *atoms = EwmhAtoms::instance();
    Atom *supported_atoms = atoms->getAtomArray();

    // Set hits on the hint window, these are not updated so they are
    // set in the constructor.
    AtomUtil::setLong(_window, atoms->getAtom(NET_WM_PID), static_cast<long>(getpid()));
    AtomUtil::setString(_window, IcccmAtoms::instance()->getAtom(WM_CLIENT_MACHINE), Util::getHostname());

    AtomUtil::setWindow(_window, atoms->getAtom(NET_SUPPORTING_WM_CHECK), HintWO::instance()->getWindow());
    AtomUtil::setAtoms(_window, atoms->getAtom(NET_SUPPORTED), supported_atoms, atoms->size());
    AtomUtil::setLong(_window, atoms->getAtom(NET_NUMBER_OF_DESKTOPS), Config::instance()->getWorkspaces());
    AtomUtil::setLong(_window, atoms->getAtom(NET_CURRENT_DESKTOP), 0);

    long desktop_geometry[2] = { _gm.width, _gm.height };
    AtomUtil::setLongs(_window, atoms->getAtom(NET_DESKTOP_GEOMETRY), desktop_geometry, 2);

    delete [] supported_atoms;
    
    woListAdd(this);
    _wo_map[_window] = this;
}

/**
 * Root window destructor, clears atoms set.
 */
RootWO::~RootWO(void)
{
    // Remove atoms, PID will not be valid on shutdown.
    AtomUtil::unsetProperty(_window, EwmhAtoms::instance()->getAtom(NET_WM_PID));
    AtomUtil::unsetProperty(_window, IcccmAtoms::instance()->getAtom(WM_CLIENT_MACHINE));

    _wo_map.erase(_window);
    woListRemove(this);
}

/**
 * Button press event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleButtonPress(XButtonEvent *ev)
{
    return ActionHandler::findMouseAction(ev->button, ev->state, MOUSE_EVENT_PRESS,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

/**
 * Button release event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleButtonRelease(XButtonEvent *ev)
{
    MouseEventType mb = MOUSE_EVENT_RELEASE;

    // first we check if it's a double click
    if (PScreen::instance()->isDoubleClick(ev->window, ev->button - 1, ev->time,
                                           Config::instance()->getDoubleClickTime())) {
        PScreen::instance()->setLastClickID(ev->window);
        PScreen::instance()->setLastClickTime(ev->button - 1, 0);

        mb = MOUSE_EVENT_DOUBLE;

    } else {
        PScreen::instance()->setLastClickID(ev->window);
        PScreen::instance()->setLastClickTime(ev->button - 1, ev->time);
    }

    return ActionHandler::findMouseAction(ev->button, ev->state, mb,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

/**
 * Motion event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleMotionEvent(XMotionEvent *ev)
{
    unsigned int button = PScreen::instance()->getButtonFromState(ev->state);

    return ActionHandler::findMouseAction(button, ev->state, MOUSE_EVENT_MOTION,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

/**
 * Enter event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleEnterEvent(XCrossingEvent *ev)
{
    return ActionHandler::findMouseAction(BUTTON_ANY, ev->state, MOUSE_EVENT_ENTER,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

/**
 * Leave event handler, gets actions from root list.
 */
ActionEvent*
RootWO::handleLeaveEvent(XCrossingEvent *ev)
{
    return ActionHandler::findMouseAction(BUTTON_ANY, ev->state, MOUSE_EVENT_LEAVE,
                                          Config::instance()->getMouseActionList(MOUSE_ACTION_LIST_ROOT));
}

/**
 * Update _NET_WORKAREA property.
 *
 * @param workarea Geometry with work area.
 */
void
RootWO::setEwmhWorkarea(const Geometry &workarea)
{
    long workarea_array[4] = { workarea.x, workarea.y, workarea.width, workarea.height };
    AtomUtil::setLongs(_window, EwmhAtoms::instance()->getAtom(NET_WORKAREA), workarea_array, 4);
}

/**
 * Update _NET_ACTIVE_WINDOW property.
 *
 * @param win Window to set as active window.
 */
void
RootWO::setEwmhActiveWindow(Window win)
{
    AtomUtil::setWindow(PScreen::instance()->getRoot(), EwmhAtoms::instance()->getAtom(NET_ACTIVE_WINDOW), win);
}

/**
 * Reads the _NET_DESKTOP_NAMES hint and sets the workspaces names accordingly.
 */
void
RootWO::readEwmhDesktopNames(void)
{
    uchar *data;
    ulong data_length;
    if (AtomUtil::getProperty(PScreen::instance()->getRoot(), 
                              EwmhAtoms::instance()->getAtom(NET_DESKTOP_NAMES),
                              EwmhAtoms::instance()->getAtom(UTF8_STRING),
                              EXPECTED_DESKTOP_NAMES_LENGTH, &data, &data_length)) {
        Config::instance()->setDesktopNamesUTF8(reinterpret_cast<char *>(data), data_length);

        XFree(data);
    }
}

/**
 * Update _NET_DESKTOP_NAMES property on the root window.
 */
void
RootWO::setEwmhDesktopNames(void)
{
    unsigned char *desktopnames = 0;
    unsigned int length = 0;
    Config::instance()->getDesktopNamesUTF8(&desktopnames, &length);

    if (desktopnames) {
        AtomUtil::setUtf8StringArray(PScreen::instance()->getRoot(),
                                     EwmhAtoms::instance()->getAtom(NET_DESKTOP_NAMES), desktopnames, length);
        delete [] desktopnames;
    }
}


/**
 * Edge window constructor, create window, setup strut and register
 * window.
 */
EdgeWO::EdgeWO(Display *dpy, Window root, EdgeType edge, bool set_strut)
    : PWinObj(dpy),
      _edge(edge)
{
    _type = WO_SCREEN_EDGE;
    _layer = LAYER_NONE; // hack, goes over LAYER_MENU
    _sticky = true; // don't map/unmap
    _iconified = true; // hack, to be ignored when placing
    _focusable = false; // focusing input only windows crashes X

    XSetWindowAttributes sattr;
    sattr.override_redirect = True;
    sattr.event_mask = EnterWindowMask|LeaveWindowMask|ButtonPressMask|ButtonReleaseMask;

    _window = XCreateWindow(_dpy, root, 0, 0, 1, 1, 0, CopyFromParent, InputOnly, CopyFromParent,
                            CWOverrideRedirect|CWEventMask, &sattr);

    configureStrut(set_strut);
    PScreen::instance()->addStrut(&_strut);

    woListAdd(this);
    _wo_map[_window] = this;
}

/**
 * Edge window destructor, remove strut and destroy window resources.
 */
EdgeWO::~EdgeWO(void)
{
    PScreen::instance()->removeStrut(&_strut);
    _wo_map.erase(_window);
    woListRemove(this);

    XDestroyWindow(_dpy, _window);
}

/**
 * Configure strut on edge window.
 *
 * @param set_strut If true, set actual values on the strut, false sets all to 0.
 */
void
EdgeWO::configureStrut(bool set_strut)
{
    // Reset value, on strut to zero.
    _strut.left = _strut.right = _strut.top = _strut.bottom = 0;

    // Set strut if requested.
    if (set_strut) {
        switch (_edge) {
        case SCREEN_EDGE_TOP:
            _strut.top = _gm.height;
            break;
        case SCREEN_EDGE_BOTTOM:
            _strut.bottom = _gm.height;
            break;
        case SCREEN_EDGE_LEFT:
            _strut.left = _gm.width;
            break;
        case SCREEN_EDGE_RIGHT:
            _strut.right = _gm.width;
            break;
        case SCREEN_EDGE_NO:
        default:
            // do nothing
            break;
        }
    }
}

/**
 * Edge version of mapped window, makes sure the iconified state is
 * set at all times in order to avoid counting the edge windows when
 * snapping windows etc.
 */
void
EdgeWO::mapWindow(void)
{
    if (_mapped) {
        return;
    }

    PWinObj::mapWindow();
    _iconified = true;
}

/**
 * Enter event handler, gets actions from EdgeList on _edge.
 */
ActionEvent*
EdgeWO::handleEnterEvent(XCrossingEvent *ev)
{
    return ActionHandler::findMouseAction(BUTTON_ANY, ev->state, MOUSE_EVENT_ENTER,
                                          Config::instance()->getEdgeListFromPosition(_edge));
}

/**
 * Button press event handler, gets actions from EdgeList on _edge.
 */
ActionEvent*
EdgeWO::handleButtonPress(XButtonEvent *ev)
{
    return ActionHandler::findMouseAction(ev->button, ev->state, MOUSE_EVENT_PRESS,
                                          Config::instance()->getEdgeListFromPosition(_edge));
}

/**
 * Button release event handler, gets actions from EdgeList on _edge.
 */
ActionEvent*
EdgeWO::handleButtonRelease(XButtonEvent *ev)
{
    MouseEventType mb = MOUSE_EVENT_RELEASE;

    // first we check if it's a double click
    if (PScreen::instance()->isDoubleClick(ev->window, ev->button - 1, ev->time,
                                           Config::instance()->getDoubleClickTime())) {
        PScreen::instance()->setLastClickID(ev->window);
        PScreen::instance()->setLastClickTime(ev->button - 1, 0);

        mb = MOUSE_EVENT_DOUBLE;
    } else {
        PScreen::instance()->setLastClickID(ev->window);
        PScreen::instance()->setLastClickTime(ev->button - 1, ev->time);
    }

    return ActionHandler::findMouseAction(ev->button, ev->state, mb,
                                          Config::instance()->getEdgeListFromPosition(_edge));
}
