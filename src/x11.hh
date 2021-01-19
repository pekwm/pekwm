//
// x11.hh for pekwm
// Copyright (C) 2003-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_X11_HH_
#define _PEKWM_X11_HH_

#include "config.h"

#include "pekwm.hh"

#include <array>
#include <iostream>
#include <string>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
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

/**
 * Bitmask values for parseGeometry result.
 */
enum XGeometryMask {
    NO_VALUE = 0,
    X_VALUE = 1 << 0,
    Y_VALUE = 1 << 1,
    WIDTH_VALUE = 1 << 2,
    HEIGHT_VALUE = 1 << 3,
    ALL_VALUES = 1 << 4,
    X_NEGATIVE = 1 << 5,
    Y_NEGATIVE = 1 << 6,
    X_PERCENT = 1 << 7,
    Y_PERCENT = 1 << 8,
    WIDTH_PERCENT = 1 << 9,
    HEIGHT_PERCENT = 1 << 10
};

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
    Head(int nx, int ny, uint nwidth, uint nheight)
        : x(nx),
          y(ny),
          width(nwidth),
          height(nheight)
    {
    };

public:
    int x;
    int y;
    uint width;
    uint height;
};

//! @brief Display information class.
class X11
{
    /** Bits 1-15 are modifier masks, but bits 13 and 14 aren't
        named in X11/X.h. */
    static const unsigned KbdLayoutMask1 = 1<<13;
    static const unsigned KbdLayoutMask2 = 1<<14;

public:
    static void init(Display *dpy, bool honour_randr = true);
    static void destruct(void);

    inline static Display* getDpy(void) { return _dpy; }
    inline static int getScreenNum(void) { return _screen; }
    inline static Window getRoot(void) { return _root; }

    inline static const Geometry &getScreenGeometry(void) { return _screen_gm; }
    inline static uint getWidth(void)  { return _screen_gm.width; }
    inline static uint getHeight(void) { return _screen_gm.height; }

    inline static int getDepth(void) { return _depth; }
    inline static Visual *getVisual(void) { return _visual; }
    inline static GC getGC(void) { return DefaultGC(_dpy, _screen); }
    inline static Colormap getColormap(void) { return _colormap; }

    static XColor *getColor(const std::string &color);
    static void returnColor(XColor *xc);

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

    static bool updateGeometry(uint width, uint height);

    inline static bool hasExtensionXRandr(void) { return _has_extension_xrandr; }
    inline static int getEventXRandr(void) { return _event_xrandr; }

    inline static Cursor getCursor(CursorType type) { return _cursor_map[type]; }

    static bool getNextEvent(XEvent &ev);
    static bool grabServer(void);
    static bool ungrabServer(bool sync);
    static bool grabKeyboard(Window win);
    static bool ungrabKeyboard(void);
    static bool grabPointer(Window win, uint event_mask, CursorType cursor);
    static bool ungrabPointer(void);

    static uint getNearestHead(int x, int y);
    static uint getCurrHead(void);
    static bool getHeadInfo(uint head, Geometry &head_info);
    static Geometry getHeadGeometry(uint head);
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

    inline static Atom getAtom(AtomName name) { return _atoms[name]; }
    inline static void setAtom(Window win, AtomName aname, AtomName value) {
        XChangeProperty(_dpy, win, _atoms[aname], XA_ATOM, 32,
                        PropModeReplace, (uchar *) &_atoms[value], 1);
    }
    inline static void setAtoms(Window win, AtomName aname, Atom *values, int size) {
        XChangeProperty(_dpy, win, _atoms[aname], XA_ATOM, 32,
                        PropModeReplace, (uchar *) values, size);
    }
    inline static void setEwmhAtomsSupport(Window win) {
        XChangeProperty(_dpy, win, _atoms[NET_SUPPORTED], XA_ATOM, 32,
                        PropModeReplace, (uchar *) _atoms, UTF8_STRING+1);
    }

    inline static void setWindow(Window win, AtomName aname, Window value) {
        XChangeProperty(_dpy, win, _atoms[aname], XA_WINDOW, 32,
                        PropModeReplace, (uchar *) &value, 1);
    }
    inline static void setWindows(Window win, AtomName aname, Window *values, int size) {
        XChangeProperty(_dpy, win, _atoms[aname], XA_WINDOW, 32,
                        PropModeReplace, (uchar *) values, size);
    }

    inline static bool getLong(Window win, AtomName aname, long &value) {
        uchar *udata = 0;
        if (getProperty(win, aname, XA_CARDINAL, 1L, &udata, 0)) {
            value = *reinterpret_cast<long*>(udata);
            XFree(udata);
            return true;
        }
        return false;
    }
    static void setLong(Window win, AtomName aname, long value,
                        long format=XA_CARDINAL) {
        XChangeProperty(_dpy, win, _atoms[aname], format, 32,
                        PropModeReplace, (uchar *) &value, 1);
    }
    inline static void setLongs(Window win, AtomName aname, long *values, int num) {
        XChangeProperty(_dpy, win, _atoms[aname], XA_CARDINAL, 32,
                        PropModeReplace, reinterpret_cast<unsigned char*>(values), num);
    }

    inline static bool getUtf8String(Window win, AtomName aname, std::string &value) {
        unsigned char *data = 0;
        if (getProperty(win, aname, _atoms[UTF8_STRING], 32, &data, 0)) {
            value = std::string(reinterpret_cast<char*>(data));
            XFree(data);
            return true;
        }
        return false;
    }

    inline static void setUtf8String(Window win, AtomName aname, const std::string &value) {
        XChangeProperty(_dpy, win, _atoms[aname], _atoms[UTF8_STRING], 8, PropModeReplace,
                        reinterpret_cast<const uchar*>(value.c_str()), value.size());
    }

    inline static void setUtf8StringArray(Window win, AtomName aname, unsigned char *values, uint length) {
        XChangeProperty(_dpy, win, _atoms[aname], _atoms[UTF8_STRING], 8, PropModeReplace, values, length);
    }

    inline static bool getString(Window win, AtomName aname, std::string &value) {
        uchar *data = 0;
        if (getProperty(win, aname, XA_STRING, 64L, &data, 0)) {
            value = std::string((const char*) data);
            XFree(data);
            return true;
        }
        return false;
    }

    inline static void setString(Window win, AtomName aname, const std::string &value) {
        XChangeProperty(_dpy, win, _atoms[aname], XA_STRING, 8, PropModeReplace,
                        (uchar*)value.c_str(), value.size());
    }

    static bool getProperty(Window win, AtomName aname, Atom type, ulong expected,
                     uchar **data, ulong *actual);
    static bool getTextProperty(Window win, Atom atom, std::string &value);
    static void *getEwmhPropData(Window win, AtomName prop, Atom type, int &num);
    inline static void unsetProperty(Window win, AtomName aname) {
        XDeleteProperty(_dpy, win, _atoms[aname]);
    }

    static void getMousePosition(int &x, int &y);
    static uint getButtonFromState(uint state);

    static uint getMaskFromKeycode(KeyCode keycode);
    static KeyCode getKeycodeFromMask(uint mask);
    static KeySym getKeysymFromKeycode(KeyCode keycode);

    inline static void removeMotionEvents(void)
    {
        XEvent xev;
        while (XCheckMaskEvent(_dpy, PointerMotionMask, &xev))
            ;
    }

    static const uint MODIFIER_TO_MASK[]; /**< Modifier from (XModifierKeymap) to mask table. */
    static const uint MODIFIER_TO_MASK_NUM; /**< Number of entries in MODIFIER_TO_MASK. */

    // helper functions

    static int parseGeometry(const std::string& str, Geometry& gm);

    inline static
    void keepVisible(Geometry &gm) {
        if (gm.x > static_cast<int>(getWidth()) - 3) {
            gm.x = getWidth() - 3;
        }
        if (gm.x + static_cast<int>(gm.width) < 3) {
            gm.x = 3 - gm.width;
        }
        if (gm.y > static_cast<int>(getHeight()) - 3) {
            gm.y = getHeight() - 3;
        }
        if (gm.y + static_cast<int>(gm.height) < 3) {
            gm.y = 3 - gm.height;
        }
    }

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

    /**
     * Wrapper for XRestackWindows, windows go in top-to-bottom order.
     */
    inline static void stackWindows(Window *wins, unsigned len) {
        if (len > 1) {
            XRestackWindows(_dpy, wins, len);
        }
    }

    inline static bool checkTypedEvent(int type, XEvent *ev) {
        return XCheckTypedEvent(_dpy, type, ev);
    }

    inline static int selectInput(Window w, long mask) {
        return XSelectInput(_dpy, w, mask);
    }

    inline static void setInputFocus(Window w) {
        XSetInputFocus(_dpy, w, RevertToPointerRoot, CurrentTime);
    }

    static int sendEvent(Window win, Atom atom, long mask,
                         long v1=0l, long v2=0l, long v3=0l,
                         long v4=0l, long v5=0l);

    inline static int
    changeProperty(Window win, Atom prop, Atom type, int format,
                   int mode, unsigned char *data, int ne)
    {
        return XChangeProperty(_dpy, win, prop, type, format, mode, data, ne);
    }

    inline static int
    getGeometry(Window win, unsigned *w, unsigned *h, unsigned *bw)
    {
        Window wn; int x, y; unsigned foo;
        return XGetGeometry(_dpy, win, &wn, &x, &y, w, h, bw, &foo);
    }

    inline static bool
    getWindowAttributes(Window win, XWindowAttributes *wa) {
        return XGetWindowAttributes(_dpy, win, wa);
    }

    inline static Pixmap
    createPixmapMask(unsigned w, unsigned h) {
        return XCreatePixmap(_dpy, _root, w, h, 1);
    }

    inline static Pixmap
    createPixmap(unsigned w, unsigned h) {
        return XCreatePixmap(_dpy, _root, w, h, _depth);
    }

    inline static void
    freePixmap(Pixmap pixmap) {
        XFreePixmap(_dpy, pixmap);
    }

    static void setWindowBackgroundPixmap(Window window, Pixmap pixmap) {
        XSetWindowBackgroundPixmap(_dpy, window, pixmap);
    }
    static void  clearWindow(Window window) { XClearWindow(_dpy, window); }

#ifdef HAVE_SHAPE
    inline static void shapeQuery(Window dst, int *bshaped) {
        int foo; unsigned bar;
        XShapeQueryExtents(_dpy, dst, bshaped, &foo, &foo, &bar, &bar,
                           &foo, &foo, &foo, &bar, &bar);
    }

    inline static void shapeCombine(Window dst, int kind, int x, int y, Window src, int op) {
        XShapeCombineShape(_dpy, dst, kind, x, y, src, kind, op);
    }

    inline static void shapeSetRect(Window dst, XRectangle *rect) {
        XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
                                ShapeSet, YXBanded);
    }

    inline static void shapeIntersectRect(Window dst, XRectangle *rect) {
        XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
                                ShapeIntersect, YXBanded);
    }

    inline static void shapeSetMask(Window dst, int kind, Pixmap pix) {
        XShapeCombineMask(_dpy, dst, kind, 0, 0, pix, ShapeSet);
    }

    inline static XRectangle *shapeGetRects(Window win, int *num) {
        int t;
        return XShapeGetRectangles(_dpy, win, ShapeBounding, num, &t);
    }
#endif

protected:
    static int parseGeometryVal(const char *c_str, const char *e_end, int &val_ret);

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

private:

    static Display *_dpy;
    static bool _honour_randr; /**< Boolean flag if randr should be honoured. */
    static int _fd;

    static int _screen, _depth;

    static Geometry _screen_gm; /**< Screen geometry, no head information. */

    static Window _root;
    static Visual *_visual;
    static Colormap _colormap;
    static XModifierKeymap *_modifier_map; /**< Key to modifier mappings. */

    static uint _num_lock;
    static uint _scroll_lock;

    static bool _has_extension_shape;
    static int _event_shape;

    static bool _has_extension_xkb;

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

    static std::array<Cursor, CURSOR_NONE> _cursor_map;

    class ColorEntry;
    static std::vector<ColorEntry *> _colours;
    static XColor _xc_default; // when allocating fails

    static Atom _atoms[MAX_NR_ATOMS];

    X11(void) {}
    ~X11(void) {}
};

#endif // _PEKWM_X11_HH_
