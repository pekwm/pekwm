//
// ThemeGm.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "Theme.hh"
#include "PTexturePlain.hh"
#include "X11Util.hh"

class ThemeState {
public:
    virtual bool hasBorder(void) const = 0;
    virtual bool hasTitlebar(void) const = 0;
    virtual bool isShaded(void) const = 0;
    virtual FocusedState getFocusedState(bool selected) const = 0;
};

/**
 * Theme state based on MWM hints.
 */
class MwmThemeState : public ThemeState {
public:
    void setHints(const MwmHints &hints) { _hints = hints; }

    virtual bool hasBorder(void) const override {
        if (_hints.flags & MWM_HINTS_DECORATIONS) {
            return _hints.decorations & (MWM_DECOR_ALL | MWM_DECOR_BORDER);
        } else {
            return true;
        }
    }
    virtual bool hasTitlebar(void) const override {
        if (_hints.flags & MWM_HINTS_DECORATIONS) {
            return _hints.decorations & (MWM_DECOR_ALL | MWM_DECOR_TITLE);
        } else {
            return true;
        }
    }
    virtual bool isShaded(void) const override {
        return false;
    }
    virtual FocusedState getFocusedState(bool selected) const override {
        return FOCUSED_STATE_FOCUSED;
    }

private:
    MwmHints _hints;
};

class ThemeGm {
public:
    ThemeGm(Theme::PDecorData *data)
        : _data(data)
    {
    }
    ~ThemeGm(void)
    {
    }

    void setData(Theme::PDecorData *data) { _data = data; }

    uint decorWidth(const ThemeState *state) const {
        return bdLeft(state) + bdRight(state);
    }
    uint decorHeight(const ThemeState *state) const {
       return bdTop(state) + bdBottom(state) + titleHeight(state);
    }

    /** Returns font used at FocusedState state. */
    PFont *getFont(FocusedState state) const {
        return _data->getFont(state);
    }

    /**
     * Calculate title width (for given title)
     */
    uint titleWidth(const ThemeState *state, const std::wstring& str) const {
        return getFont(state->getFocusedState(false))->getWidth(str)
            + _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT)
            + titleLeftOffset(state) + titleRightOffset(state);
    }

    /** Calculate title height, 0 if titlebar is disabled. */
    uint titleHeight(const ThemeState *state) const {
        if (! state->hasTitlebar()) {
            return 0;
        }

        if (_data->isTitleHeightAdapt()) {
            return getFont(state->getFocusedState(false))->getHeight()
                + _data->getPad(PAD_UP) + _data->getPad(PAD_DOWN);
        } else {
            return _data->getTitleHeight();
        }
    }

    /**
     * Offset for title position, depends on border size (and shape)
     */
    uint titleLeftOffset(const ThemeState *state) const {
        // Top left corner has same height as top border, title should
        // be placed right of the left border.
        uint top_left = bdTopLeft(state);
        if (top_left && bdTopLeftHeight(state) == bdTop(state)) {
            return bdLeft(state);
        }
        return top_left;
    }
    /**
     * Offset for title position, depends on border size (and shape)
     */
    uint titleRightOffset(const ThemeState *state) const {
        // Top right corner has same height as top border, title should
        // be placed left of the right border.
        uint top_right = bdTopRight(state);
        if (top_right && bdTopRightHeight(state) == bdTop(state)) {
            return bdRight(state);
        }
        return top_right;
    }

    /**
     * Offset for top border, if non full width the border is placed
     * below the titlebar.
     */
    uint bdTopOffset(const ThemeState *state) const {
        return _data->getTitleWidthMin()
            ? titleHeight(state) : 0;
    }

    uint bdTop(const ThemeState *state) const {
        return getBorder(state, BORDER_TOP)->getHeight();
    };
    uint bdTopLeft(const ThemeState *state) const {
        return getBorder(state, BORDER_TOP_LEFT)->getWidth();
    }
    uint bdTopLeftHeight(const ThemeState *state) const {
        return getBorder(state, BORDER_TOP_LEFT)->getHeight();
    }
    uint bdTopRight(const ThemeState *state) const {
        return getBorder(state, BORDER_TOP_RIGHT)->getWidth();
    }
    uint bdTopRightHeight(const ThemeState *state) const {
        return getBorder(state, BORDER_TOP_RIGHT)->getHeight();
    }
    uint bdBottom(const ThemeState *state) const {
        return getBorder(state, BORDER_BOTTOM)->getHeight();
    }
    uint bdBottomLeft(const ThemeState *state) const {
        return getBorder(state, BORDER_BOTTOM_LEFT)->getWidth();
    }
    uint bdBottomLeftHeight(const ThemeState *state) const {
        return getBorder(state, BORDER_BOTTOM_LEFT)->getHeight();
    }
    uint bdBottomRight(const ThemeState *state) const {
        return getBorder(state, BORDER_BOTTOM_RIGHT)->getWidth();
    }
    uint bdBottomRightHeight(const ThemeState *state) const {
        return getBorder(state, BORDER_BOTTOM_RIGHT)->getHeight();
    }
    uint bdLeft(const ThemeState *state) const {
        return getBorder(state, BORDER_LEFT)->getWidth();
    }
    uint bdRight(const ThemeState *state) const {
        return getBorder(state, BORDER_RIGHT)->getWidth();
    }

private:
    const PTexture* getBorder(const ThemeState *state,
                              BorderPosition pos) const {
        if (! _data || ! state->hasBorder()) {
            return &_empty;
        }
        auto focused_state = state->getFocusedState(false);
        return _data->getBorderTexture(focused_state, pos);
    }

private:
    Theme::PDecorData *_data;
    PTextureEmpty _empty;
};
