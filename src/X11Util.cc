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
     * Grabs the button button, with the mod mod and mask mask on the
     * window win and cursor curs with "all" possible extra modifiers
     */
    void
    grabButton(int button, int mod, int mask, Window win, int mode)
    {
        uint num_lock = X11::getNumLock();
        uint scroll_lock = X11::getScrollLock();

        X11::grabButton(button, mod, win, mask, mode);
        X11::grabButton(button, mod|LockMask, win, mask, mode);

        if (num_lock) {
            X11::grabButton(button, mod|num_lock, win, mask, mode);
            X11::grabButton(button, mod|num_lock|LockMask, win, mask, mode);
        }
        if (scroll_lock) {
            X11::grabButton(button, mod|scroll_lock, win, mask, mode);
            X11::grabButton(button, mod|scroll_lock|LockMask, win, mask, mode);
        }
        if (num_lock && scroll_lock) {
            X11::grabButton(button, mod|num_lock|scroll_lock, win, mask, mode);
            X11::grabButton(button, mod|num_lock|scroll_lock|LockMask,
                            win, mask, mode);
        }
    }

    /**
     * Reads MWM hints from a client.
     *
     * @return true if the hint was read succesfully.
     */
    bool
    readMwmHints(Window win, MwmHints &hints)
    {
        Atom atom = X11::getAtom(MOTIF_WM_HINTS);
        uchar *data;
        ulong items_read;
        if (! X11::getProperty(win, atom, atom, 20L, &data, &items_read)) {
            return false;
        }

        if (items_read >= MWM_HINTS_NUM) {
            hints = *reinterpret_cast<MwmHints*>(data);
        }

        X11::free(data);
        return items_read >= MWM_HINTS_NUM;
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
