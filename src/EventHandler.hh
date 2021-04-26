//
// EvenHandler.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_EVENTHANDLER_HH_
#define _PEKWM_EVENTHANDLER_HH_

#include "X11.hh"

/**
 * EventHandler interface for implementing event handling overriding
 * default event handling to support operations such as move drag from
 * the main event loop.
 */
class EventHandler {
public:
    enum Result {
        EVENT_PROCESSED,
        EVENT_SKIP,
        EVENT_STOP_PROCESSED,
        EVENT_STOP_SKIP
    };

    virtual ~EventHandler(void) { }

    virtual bool initEventHandler(void) = 0;

    virtual Result handleButtonPressEvent(XButtonEvent*) = 0;
    virtual Result handleButtonReleaseEvent(XButtonEvent*) = 0;
    virtual Result handleExposeEvent(XExposeEvent*) = 0;
    virtual Result handleKeyEvent(XKeyEvent*) = 0;
    virtual Result handleMotionNotifyEvent(XMotionEvent*) = 0;

protected:
    EventHandler(void) { }
};

#endif // _PEKWM_EVENTHANDLER_HH_
