//
// EventLoop.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "EventHandler.hh"

extern "C" {
#include <X11/Xlib.h>
}

/**
 * EventLoop interfaction
 */
class EventLoop {
public:
    virtual void setEventHandler(EventHandler* event_handler) = 0;

    virtual void skipNextEnter(Window window) = 0;
};
