//
// Screen.hh for pekwm
// Copyright © 2003-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef _SCREENINFO_HH_
#define _SCREENINFO_HH_

#include "pekwm.hh"
#include "PWinObj.hh"

extern "C" {
#include <X11/Xlib.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif // HAVE_XINERAMA
}

#include <vector>
#include <list>
#include <map>

class Strut {
public:
    Strut(void) : left(0), right(0), top(0), bottom(0), head(-1) { };
    ~Strut(void) { };
public: // member variables
    CARD32 left, right;
    CARD32 top, bottom;
    int head;

    inline void operator=(const CARD32 *s) {
        if (sizeof(s) > 3) {
            left = s[0];
            right = s[1];
            top = s[2];
            bottom = s[3];
        }
    }
};

//! Output head, used to share same code with Xinerama and RandR
class Head {
public:
  Head(int nx, int ny, uint nwidth, uint nheight) : x(nx), y(ny), width(nwidth), height(nheight) { };

public:
  int x;
  int y;
  uint width;
  uint height;

  Strut strut;
};

//! @brief Display information class.
class PScreen
{
public:
    //! @brief Visual wrapper class.
    class PVisual {
    public:
        PVisual(Visual *x_visual);
        ~PVisual(void);

        //! @brief Returns pointer to X Visual.
        inline Visual *getXVisual(void) { return _x_visual; }

        inline int getRShift(void) const { return _r_shift; }
        inline int getRPrec(void) const { return _r_prec; }
        inline int getGShift(void) const { return _g_shift; }
        inline int getGPrec(void) const { return _g_prec; }
        inline int getBShift(void) const { return _b_shift; }
        inline int getBPrec(void) const { return _b_prec; }

    private:
        void getShiftPrecFromMask(ulong mask, int &shift, int &prec);

    private:
        Visual *_x_visual; //!< Pointer to X Visual.

        int _r_shift; //!< Red shift, alternative red_mask representation.
        int _r_prec; //!< Red prec, alternative red_mask representation.
        int _g_shift; //!< Green shift, alternative green_mask representation.
        int _g_prec; //!< Green prec, alternative green_mask representation.
        int _b_shift; //!< Blue shift, alternative blue_mask representation.
        int _b_prec; //!< Blue prec, alternative blue_mask representation.
    };

    PScreen(Display *dpy);
    ~PScreen(void);

    static PScreen* instance(void) { return _instance; }

    inline Display* getDpy(void) const { return _dpy; }
    inline int getScreenNum(void) const { return _screen; }
    inline Window getRoot(void) const { return _root; }
    inline uint getWidth(void) const { return _width; }
    inline uint getHeight(void) const { return _height; }

    inline int getDepth(void) const { return _depth; }
    inline PScreen::PVisual *getVisual(void) { return _visual; }
    inline GC getGC(void) { return DefaultGC(_dpy, _screen); }
    inline Colormap getColormap(void) const { return _colormap; }

    inline ulong getWhitePixel(void)
    const { return WhitePixel(_dpy, _screen); }
    inline ulong getBlackPixel(void)
    const { return BlackPixel(_dpy, _screen); }

    inline uint getNumLock(void) const { return _num_lock; }
    inline uint getScrollLock(void) const { return _scroll_lock; }

#ifdef HAVE_SHAPE
    inline bool hasExtensionShape(void) const { return _has_extension_shape; }
    inline int getEventShape(void) const { return _event_shape; }
#endif // HAVE_SHAPE

    void updateGeometry(uint width, uint height);
#ifdef HAVE_XRANDR
    inline bool hasExtensionXRandr(void) const { return _has_extension_xrandr; }
    inline int getEventXRandr(void) const { return _event_xrandr; }
#endif // HAVE_XRANDR

    bool grabServer(void);
    bool ungrabServer(bool sync);
    bool grabKeyboard(Window win);
    bool ungrabKeyboard(void);
    bool grabPointer(Window win, uint event_mask, Cursor cursor);
    bool ungrabPointer(void);

    uint getNearestHead(int x, int y);
    uint getCurrHead(void);
    bool getHeadInfo(uint head, Geometry &head_info);
    void getHeadInfoWithEdge(uint head, Geometry &head_info);
    inline int getNumHeads(void) const { return _heads.size(); }

    inline long getLastEventTime(void) const { return _last_event_time; }
    inline void setLastEventTime(long t) { _last_event_time = t; }

    inline Window getLastClickID(void) { return _last_click_id; }
    inline void setLastClickID(Window id) { _last_click_id = id; }

    inline Time getLastClickTime(uint button) {
        if (button < BUTTON_NO) {
            return _last_click_time[button];
        }
        return 0;
    }
    inline void setLastClickTime(uint button, Time time) {
        if (button < BUTTON_NO) {
            _last_click_time[button] = time;
        }
    }

    inline bool isDoubleClick(Window id, uint button, Time time, Time dc_time) {
        if ((_last_click_id == id) &&
                ((time - getLastClickTime(button)) < dc_time)) {
            return true;
        }
        return false;
    }

    void getMousePosition(int &x, int &y);
    uint getButtonFromState(uint state);

    void addStrut(Strut *strut);
    void removeStrut(Strut *rem_strut);
    void updateStrut(void);
    inline Strut *getStrut(void) { return &_strut; }

private:
    // squared distance because computing with sqrt is expensive

    // gets the squared distance between 2 points
    inline uint calcDistance(int x1, int y1, int x2, int y2) {
        return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
    }
    // gets the squared distance between 2 points with either x or y the same
    inline uint calcDistance(int p1, int p2) { return (p1 - p2) * (p1 - p2); }

  void initHeads(void);
  void initHeadsRandr(void);
  void initHeadsXinerama(void);

private:
    Display *_dpy;

    int _screen, _depth;
    uint _width, _height;

    Window _root;
    PScreen::PVisual *_visual;
    Colormap _colormap;

    uint _num_lock;
    uint _scroll_lock;

    bool _has_extension_shape;
    int _event_shape;

    bool _has_extension_xinerama;

    bool _has_extension_xrandr;
    int _event_xrandr;

  std::vector<Head> _heads; //! Array of head information
  uint _last_head; //! Last accessed head

    uint _server_grabs;

    long _last_event_time;
    // information for dobule clicks
    Window _last_click_id;
    Time _last_click_time[BUTTON_NO - 1];

    Strut _strut;
    std::list<Strut*> _strut_list;

    static PScreen *_instance;
};

#endif // _SCREENINFO_HH_
