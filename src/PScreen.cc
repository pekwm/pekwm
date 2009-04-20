//
// PScreen.cc for pekwm
// Copyright © 2003-2009 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <string>
#include <iostream>
#include <cassert>

#ifdef HAVE_LIMITS
#include <limits>
using std::numeric_limits;
#endif // HAVE_LIMITS

extern "C" {
#include <X11/Xlib.h>
#ifdef HAVE_SHAPE
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif // HAVE_SHAPE
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif // HAVE_XRANDR
#include <X11/keysym.h> // For XK_ entries
#include <sys/select.h>

#ifdef DEBUG
bool xerrors_ignore = false;
#endif // DEBUG

unsigned int xerrors_count = 0;
}

#include "PScreen.hh"
// FIXME: Remove when strut handling is moved away from here.
#include "PWinObj.hh"
#include "ManagerWindows.hh"

using std::cerr;
using std::endl;
using std::vector;
using std::list;
using std::map;
using std::string;

const uint PScreen::MODIFIER_TO_MASK[] = {
    ShiftMask, LockMask, ControlMask,
    Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
};
const uint PScreen::MODIFIER_TO_MASK_NUM = sizeof(PScreen::MODIFIER_TO_MASK[0]) / sizeof(PScreen::MODIFIER_TO_MASK);

PScreen* PScreen::_instance = 0;


extern "C" {
    /**
      * XError handler, prints error.
      */
    static int
    handleXError(Display *dpy, XErrorEvent *ev)
    {
        ++xerrors_count;

#ifdef DEBUG
        if (xerrors_ignore) {
            return 0;
        }

        char error_buf[256];
        XGetErrorText(dpy, ev->error_code, error_buf, 256);
        cerr << "XError: " << error_buf << " id: " << ev->resourceid << endl;
#endif // DEBUG

        return 0;
    }
}

//! @brief PScreen::Visual constructor.
//! @param x_visual X Visual to wrap.
PScreen::PVisual::PVisual(Visual *x_visual) : _x_visual(x_visual),
        _r_shift(0), _r_prec(0),
        _g_shift(0), _g_prec(0),
        _b_shift(0), _b_prec(0)
{
    getShiftPrecFromMask(_x_visual->red_mask, _r_shift, _r_prec);
    getShiftPrecFromMask(_x_visual->green_mask, _g_shift, _g_prec);
    getShiftPrecFromMask(_x_visual->blue_mask, _b_shift, _b_prec);
}

//! @brief PScreen::Visual destructor.
PScreen::PVisual::~PVisual(void)
{
}

//! @brief Gets shift and prec from mask.
//! @param mask red,green,blue mask of Visual.
//! @param shift Set to the shift of mask.
//! @param prec Set to the prec of mask.
void
PScreen::PVisual::getShiftPrecFromMask(ulong mask, int &shift, int &prec)
{
    for (shift = 0; ! (mask&0x1); ++shift) {
        mask >>= 1;
    }
    
    for (prec = 0; (mask&0x1); ++prec) {
        mask >>= 1;
    }
}

//! @brief PScreen constructor
PScreen::PScreen(Display *dpy, bool honour_randr)
    : _dpy(dpy), _honour_randr(honour_randr), _fd(-1),
      _screen(-1), _depth(-1),
      _root(None), _visual(0), _colormap(None),
      _modifier_map(0),
      _num_lock(0), _scroll_lock(0),
      _has_extension_shape(false), _event_shape(-1),
      _has_extension_xinerama(false),
      _has_extension_xrandr(false), _event_xrandr(-1),
      _server_grabs(0), _last_event_time(0), _last_click_id(None)
{
    if (_instance) {
        throw string("PScreen, trying to create multiple instances");
    }
    _instance = this;

    XSetErrorHandler(handleXError);

    XGrabServer(_dpy);

    _fd = ConnectionNumber(dpy);
    _screen = DefaultScreen(_dpy);
    _root = RootWindow(_dpy, _screen);

    _depth = DefaultDepth(_dpy, _screen);
    _visual = new PScreen::PVisual(DefaultVisual(_dpy, _screen));
    _colormap = DefaultColormap(_dpy, _screen);
    _modifier_map = XGetModifierMapping(_dpy);

    _screen_gm.width = WidthOfScreen(ScreenOfDisplay(_dpy, _screen));
    _screen_gm.height = HeightOfScreen(ScreenOfDisplay(_dpy, _screen));

#ifdef HAVE_SHAPE
    {
        int dummy_error;
        _has_extension_shape = XShapeQueryExtension(_dpy, &_event_shape, &dummy_error);
    }
#endif // HAVE_SHAPE

#ifdef HAVE_XRANDR
    {
        int dummy_error;
        _has_extension_xrandr = XRRQueryExtension(_dpy, &_event_xrandr, &dummy_error);
    }
#endif // HAVE_XRANDR

    // Now screen geometry has been read and extensions have been
    // looked for, read head information.
    initHeads();

    // initialize array values
    for (uint i = 0; i < (BUTTON_NO - 1); ++i) {
        _last_click_time[i] = 0;
    }

    // Figure out what keys the Num and Scroll Locks are
    setLockKeys();

    XSync(_dpy, false);
    XUngrabServer(_dpy);
}

//! @brief PScreen destructor
PScreen::~PScreen(void) {
    delete _visual;

    if (_modifier_map) {
        XFreeModifiermap(_modifier_map);
    }

    _instance = 0;
}

/**
 * Figure out what keys the Num and Scroll Locks are
 */
void
PScreen::setLockKeys(void)
{
    _num_lock = getMaskFromKeycode(XKeysymToKeycode(_dpy, XK_Num_Lock));
    _scroll_lock = getMaskFromKeycode(XKeysymToKeycode(_dpy, XK_Scroll_Lock));
}

//! @brief Get next event using select to avoid signal blocking
//! @param ev Event to fill in.
//! @return true if event was fetched, else false.
bool
PScreen::getNextEvent(XEvent &ev)
{
    if (XPending(_dpy) > 0) {
        XNextEvent(_dpy, &ev);
        return true;
    }

    int ret;
    fd_set rfds;

    XFlush(_dpy);

    FD_ZERO(&rfds);
    FD_SET(_fd, &rfds);

    ret = select(_fd + 1, &rfds, 0, 0, 0);
    if (ret > 0) {
        XNextEvent(_dpy, &ev);
    }

    return ret > 0;
}

//! @brief Grabs the server, counting number of grabs
bool
PScreen::grabServer(void)
{
    if (_server_grabs == 0) {
        XGrabServer(_dpy);
    }

    ++_server_grabs;
    return (_server_grabs == 1); // was actually grabbed
}

//! @brief Ungrabs the server, counting number of grabs
bool
PScreen::ungrabServer(bool sync)
{
    if (_server_grabs > 0) {
        --_server_grabs;

        if (_server_grabs == 0) { // no more grabs left
            if (sync) {
                XSync(_dpy, false);
            }
            XUngrabServer(_dpy);
        }
    }
    return (_server_grabs == 0); // is actually ungrabbed
}

//! @brief Grabs the keyboard
bool
PScreen::grabKeyboard(Window win)
{
    if (XGrabKeyboard(_dpy, win, false, GrabModeAsync, GrabModeAsync,
                      CurrentTime) == GrabSuccess) {
        return true;
    }
#ifdef DEBUG
    cerr << __FILE__ << "@" << __LINE__ << ": "
         << "PScreen(" << this << ")::grabKeyboard(" << win << ")" << endl
         << " *** unable to grab keyboard." << endl;
#endif // DEBUG
    return false;
}

//! @brief Ungrabs the keyboard
bool
PScreen::ungrabKeyboard(void)
{
    XUngrabKeyboard(_dpy, CurrentTime);
    return true;
}

//! @brief Grabs the pointer
bool
PScreen::grabPointer(Window win, uint event_mask, Cursor cursor)
{
    if (XGrabPointer(_dpy, win, false, event_mask, GrabModeAsync, GrabModeAsync,
                     None, cursor, CurrentTime) == GrabSuccess) {
        return true;
    }
#ifdef DEBUG
    cerr << __FILE__ << "@" << __LINE__ << ": "
         << "PScreen(" << this << ")::grabPointer(" << win << ","
         << event_mask << "," << cursor << ")" << endl
         << " *** unable to grab pointer." << endl;
#endif // DEBUG
    return false;
}

//! @brief Ungrabs the pointer
bool
PScreen::ungrabPointer(void)
{
    XUngrabPointer(_dpy, CurrentTime);
    return true;
}

//! @brief Refetches the root-window size.
void
PScreen::updateGeometry(uint width, uint height)
{
#ifdef HAVE_XRANDR
  if (! _honour_randr || ! _has_extension_xrandr) {
    return;
  }

  // The screen has changed geometry in some way. To handle this the
  // head information is read once again, the root window is re sized
  // and strut information is updated.
  initHeads();

  _screen_gm.width = width;
  _screen_gm.height = height;
  PWinObj::getRootPWinObj()->resize(width, height);

  updateStrut();
#endif // HAVE_XRANDR
}

//! @brief Searches for the head closest to the coordinates x,y.
//! @return The nearest head.  Head numbers are indexed from 0.
uint
PScreen::getNearestHead(int x, int y)
{
  if(_heads.size() > 1) {
        // set distance to the highest uint value
#ifdef HAVE_LIMITS
        uint min_distance = numeric_limits<uint>::max();
#else //! HAVE_LIMITS
        uint min_distance = ~0;
#endif // HAVE_LIMITS
        uint nearest_head = 0;

        uint distance;
        int head_t, head_b, head_l, head_r;
        for(uint head = 0; head < _heads.size(); ++head) {
            head_t = _heads[head].y;
            head_b = _heads[head].y + _heads[head].height;
            head_l = _heads[head].x;
            head_r = _heads[head].x + _heads[head].width;

            if(x > head_r) {
                if(y < head_t) {
                    // above and right of the head
                    distance = calcDistance(x, y, head_r, head_t);
                } else if(y > head_b) {
                    // below and right of the head
                    distance = calcDistance(x, y, head_r, head_b);
                } else {
                    // right of the head
                    distance = calcDistance(x, head_r);
                }
            } else if(x < head_l) {
                if(y < head_t) {
                    // above and left of the head
                    distance = calcDistance(x, y, head_l, head_t);
                } else if(y > head_b) {
                    // below and left of the head
                    distance = calcDistance(x, y, head_l, head_b);
                } else {
                    // left of the head
                    distance = calcDistance(x, head_l);
                }
            } else {
                if(y < head_t) {
                    // above the head
                    distance = calcDistance(y, head_t);
                } else if(y > head_b) {
                    // below the head
                    distance = calcDistance(y, head_b);
                } else {
                    // on the head
                    return head;
                }
            }

#ifdef DEBUG
            cerr << __FILE__ << "@" << __LINE__ << ": PScreen::getNearestHead( " << x << "," << y << ") "
                 << "head boundaries " << head_t << "," << head_b << "," << head_l << "," << head_r << " "
                 << "distance " << distance << " min_distance " << min_distance << endl;
#endif // DEBUG

            if(distance < min_distance) {
                min_distance = distance;
                nearest_head = head;
            }
        }
        return nearest_head;
    } else {
        return 0;
    }
}

//! @brief Searches for the head that the pointer currently is on.
//! @return Active head number
uint
PScreen::getCurrHead(void)
{
    uint head = 0;

    if (_heads.size() > 1) {
        int x = 0, y = 0;
        getMousePosition(x, y);
        head = getNearestHead(x, y);
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": PScreen::getCurrHead() got head "
             << head << " from mouse position " << x << "," << y << endl;
#endif // DEBUG
    }

    return head;
}

//! @brief Fills head_info with info about head nr head
//! @param head Head number to examine
//! @param head_info Returning info about the head
//! @return true if xinerama is off or head exists.
bool
PScreen::getHeadInfo(uint head, Geometry &head_info)
{
    if (head  < _heads.size()) {
        head_info.x = _heads[head].x;
        head_info.y = _heads[head].y;
        head_info.width = _heads[head].width;
        head_info.height = _heads[head].height;
        return true;
    } else {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": Head: " << head << " doesn't exist!" << endl;
#endif // DEBUG
        return false;
    }
}

/**
 * Same as getHeadInfo but returns Geometry instead of filling it in.
 */
Geometry
PScreen::getHeadGeometry(uint head)
{
  Geometry gm(_screen_gm);
  getHeadInfo(head, gm);
  return gm;
}

//! @brief Fill information about head and the strut.
void
PScreen::getHeadInfoWithEdge(uint num, Geometry &head)
{
    if (! getHeadInfo(num, head)) {
        return;
    }

    int strut_val;
    Strut strut(_heads[num].strut); // Convenience

    // Remove the strut area from the head info
    strut_val = (head.x == 0) ? std::max(_strut.left, strut.left) : strut.left;
    head.x += strut_val;
    head.width -= strut_val;  

    strut_val = ((head.x + head.width) == _screen_gm.width) ? std::max(_strut.right, strut.right) : strut.right;
    head.width -= strut_val;

    strut_val = (head.y == 0) ? std::max(_strut.top, strut.top) : strut.top;
    head.y += strut_val;
    head.height -= strut_val;

    strut_val = (head.y + head.height == _screen_gm.height) ? std::max(_strut.bottom, strut.bottom) : strut.bottom;
    head.height -= strut_val;
}

void
PScreen::getMousePosition(int &x, int &y)
{
    Window d_root, d_win;
    int win_x, win_y;
    uint mask;

    XQueryPointer(_dpy, _root, &d_root, &d_win, &x, &y, &win_x, &win_y, &mask);
}

uint
PScreen::getButtonFromState(uint state)
{
    uint button = 0;

    if (state&Button1Mask)
        button = BUTTON1;
    else if (state&Button2Mask)
        button = BUTTON2;
    else if (state&Button3Mask)
        button = BUTTON3;
    else if (state&Button4Mask)
        button = BUTTON4;
    else if (state&Button5Mask)
        button = BUTTON5;

    return button;
}

//! @brief Adds a strut to the strut list, updating max strut sizes
void
PScreen::addStrut(Strut *strut)
{
    assert(strut);
    _strut_list.push_back(strut);

    updateStrut();
}

//! @brief Removes a strut from the strut list
void
PScreen::removeStrut(Strut *strut)
{
    assert(strut);
    _strut_list.remove(strut);

    updateStrut();
}

//! @brief Updates strut max size.
void
PScreen::updateStrut(void)
{
    // Reset strut data.
    _strut.left = 0;
    _strut.right = 0;
    _strut.top = 0;
    _strut.bottom = 0;

    for (vector<Head>::iterator it(_heads.begin()); it != _heads.end(); ++it) {
      it->strut.left = 0;
      it->strut.right = 0;
      it->strut.top = 0;
      it->strut.bottom = 0;
    }

    Strut *strut;
    for(list<Strut*>::iterator it(_strut_list.begin()); it != _strut_list.end(); ++it) {
        if ((*it)->head < 0) {
            strut = &_strut;
        } else if (static_cast<uint>((*it)->head) < _heads.size()) {
            strut = &(_heads[(*it)->head].strut);
        } else {
            continue;
        }

        if (strut->left < (*it)->left) {
            strut->left = (*it)->left;
        }
        if (strut->right < (*it)->right) {
            strut->right = (*it)->right;
        }
        if (strut->top < (*it)->top) {
            strut->top = (*it)->top;
        }
        if (strut->bottom < (*it)->bottom) {
          strut->bottom = (*it)->bottom;
        }
    }

    // Update hints on the root window
    Geometry workarea(_strut.left, _strut.top,
                      _screen_gm.width - _strut.left - _strut.right, _screen_gm.height - _strut.top - _strut.bottom);

    static_cast<RootWO*>(PWinObj::getRootPWinObj())->setEwmhWorkarea(workarea);
}

//! @brief Initialize head information
void
PScreen::initHeads(void)
{
    _heads.clear();

    // Read head information, randr has priority over xinerama then
    // comes ordinary X11 information.

    initHeadsRandr();
    if (! _heads.size()) {
        initHeadsXinerama();

        if (! _heads.size()) {
            _heads.push_back(Head(0, 0, _screen_gm.width, _screen_gm.height));
        }
    }
}

//! @brief Initialize head information from Xinerama
void
PScreen::initHeadsXinerama(void)
{
#ifdef HAVE_XINERAMA
    // Check if there are heads already initialized from example Randr
    if (! XineramaIsActive(_dpy)) {
        return;
    }

    int num_heads = 0;
    XineramaScreenInfo *infos = XineramaQueryScreens(_dpy, &num_heads);

    for (int i = 0; i < num_heads; ++i) {
        _heads.push_back(Head(infos[i].x_org, infos[i].y_org, infos[i].width, infos[i].height));
    }

    XFree(infos);
#endif // HAVE_XINERAMA
}

//! @brief Initialize head information from RandR
void
PScreen::initHeadsRandr(void)
{
#ifdef HAVE_XRANDR
    if (! _honour_randr || ! _has_extension_xrandr) {
        return;
    }

    XRRScreenResources *resources = XRRGetScreenResources(_dpy, _root);
    if (! resources) {
        return;
    }

    for (int i = 0; i < resources->noutput; ++i) {
        XRROutputInfo *output = XRRGetOutputInfo(_dpy, resources, resources->outputs[i]);

        if (output->crtc) {
            XRRCrtcInfo *crtc = XRRGetCrtcInfo(_dpy, resources, output->crtc);

            _heads.push_back(Head(crtc->x, crtc->y, crtc->width, crtc->height));
#ifdef DEBUG
            cerr << __FILE__ << "@" << __LINE__ << ": PScreen::initHeadsRandr() added head "
                << crtc->x << "," << crtc->y << "," << crtc->width << "," << crtc->height << endl;
#endif // DEBUG

            XRRFreeCrtcInfo (crtc);
        }

        XRRFreeOutputInfo (output);
    }

    XRRFreeScreenResources (resources);
#endif // HAVE_XRANDR
}

/**
 * Lookup mask from keycode.
 *
 * @param keycode KeyCode to lookup.
 * @return Mask for keycode, 0 if something fails.
 */
uint
PScreen::getMaskFromKeycode(KeyCode keycode)
{
    // Make sure modifier mappings were looked up ok
    if (! _modifier_map || _modifier_map->max_keypermod < 1) {
        return 0;
    }

    // .h files states that modifiermap is an 8 * max_keypermod array.
    int max_info = _modifier_map->max_keypermod * 8;
    for (int i = 0; i < max_info; ++i) {
        if (_modifier_map->modifiermap[i] == keycode) {
            return MODIFIER_TO_MASK[i / _modifier_map->max_keypermod];
        }
    }

    return 0;
}

/**
 * Figure out what key you can press to generate mask
 *
 * @param mask Modifier mask to get keycode for.
 * @return KeyCode for mask, 0 if failing.
 */
KeyCode
PScreen::getKeycodeFromMask(uint mask)
{
    // Make sure modifier mappings were looked up ok
    if (! _modifier_map || _modifier_map->max_keypermod < 1) {
        return 0;
    }

    for (int i = 0; i < 8; ++i) {
        if (MODIFIER_TO_MASK[i] == mask) {
            // FIXME: Is iteration over the range required?
            return _modifier_map->modifiermap[i * _modifier_map->max_keypermod];
        }
    }

    return 0;
}
