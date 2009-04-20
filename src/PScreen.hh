//
// Screen.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me{@}pekdon{.}net>
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

    PScreen(Display *dpy, bool honour_randr = true);
    ~PScreen(void);

    static PScreen* instance(void) { return _instance; }

    inline Display* getDpy(void) const { return _dpy; }
    inline int getScreenNum(void) const { return _screen; }
    inline Window getRoot(void) const { return _root; }

    const Geometry &getScreenGeometry(void) const { return _screen_gm; }
    inline uint getWidth(void) const { return _screen_gm.width; }
    inline uint getHeight(void) const { return _screen_gm.height; }

    inline int getDepth(void) const { return _depth; }
    inline PScreen::PVisual *getVisual(void) { return _visual; }
    inline GC getGC(void) { return DefaultGC(_dpy, _screen); }
    inline Colormap getColormap(void) const { return _colormap; }

    inline ulong getWhitePixel(void)
    const { return WhitePixel(_dpy, _screen); }
    inline ulong getBlackPixel(void)
    const { return BlackPixel(_dpy, _screen); }

    /**
     * Remove state modifiers such as NumLock from state.
     */
    unsigned int stripStateModifiers(unsigned int state) const {
        state &= ~_num_lock & ~_scroll_lock & ~LockMask;
        return state;
    }

    /** 
     * Remove button modifiers from state.
     */
    unsigned int stripButtonModifiers(unsigned int state) const {
        state &= ~Button1Mask & ~Button2Mask & ~Button3Mask & ~Button4Mask & ~Button5Mask;
        return state;
    }
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

    bool getNextEvent(XEvent &ev);
    bool grabServer(void);
    bool ungrabServer(bool sync);
    bool grabKeyboard(Window win);
    bool ungrabKeyboard(void);
    bool grabPointer(Window win, uint event_mask, Cursor cursor);
    bool ungrabPointer(void);

    uint getNearestHead(int x, int y);
    uint getCurrHead(void);
    bool getHeadInfo(uint head, Geometry &head_info);
    Geometry getHeadGeometry(uint head);
    void getHeadInfoWithEdge(uint head, Geometry &head_info);
    inline int getNumHeads(void) const { return _heads.size(); }

    inline Time getLastEventTime(void) const { return _last_event_time; }
    inline void setLastEventTime(Time t) { _last_event_time = t; }

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

  uint getMaskFromKeycode(KeyCode keycode);
  KeyCode getKeycodeFromMask(uint mask);

public:
  static const uint MODIFIER_TO_MASK[]; /**< Modifier from (XModifierKeymap) to mask table. */
  static const uint MODIFIER_TO_MASK_NUM; /**< Number of entries in MODIFIER_TO_MASK. */

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
    bool _honour_randr; /**< Boolean flag if randr should be honoured. */
    int _fd;

    int _screen, _depth;

  Geometry _screen_gm; /**< Screen geometry, no head information. */

    Window _root;
    PScreen::PVisual *_visual;
    Colormap _colormap;
  XModifierKeymap *_modifier_map; /**< Key to modifier mappings. */

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

    Time _last_event_time;
    // information for dobule clicks
    Window _last_click_id;
    Time _last_click_time[BUTTON_NO - 1];

    Strut _strut;
    std::list<Strut*> _strut_list;

    static PScreen *_instance;
};

#endif // _SCREENINFO_HH_
