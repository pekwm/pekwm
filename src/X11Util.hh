//
// X11Util.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "Config.hh"
#include "PWinObj.hh"
#include "x11.hh"

namespace X11Util {
    uint getCurrHead(void);
    uint getNearestHead(const PWinObj& wo);
}
