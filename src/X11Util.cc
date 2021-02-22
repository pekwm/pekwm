//
// X11Util.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PWinObj.hh"
#include "X11Util.hh"

namespace X11Util {

    /**
     * Get current head, depending on configuration it is the same
     * head as the cursor is on OR the head where the focused window
     * is on.
     */
    uint getCurrHead(CurrHeadSelector chs)
    {
        PWinObj *wo;
        switch (chs) {
        case CURR_HEAD_SELECTOR_FOCUSED_WINDOW:
            wo = PWinObj::getFocusedPWinObj();
            if (wo != nullptr) {
                return X11::getNearestHead(wo->getX() + (wo->getWidth() / 2),
                                           wo->getY() + (wo->getHeight() / 2));
            }
        case CURR_HEAD_SELECTOR_CURSOR:
        case CURR_HEAD_SELECTOR_NO:
            return X11::getCursorHead();
        }

        return 0;
    }

    /**
     * Get head closest to center of provided PWinObj.
     */
    uint getNearestHead(const PWinObj& wo)
    {
        return X11::getNearestHead(wo.getX() + (wo.getWidth() / 2),
                                   wo.getY() + (wo.getHeight() / 2));
    }

    /**
     * Read EWMH state atoms on window.
     */
    bool
    readEwmhStates(Window win, NetWMStates &win_states)
    {
        int num = 0;
        auto states = static_cast<Atom*>(X11::getEwmhPropData(win, STATE,
                                                              XA_ATOM, num));
        if (! states) {
            return false;
        }

        for (int i = 0; i < num; ++i) {
            if (states[i] == X11::getAtom(STATE_MODAL)) {
                win_states.modal = true;
            } else if (states[i] == X11::getAtom(STATE_STICKY)) {
                win_states.sticky = true;
            } else if (states[i] == X11::getAtom(STATE_MAXIMIZED_VERT)) {
                win_states.max_vert = true;
            } else if (states[i] == X11::getAtom(STATE_MAXIMIZED_HORZ)) {
                win_states.max_horz = true;
            } else if (states[i] == X11::getAtom(STATE_SHADED)) {
                win_states.shaded = true;
            } else if (states[i] == X11::getAtom(STATE_SKIP_TASKBAR)) {
                win_states.skip_taskbar = true;
            } else if (states[i] == X11::getAtom(STATE_SKIP_PAGER)) {
                win_states.skip_pager = true;
            } else if (states[i] == X11::getAtom(STATE_DEMANDS_ATTENTION)) {
                win_states.demands_attention = true;
            } else if (states[i] == X11::getAtom(STATE_HIDDEN)) {
                win_states.hidden = true;
            } else if (states[i] == X11::getAtom(STATE_FULLSCREEN)) {
                win_states.fullscreen = true;
            } else if (states[i] == X11::getAtom(STATE_ABOVE)) {
                win_states.above = true;
            } else if (states[i] == X11::getAtom(STATE_BELOW)) {
                win_states.below = true;
            }
        }

        X11::free(states);

        return true;
    }

}
