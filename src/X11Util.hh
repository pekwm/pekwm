//
// X11Util.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "PWinObj.hh"

// Extended Net Hints stuff
class NetWMStates {
public:
    NetWMStates(void)
        : modal(false),
          sticky(false),
          max_vert(false),
          max_horz(false), shaded(false),
          skip_taskbar(false),
          skip_pager(false),
          hidden(false),
          fullscreen(false),
          above(false),
          below(false),
          demands_attention(false)
    {
    }
    ~NetWMStates(void) { }

    bool modal;
    bool sticky;
    bool max_vert, max_horz;
    bool shaded;
    bool skip_taskbar, skip_pager;
    bool hidden;
    bool fullscreen;
    bool above, below;
    bool demands_attention;
};

namespace X11Util {
    uint getCurrHead(CurrHeadSelector chs);
    uint getNearestHead(const PWinObj& wo);

    bool readEwmhStates(Window win, NetWMStates &win_states);
}
