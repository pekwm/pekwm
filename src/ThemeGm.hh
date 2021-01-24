#ifndef _THEME_GM_HH_
#define _THEME_GM_HH_

#include "Theme.hh"
#include "PTexturePlain.hh"

class ThemeState {
public:
    virtual bool hasBorder(void) const = 0;
    virtual bool hasTitlebar(void) const = 0;
    virtual bool isShaded(void) const = 0;
    virtual FocusedState getFocusedState(bool selected) const = 0;
};

class ThemeGm {
public:
    ThemeGm(Theme::PDecorData *data)
        : _data(data),
          _empty(new PTextureEmpty())
    {
    }
    ~ThemeGm()
    {
        delete _empty;
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
    PTexture* getBorder(const ThemeState *state, BorderPosition pos) const {
        if (! _data || ! state->hasBorder()) {
            return _empty;
        }
        auto focused_state = state->getFocusedState(false);
        return _data->getBorderTexture(focused_state, pos);
    }

private:
    Theme::PDecorData *_data;
    PTexture *_empty;
};

#endif // _PDECOR_GM_HH_
