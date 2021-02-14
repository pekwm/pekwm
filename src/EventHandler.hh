//
// EvenHandler.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "x11.hh"

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

    virtual Result handleButtonPressEvent(XButtonEvent *ev) = 0;
    virtual Result handleButtonReleaseEvent(XButtonEvent *ev) = 0;
    virtual Result handleKeyEvent(XKeyEvent *ev) = 0;
    virtual Result handleMotionNotifyEvent(XMotionEvent *ev) = 0;

protected:
    EventHandler(void) { }
};
