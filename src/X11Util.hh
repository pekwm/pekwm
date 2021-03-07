//
// X11Util.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "PWinObj.hh"

struct MwmHints {
    ulong flags;
    ulong functions;
    ulong decorations;
};

enum {
    MWM_HINTS_FUNCTIONS = (1L << 0),
    MWM_HINTS_DECORATIONS = (1L << 1),
    MWM_HINTS_NUM = 3
};

enum {
    MWM_FUNC_ALL = (1L << 0),
    MWM_FUNC_RESIZE = (1L << 1),
    MWM_FUNC_MOVE = (1L << 2),
    MWM_FUNC_ICONIFY = (1L << 3),
    MWM_FUNC_MAXIMIZE = (1L << 4),
    MWM_FUNC_CLOSE = (1L << 5)
};

enum {
    MWM_DECOR_ALL = (1L << 0),
    MWM_DECOR_BORDER = (1L << 1),
    MWM_DECOR_HANDLE = (1L << 2),
    MWM_DECOR_TITLE = (1L << 3),
    MWM_DECOR_MENU = (1L << 4),
    MWM_DECOR_ICONIFY = (1L << 5),
    MWM_DECOR_MAXIMIZE = (1L << 6)
};

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

    void grabButton(int button, int mod, int mask, Window win,
                    int mode=GrabModeAsync);

    bool readMwmHints(Window win, MwmHints &hints);
    bool readEwmhStates(Window win, NetWMStates &win_states);
}
