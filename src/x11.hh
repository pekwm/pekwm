//
// x11.hh for pekwm
// Copyright © 2003-2011 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef _PEKWM_X11_HH_
#define _PEKWM_X11_HH_

#include "pekwm.hh"

extern "C" {
#include <X11/Xlib.h>
#ifdef HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif // HAVE_XINERAMA
#ifdef HAVE_SHAPE
#include <X11/extensions/shape.h>
#endif // HAVE_SHAPE

extern bool xerrors_ignore; /**< If true, ignore X errors. */
extern unsigned int xerrors_count; /**< Number of X errors occured. */

#ifdef DEBUG
#define setXErrorsIgnore(X) xerrors_ignore = (X)
#else // ! DEBUG
#define setXErrorsIgnore(X)
#endif // DEBUG

}

#include <vector>
#include <list>
#include <map>

/**
 * Class holding information about screen edge allocation.
 */
class Strut {
public:
    Strut(long l=0, long r=0, long t=0, long b=0, int nhead=-1)
        : left(l), right(r), top(t), bottom(b), head(nhead) { };
    ~Strut(void) { };
public: // member variables
    long left; /**< Pixels allocated on the left of the head. */
    long right; /**<Pixels allocated on the right of the head. */
    long top; /**< Pixels allocated on the top of the head.*/
    long bottom; /**< Pixels allocated on the bottom of the head.*/
    int head; /**< Which head is the strut valid for */

    /** Assign values from array of longs. */
    inline void operator=(const long *s) {
        left = s[0];
        right = s[1];
        top = s[2];
        bottom = s[3];
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
class X11
{
    //! Bits 1-15 are modifier masks, but bits 13 and 14 aren't named in X11/X.h.
    static const unsigned KbdLayoutMask1 = 1<<13;
    static const unsigned KbdLayoutMask2 = 1<<14;

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

    static void init(Display *dpy, bool honour_randr = true);
    static void destruct(void);

    inline static Display* getDpy(void) { return _dpy; }
    inline static int getScreenNum(void) { return _screen; }
    inline static Window getRoot(void) { return _root; }

    inline static const Geometry &getScreenGeometry(void) { return _screen_gm; }
    inline static uint getWidth(void)  { return _screen_gm.width; }
    inline static uint getHeight(void) { return _screen_gm.height; }

    inline static int getDepth(void) { return _depth; }
    inline static X11::PVisual *getVisual(void) { return _visual; }
    inline static GC getGC(void) { return DefaultGC(_dpy, _screen); }
    inline static Colormap getColormap(void) { return _colormap; }

    inline static
    ulong getWhitePixel(void) { return WhitePixel(_dpy, _screen); }
    inline static 
    ulong getBlackPixel(void) { return BlackPixel(_dpy, _screen); }

    /**
     * Remove state modifiers such as NumLock from state.
     */
    static void stripStateModifiers(unsigned int *state) {
        *state &= ~(_num_lock | _scroll_lock | LockMask | KbdLayoutMask1 | KbdLayoutMask2);
    }

    /** 
     * Remove button modifiers from state.
     */
    static void stripButtonModifiers(unsigned int *state) {
        *state &= ~(Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask);
    }

    static void setLockKeys(void);
    inline static uint getNumLock(void) { return _num_lock; }
    inline static uint getScrollLock(void) { return _scroll_lock; }

    inline static bool hasExtensionShape(void) { return _has_extension_shape; }
    inline static int getEventShape(void) { return _event_shape; }

    static void updateGeometry(uint width, uint height);

    inline static bool hasExtensionXRandr(void) { return _has_extension_xrandr; }
    inline static int getEventXRandr(void) { return _event_xrandr; }

    static bool getNextEvent(XEvent &ev);
    static bool grabServer(void);
    static bool ungrabServer(bool sync);
    static bool grabKeyboard(Window win);
    static bool ungrabKeyboard(void);
    static bool grabPointer(Window win, uint event_mask, Cursor cursor);
    static bool ungrabPointer(void);

    static uint getNearestHead(int x, int y);
    static uint getCurrHead(void);
    static bool getHeadInfo(uint head, Geometry &head_info);
    static Geometry getHeadGeometry(uint head);
    static void getHeadInfoWithEdge(uint head, Geometry &head_info);
    inline static int getNumHeads(void) { return _heads.size(); }

    inline static Time getLastEventTime(void) { return _last_event_time; }
    inline static void setLastEventTime(Time t) { _last_event_time = t; }

    inline static Window getLastClickID(void) { return _last_click_id; }
    inline static void setLastClickID(Window id) { _last_click_id = id; }

    inline static Time getLastClickTime(uint button) {
        if (button < BUTTON_NO) {
            return _last_click_time[button];
        }
        return 0;
    }
    inline static void setLastClickTime(uint button, Time time) {
        if (button < BUTTON_NO) {
            _last_click_time[button] = time;
        }
    }

    inline static bool isDoubleClick(Window id, uint button, Time time, Time dc_time) {
        if ((_last_click_id == id) &&
                ((time - getLastClickTime(button)) < dc_time)) {
            return true;
        }
        return false;
    }

    static void getMousePosition(int &x, int &y);
    static uint getButtonFromState(uint state);

    static void addStrut(Strut *strut);
    static void removeStrut(Strut *rem_strut);
    static void updateStrut(void);
    inline static Strut *getStrut(void) { return &_strut; }

    static uint getMaskFromKeycode(KeyCode keycode);
    static KeyCode getKeycodeFromMask(uint mask);

    inline static void removeMotionEvents(void)
    {
        XEvent xev;
        while (XCheckMaskEvent(_dpy, PointerMotionMask, &xev))
            ;
    }

    static const uint MODIFIER_TO_MASK[]; /**< Modifier from (XModifierKeymap) to mask table. */
    static const uint MODIFIER_TO_MASK_NUM; /**< Number of entries in MODIFIER_TO_MASK. */

    // X11 function wrappers

    inline static void grabButton(unsigned b, unsigned int mod, Window win, 
                                  unsigned mask, int mode=GrabModeAsync)
    {
        XGrabButton(_dpy, b, mod, win, true, mask, mode, GrabModeAsync, None, None);
    }

    inline static void mapWindow(Window w)   { XMapWindow(_dpy, w); }
    inline static void unmapWindow(Window w) { XUnmapWindow(_dpy, w); }

    inline static void ungrabButton(Window win) {
        XUngrabButton(_dpy, AnyButton, AnyModifier, win);
    }

    inline static bool checkTypedEvent(int type, XEvent *ev) {
        return XCheckTypedEvent(_dpy, type, ev);
    }

    inline static int selectInput(Window w, long mask) {
        return XSelectInput(_dpy, w, mask);
    }

    static int sendEvent(Window win, Atom atom, long mask,
                         long v1=0l, long v2=0l, long v3=0l,
                         long v4=0l, long v5=0l);

#ifdef HAVE_SHAPE
    inline static void shapeCombine(Window dst, int x, int y, Window src, int op) {
        XShapeCombineShape(_dpy, dst, ShapeBounding, x, y, src, ShapeBounding, op);
    }

    inline static void shapeSetRect(Window dst, XRectangle *rect) {
        XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
                                ShapeSet, YXBanded);
    }

    inline static void shapeIntersectRect(Window dst, XRectangle *rect) {
        XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
                                ShapeIntersect, YXBanded);
    }

    inline static void shapeSetMask(Window dst, Pixmap pix) {
        XShapeCombineMask(_dpy, dst, ShapeBounding, 0, 0, pix, ShapeSet);
    }

    inline static XRectangle *shapeGetRects(Window win, int *num) {
        int t;
        return XShapeGetRectangles(_dpy, win, ShapeBounding, num, &t);
    }
#endif

private:
    // squared distance because computing with sqrt is expensive

    // gets the squared distance between 2 points
    inline static uint calcDistance(int x1, int y1, int x2, int y2) {
        return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
    }
    // gets the squared distance between 2 points with either x or y the same
    inline static uint calcDistance(int p1, int p2) { return (p1 - p2) * (p1 - p2); }

    static void initHeads(void);
    static void initHeadsRandr(void);
    static void initHeadsXinerama(void);

    static Display *_dpy;
    static bool _honour_randr; /**< Boolean flag if randr should be honoured. */
    static int _fd;

    static int _screen, _depth;

    static Geometry _screen_gm; /**< Screen geometry, no head information. */

    static Window _root;
    static X11::PVisual *_visual;
    static Colormap _colormap;
    static XModifierKeymap *_modifier_map; /**< Key to modifier mappings. */

    static uint _num_lock;
    static uint _scroll_lock;

    static bool _has_extension_shape;
    static int _event_shape;

    static bool _has_extension_xinerama;

    static bool _has_extension_xrandr;
    static int _event_xrandr;

    static std::vector<Head> _heads; //! Array of head information
    static uint _last_head; //! Last accessed head

    static uint _server_grabs;

    static Time _last_event_time;
    // information for dobule clicks
    static Window _last_click_id;
    static Time _last_click_time[BUTTON_NO - 1];

    static Strut _strut;
    static std::list<Strut*> _strut_list;

    X11(void) {}
    ~X11(void) {}
};

#endif // _PEKWM_X11_HH_
