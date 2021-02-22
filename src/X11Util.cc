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
}
