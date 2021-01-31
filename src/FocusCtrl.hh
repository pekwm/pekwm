//
// FocusCtrl.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

extern "C" {
#include <X11/Xlib.h>
}

/**
 * Interface for tweaking the focus handling
 */
class FocusCtrl {
public:
    virtual void skipNextEnter(Window window) = 0;
};
