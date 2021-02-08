//
// x11.hh for pekwm
// Copyright (C) 2003-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

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
#else // ! HAVE_SHAPE
#define ShapeSet 0
#define ShapeIntersect 2

#define ShapeBounding 0
#define ShapeInput 2

#define ShapeNotifyMask 1
#endif // HAVE_SHAPE

extern bool xerrors_ignore; /**< If true, ignore X errors. */
extern unsigned int xerrors_count; /**< Number of X errors occured. */

#define setXErrorsIgnore(X) xerrors_ignore = (X)

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
    void operator=(const long *s) {
        left = s[0];
        right = s[1];
        top = s[2];
        bottom = s[3];
    }
    bool operator==(const Strut& rhs) {
        return left == rhs.left
            && right == rhs.right
            && top == rhs.top
            && bottom == rhs.bottom
            && head == rhs.head;
    }
    bool operator!=(const Strut& rhs) {
        return !operator==(rhs);
    }
    friend std::ostream &operator<<(std::ostream &stream, const Strut &strut) {
        stream << "Strut l: " << strut.left << " r: " << strut.right
               << " t: " << strut.top << " b: " << strut.bottom
               << " head " << strut.head;
        return stream;
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
    static void init(Display *dpy,
                     bool synchronous = false,
                     bool honour_randr = true);
    static void destruct(void);

    static Display* getDpy(void) { return _dpy; }
    static int getScreenNum(void) { return _screen; }
    static Window getRoot(void) { return _root; }

    static const Geometry &getScreenGeometry(void) { return _screen_gm; }
    static uint getWidth(void)  { return _screen_gm.width; }
    static uint getHeight(void) { return _screen_gm.height; }

    static int getDepth(void) { return _depth; }
    static Visual *getVisual(void) { return _visual; }
    static GC getGC(void) { return DefaultGC(_dpy, _screen); }
    static Colormap getColormap(void) { return _colormap; }

    static XColor *getColor(const std::string &color);
    static void returnColor(XColor *xc);

    static ulong getWhitePixel(void) { return WhitePixel(_dpy, _screen); }
    static ulong getBlackPixel(void) { return BlackPixel(_dpy, _screen); }

    static void free(void* data) { XFree(data); }

    static void warpPointer(int x, int y) {
        if (_dpy) {
            XWarpPointer(_dpy, None, _root, 0, 0, 0, 0, x, y);
        }
    }

    static void moveWindow(Window win, int x, int y) {
        if (_dpy) {
            XMoveWindow(_dpy, win, x, y);
        }
    }
    static void resizeWindow(Window win,
                             unsigned int width, unsigned int height) {
        if (_dpy) {
            XResizeWindow(_dpy, win, width, height);
        }
    }
    static void moveResizeWindow(Window win, int x, int y,
                                 unsigned int width, unsigned int height) {
        if (_dpy) {
            XMoveResizeWindow(_dpy, win, x, y, width, height);
        }
    }

    /**
     * Remove state modifiers such as NumLock from state.
     */
    static void stripStateModifiers(unsigned int *state) {
        *state &= ~(_num_lock | _scroll_lock | LockMask |
                    KbdLayoutMask1 | KbdLayoutMask2);
    }

    /**
     * Remove button modifiers from state.
     */
    static void stripButtonModifiers(unsigned int *state) {
        *state &= ~(Button1Mask | Button2Mask | Button3Mask |
                    Button4Mask | Button5Mask);
    }

    static void setLockKeys(void);
    inline static uint getNumLock(void) { return _num_lock; }
    inline static uint getScrollLock(void) { return _scroll_lock; }

    inline static bool hasExtensionShape(void) { return _has_extension_shape; }
    inline static int getEventShape(void) { return _event_shape; }

    static bool updateGeometry(uint width, uint height);

    static bool hasExtensionXRandr(void) { return _has_extension_xrandr; }
    static int getEventXRandr(void) { return _event_xrandr; }

    static Cursor getCursor(CursorType type) { return _cursor_map[type]; }

    static bool getNextEvent(XEvent &ev);
    static bool grabServer(void);
    static bool ungrabServer(bool sync);
    static bool grabKeyboard(Window win);
    static bool ungrabKeyboard(void);
    static bool grabPointer(Window win, uint event_mask, CursorType cursor);
    static bool ungrabPointer(void);

    static uint getNearestHead(int x, int y);
    static uint getCursorHead(void);
    static void addHead(const Head &head) { _heads.push_back(head); }
    static bool getHeadInfo(uint head, Geometry &head_info);
    static Geometry getHeadGeometry(uint head);
    static int getNumHeads(void) { return _heads.size(); }

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

    inline static bool isDoubleClick(Window id, uint button,
                                     Time time, Time dc_time) {
        if ((_last_click_id == id)
            && ((time - getLastClickTime(button)) < dc_time)) {
            return true;
        }
        return false;
    }

    static Atom getAtom(AtomName name) { return _atoms[name]; }
    static void setAtom(Window win, AtomName aname, AtomName value) {
        changeProperty(win, _atoms[aname], XA_ATOM, 32,
                       PropModeReplace, (uchar *) &_atoms[value], 1);
    }
    static void setAtoms(Window win, AtomName aname, Atom *values, int size) {
        changeProperty(win, _atoms[aname], XA_ATOM, 32,
                       PropModeReplace, (uchar *) values, size);
    }
    static void setEwmhAtomsSupport(Window win) {
        changeProperty(win, _atoms[NET_SUPPORTED], XA_ATOM, 32,
                       PropModeReplace, (uchar *) _atoms, UTF8_STRING+1);
    }

    static void setWindow(Window win, AtomName aname, Window value) {
        changeProperty(win, _atoms[aname], XA_WINDOW, 32,
                       PropModeReplace, (uchar *) &value, 1);
    }
    static void setWindows(Window win, AtomName aname, Window *values,
                           int size) {
        changeProperty(win, _atoms[aname], XA_WINDOW, 32,
                       PropModeReplace, (uchar *) values, size);
    }

    inline static bool getLong(Window win, AtomName aname, long &value) {
        uchar *udata = 0;
        if (getProperty(win, _atoms[aname], XA_CARDINAL, 1L, &udata, 0)) {
            value = *reinterpret_cast<long*>(udata);
            X11::free(udata);
            return true;
        }
        return false;
    }
    static void setLong(Window win, AtomName aname, long value,
                        long format=XA_CARDINAL) {
        changeProperty(win, _atoms[aname], format, 32,
                       PropModeReplace, (uchar *) &value, 1);
    }
    static void setLongs(Window win, AtomName aname, long *values, int num) {
        changeProperty(win, _atoms[aname], XA_CARDINAL, 32, PropModeReplace,
                       reinterpret_cast<uchar*>(values), num);
    }

    static bool getUtf8String(Window win, AtomName aname, std::string &value) {
        uchar *data = nullptr;
        if (getProperty(win, _atoms[aname], _atoms[UTF8_STRING], 0, &data, 0)) {
            value = std::string(reinterpret_cast<char*>(data));
            X11::free(data);
            return true;
        }
        return false;
    }

    static void setUtf8String(Window win, AtomName aname,
                              const std::string &value) {
        changeProperty(win, _atoms[aname], _atoms[UTF8_STRING], 8,
                       PropModeReplace,
                       reinterpret_cast<const uchar*>(value.c_str()),
                       value.size());
    }

    static void setUtf8StringArray(Window win, AtomName aname,
                                   unsigned char *values, uint length) {
        changeProperty(win, _atoms[aname], _atoms[UTF8_STRING], 8,
                       PropModeReplace, values, length);
    }

    static bool getString(Window win, AtomName aname, std::string &value) {
        uchar *data = 0;
        if (getProperty(win, _atoms[aname], XA_STRING, 0, &data, 0)) {
            value = std::string((const char*) data);
            X11::free(data);
            return true;
        }
        return false;
    }

    static void setString(Window win, AtomName aname,
                          const std::string &value) {
        changeProperty(win, _atoms[aname], XA_STRING, 8, PropModeReplace,
                       (uchar*)value.c_str(), value.size());
    }

    static bool getProperty(Window win, Atom atom, Atom type,
                            ulong expected, uchar **data, ulong *actual);
    static bool getTextProperty(Window win, Atom atom, std::string &value);
    static void *getEwmhPropData(Window win, AtomName prop,
                                 Atom type, int &num);
    static void unsetProperty(Window win, AtomName aname) {
        if (_dpy) {
            XDeleteProperty(_dpy, win, _atoms[aname]);
        }
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

    /** Modifier from (XModifierKeymap) to mask table. */
    static const uint MODIFIER_TO_MASK[];
    /** Number of entries in MODIFIER_TO_MASK. */
    static const uint MODIFIER_TO_MASK_NUM;

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

    static Window createWindow(Window parent,
                               int x, int y, uint width, uint height,
                               uint border_width, uint depth, uint _class,
                               Visual* visual, ulong valuemask,
                               XSetWindowAttributes* attrs) {
        if (_dpy) {
            return XCreateWindow(_dpy, parent,
                                 x, y, width, height, border_width,
                                 depth, _class, visual, valuemask, attrs);
        }
        return None;
    }

    static Window createSimpleWindow(Window parent,
                                     int x, int y, uint width, uint height,
                                     uint border_width,
                                     ulong border, ulong background) {
        if (_dpy) {
            return XCreateSimpleWindow(_dpy, parent, x, y, width, height,
                                       border_width, border, background);
        }
        return None;
    }

    static void destroyWindow(Window win) {
        if (_dpy) {
            XDestroyWindow(_dpy, win);
        }
    }

    static void changeWindowAttributes(Window win, ulong mask,
                                       XSetWindowAttributes &attrs) {
        if (_dpy) {
            XChangeWindowAttributes(_dpy, win, mask, &attrs);
        }
    }

    static void grabButton(unsigned b, unsigned int mod, Window win,
                           unsigned mask, int mode=GrabModeAsync) {
        XGrabButton(_dpy, b, mod, win, true, mask, mode,
                    GrabModeAsync, None, None);
    }

    static void mapWindow(Window w) { if (_dpy) { XMapWindow(_dpy, w); } }
    static void unmapWindow(Window w) { if (_dpy) { XUnmapWindow(_dpy, w); } }
    static void reparentWindow(Window w, Window parent, int x, int y) {
        if (_dpy) {
            XReparentWindow(_dpy, w, parent, x, y);
        }
    }

    static void raiseWindow(Window w) { if (_dpy) { XRaiseWindow(_dpy, w); } }
    static void lowerWindow(Window w) { if (_dpy) { XLowerWindow(_dpy, w); } }

    static void ungrabButton(Window win) {
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

    static void sync(Bool discard) {
        if (_dpy) {
            XSync(X11::getDpy(), discard);
        }
    }

    static int selectInput(Window w, long mask) {
        if (_dpy) {
            return XSelectInput(_dpy, w, mask);
        }
        return 0;
    }

    inline static void setInputFocus(Window w) {
        XSetInputFocus(_dpy, w, RevertToPointerRoot, CurrentTime);
    }

    static int sendEvent(Window win, Atom atom, long mask,
                         long v1=0l, long v2=0l, long v3=0l,
                         long v4=0l, long v5=0l);

    static int changeProperty(Window win, Atom prop, Atom type, int format,
                              int mode, const unsigned char *data, int num_e)
    {
        if (_dpy) {
            return XChangeProperty(_dpy, win, prop, type, format, mode,
                                   data, num_e);
        }
        return BadImplementation;
    }

    static int getGeometry(Window win, unsigned *w, unsigned *h, unsigned *bw) {
        Window wn;
        int x, y;
        unsigned int depth_return;
        if (_dpy) {
            return XGetGeometry(_dpy, win, &wn, &x, &y,
                                w, h, bw, &depth_return);
        }
        return BadImplementation;
    }

    static bool getWindowAttributes(Window win, XWindowAttributes *wa) {
        if (_dpy) {
            return XGetWindowAttributes(_dpy, win, wa);
        }
        return BadImplementation;
    }

    static Pixmap createPixmapMask(unsigned w, unsigned h) {
        if (_dpy) {
            return XCreatePixmap(_dpy, _root, w, h, 1);
        }
        return None;
    }

    static Pixmap createPixmap(unsigned w, unsigned h) {
        if (_dpy) {
            return XCreatePixmap(_dpy, _root, w, h, _depth);
        }
        return None;
    }

    static void freePixmap(Pixmap& pixmap) {
        if (_dpy && pixmap != None) {
            XFreePixmap(_dpy, pixmap);
        }
        pixmap = None;
    }

    static void setWindowBackground(Window window, ulong pixel) {
        if (_dpy) {
            XSetWindowBackground(_dpy, window, pixel);
        }
    }
    static void setWindowBackgroundPixmap(Window window, Pixmap pixmap) {
        if (_dpy) {
            XSetWindowBackgroundPixmap(_dpy, window, pixmap);
        }
    }
    static void  clearWindow(Window window) { XClearWindow(_dpy, window); }

#ifdef HAVE_SHAPE
    static void shapeSelectInput(Window window, ulong mask) {
        if (_dpy) {
            XShapeSelectInput(_dpy, window, mask);
        }
    }

    static void shapeQuery(Window dst, int *bshaped) {
        int foo; unsigned bar;
        XShapeQueryExtents(_dpy, dst, bshaped, &foo, &foo, &bar, &bar,
                           &foo, &foo, &foo, &bar, &bar);
    }

    static void shapeCombine(Window dst, int kind, int x, int y,
                             Window src, int op) {
        XShapeCombineShape(_dpy, dst, kind, x, y, src, kind, op);
    }

    static void shapeSetRect(Window dst, XRectangle *rect) {
        XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
                                ShapeSet, YXBanded);
    }

    static void shapeIntersectRect(Window dst, XRectangle *rect) {
        XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
                                ShapeIntersect, YXBanded);
    }

    static void shapeSetMask(Window dst, int kind, Pixmap pix) {
        XShapeCombineMask(_dpy, dst, kind, 0, 0, pix, ShapeSet);
    }

    static XRectangle *shapeGetRects(Window win, int kind, int *num) {
        int ordering;
        return XShapeGetRectangles(_dpy, win, kind, num, &ordering);
    }
#else // ! HAVE_SHAPE
    static void shapeSelectInput(Window window, ulong mask) { }

    static void shapeQuery(Window dst, int *bshaped) {
        *bshaped = 0;
    }

    static void shapeCombine(Window dst, int kind, int x, int y,
                             Window src, int op) {
    }

    static void shapeSetRect(Window dst, XRectangle *rect) {
    }

    static void shapeIntersectRect(Window dst, XRectangle *rect) {
    }

    static void shapeSetMask(Window dst, int kind, Pixmap pix) {
    }

    static XRectangle *shapeGetRects(Window win, int kind, int *num) {
        num = 0;
        return nullptr;
    }
#endif // HAVE_SHAPE

protected:
    static int parseGeometryVal(const char *c_str, const char *e_end,
                                int &val_ret);

private:
    // squared distance because computing with sqrt is expensive

    // gets the squared distance between 2 points
    inline static uint calcDistance(int x1, int y1, int x2, int y2) {
        return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
    }
    // gets the squared distance between 2 points with either x or y the same
    inline static uint calcDistance(int p1, int p2) {
        return (p1 - p2) * (p1 - p2);
    }

    static void initHeads(void);
    static void initHeadsRandr(void);
    static void initHeadsXinerama(void);

protected:
    X11(void) {}
    ~X11(void) {}

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
};
