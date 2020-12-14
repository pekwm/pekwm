//
// ManagerWindows.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _MANAGER_WINDOWS_H_
#define _MANAGER_WINDOWS_H_

#include "config.h"

#include "pekwm.hh"

#include "PWinObj.hh"

#include <string>

/**
 * Window for handling of EWMH hints, sets supported attributes etc.
 */
class HintWO : public PWinObj
{
public:
    HintWO(Window root, bool replace);
    virtual ~HintWO(void);

    inline static HintWO *instance(void) { return _instance; }

private:
    Time getTime(void);
    bool claimDisplay(bool replace);
    bool claimDisplayWait(Window session_owner);
    void claimDisplayOwner(Window session_atom, Time timestamp);

private:
    static const std::string WM_NAME; /**< Name of the window manager, that is pekwm. */
    static HintWO *_instance; /**< Singleton HintWO pointer. */
    static const unsigned int DISPLAY_WAIT; /**< Max wait time for previous WM. */
};

/**
 * Window object representing the Root window, handles actions and
 * sets atoms on the window.
 */
class RootWO : public PWinObj
{
public:
    RootWO(Window root);
    virtual ~RootWO(void);

    /** Resize root window, does no actual resizing but updates the
        geometry of the window. */
    virtual void resize(uint width, uint height) {
        _gm.width = width;
        _gm.height = height;
    }

    virtual ActionEvent *handleButtonPress(XButtonEvent *ev);
    virtual ActionEvent *handleButtonRelease(XButtonEvent *ev);
    virtual ActionEvent *handleMotionEvent(XMotionEvent *ev);
    virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);
    virtual ActionEvent *handleLeaveEvent(XCrossingEvent *ev);

    void setEwmhWorkarea(const Geometry &workarea);
    void setEwmhActiveWindow(Window win);
    void readEwmhDesktopNames(void);
    void setEwmhDesktopNames(void);
    void setEwmhDesktopLayout(void);

private:
    static const unsigned long EVENT_MASK; /**< Root window event mask. */
    static const unsigned long EXPECTED_DESKTOP_NAMES_LENGTH; /**< Expected length of desktop hint. */
};

/**
 * Window object used as a screen border, input only window that only
 * handles actions.
 */
class EdgeWO : public PWinObj
{
public:
    EdgeWO(Window root, EdgeType edge, bool set_strut);
    virtual ~EdgeWO(void);

    void configureStrut(bool set_strut);

    virtual void mapWindow(void);

    virtual ActionEvent *handleButtonPress(XButtonEvent *ev);
    virtual ActionEvent *handleButtonRelease(XButtonEvent *ev);
    virtual ActionEvent *handleEnterEvent(XCrossingEvent *ev);

    inline EdgeType getEdge(void) const { return _edge; }

private:
    EdgeType _edge; /**< Edge position. */
    Strut _strut; /*< Strut for reserving screen edge space. */
};

#endif // _MANAGER_WINDOWS_H_
