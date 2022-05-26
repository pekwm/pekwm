//
// X11.cc for pekwm
// Copyright (C) 2022 Claes Nästén
// Copyright (C) 2009-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <string>
#include <iostream>
#include <cassert>
#ifdef PEKWM_HAVE_LIMITS
#include <limits>
#endif // PEKWM_HAVE_LIMITS

extern "C" {
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#ifdef PEKWM_HAVE_SHAPE
#include <X11/extensions/shape.h>
#endif // PEKWM_HAVE_SHAPE
#ifdef PEKWM_HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif // PEKWM_HAVE_XRANDR
#include <X11/keysym.h> // For XK_ entries
#ifdef PEKWM_HAVE_X11_XKBLIB_H
#include <X11/XKBlib.h>
#endif
	bool xerrors_ignore = false;

	unsigned int xerrors_count = 0;
}

#include "X11.hh"
#include "Debug.hh"
#include "Util.hh"

const uint X11::MODIFIER_TO_MASK[] = {
	ShiftMask, LockMask, ControlMask,
	Mod1Mask, Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
};
const uint X11::MODIFIER_TO_MASK_NUM =
	sizeof(X11::MODIFIER_TO_MASK) /
	sizeof(X11::MODIFIER_TO_MASK[0]);
Atom X11::_atoms[MAX_NR_ATOMS];

extern "C" {
	/**
	 * Invoked after all Xlib calls if run in synchronous mode.
	 */
	static int afterXlibCall(Display *dpy)
	{
		static uint last_xerrors_count = 0;
		if (xerrors_count != last_xerrors_count) {
			last_xerrors_count = xerrors_count;
		}
		return 0;
	}

	/**
	 * XError handler, prints error.
	 */
	static int
	handleXError(Display *dpy, XErrorEvent *ev)
	{
		if (xerrors_ignore) {
			return 0;
		}

		++xerrors_count;
		if (Debug::getLevel() >= Debug::LEVEL_TRACE) {
			char error_buf[256];
			XGetErrorText(dpy, ev->error_code, error_buf, 256);
			P_TRACE("XError: " << error_buf << " id: " << ev->resourceid);
		}

		return 0;
	}
}

static const char *atomnames[] = {
	// EWMH atoms
	"_NET_SUPPORTED",
	"_NET_CLIENT_LIST", "_NET_CLIENT_LIST_STACKING",
	"_NET_NUMBER_OF_DESKTOPS",
	"_NET_DESKTOP_GEOMETRY", "_NET_DESKTOP_VIEWPORT",
	"_NET_CURRENT_DESKTOP", "_NET_DESKTOP_NAMES",
	"_NET_ACTIVE_WINDOW", "_NET_WORKAREA",
	"_NET_DESKTOP_LAYOUT", "_NET_SUPPORTING_WM_CHECK",
	"_NET_CLOSE_WINDOW",
	"_NET_WM_MOVERESIZE",
	"_NET_REQUEST_FRAME_EXTENTS",
	"_NET_WM_NAME", "_NET_WM_VISIBLE_NAME",
	"_NET_WM_ICON_NAME", "_NET_WM_VISIBLE_ICON_NAME",
	"_NET_WM_ICON", "_NET_WM_DESKTOP",
	"_NET_WM_STRUT", "_NET_WM_PID",
	"_NET_WM_USER_TIME",
	"_NET_FRAME_EXTENTS",
	"_NET_WM_WINDOW_OPACITY",

	"_NET_WM_WINDOW_TYPE",
	"_NET_WM_WINDOW_TYPE_DESKTOP",
	"_NET_WM_WINDOW_TYPE_DOCK",
	"_NET_WM_WINDOW_TYPE_TOOLBAR",
	"_NET_WM_WINDOW_TYPE_MENU",
	"_NET_WM_WINDOW_TYPE_UTILITY",
	"_NET_WM_WINDOW_TYPE_SPLASH",
	"_NET_WM_WINDOW_TYPE_DIALOG",
	"_NET_WM_WINDOW_TYPE_DROPDOWN_MENU",
	"_NET_WM_WINDOW_TYPE_POPUP_MENU",
	"_NET_WM_WINDOW_TYPE_TOOLTIP",
	"_NET_WM_WINDOW_TYPE_NOTIFICATION",
	"_NET_WM_WINDOW_TYPE_COMBO",
	"_NET_WM_WINDOW_TYPE_DND",
	"_NET_WM_WINDOW_TYPE_NORMAL",

	"_NET_WM_STATE",
	"_NET_WM_STATE_MODAL", "_NET_WM_STATE_STICKY",
	"_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_STATE_MAXIMIZED_HORZ",
	"_NET_WM_STATE_SHADED",
	"_NET_WM_STATE_SKIP_TASKBAR", "_NET_WM_STATE_SKIP_PAGER",
	"_NET_WM_STATE_HIDDEN", "_NET_WM_STATE_FULLSCREEN",
	"_NET_WM_STATE_ABOVE", "_NET_WM_STATE_BELOW",
	"_NET_WM_STATE_DEMANDS_ATTENTION",

	"_NET_WM_ALLOWED_ACTIONS",
	"_NET_WM_ACTION_MOVE", "_NET_WM_ACTION_RESIZE",
	"_NET_WM_ACTION_MINIMIZE", "_NET_WM_ACTION_SHADE",
	"_NET_WM_ACTION_STICK",
	"_NET_WM_ACTION_MAXIMIZE_VERT", "_NET_WM_ACTION_MAXIMIZE_HORZ",
	"_NET_WM_ACTION_FULLSCREEN", "_NET_WM_ACTION_CHANGE_DESKTOP",
	"_NET_WM_ACTION_CLOSE",

	"UTF8_STRING",
	"STRING",
	"MANAGER",

	// pekwm atoms
	"_PEKWM_FRAME_ID",
	"_PEKWM_FRAME_ORDER",
	"_PEKWM_FRAME_ACTIVE",
	"_PEKWM_FRAME_DECOR",
	"_PEKWM_FRAME_SKIP",
	"_PEKWM_TITLE",
	"_PEKWM_BG_PID",
	"_PEKWM_CMD",
	"_PEKWM_THEME",

	// ICCCM atoms
	"WM_NAME",
	"WM_ICON_NAME",
	"WM_HINTS",
	"WM_CLASS",
	"WM_STATE",
	"WM_CHANGE_STATE",
	"WM_PROTOCOLS",
	"WM_DELETE_WINDOW",
	"WM_COLORMAP_WINDOWS",
	"WM_TAKE_FOCUS",
	"WM_WINDOW_ROLE",
	"WM_CLIENT_MACHINE",

	// miscellaneous atoms
	"_MOTIF_WM_HINTS",

	"_XROOTPMAP_ID",
	"_XSETROOT_ID",
};

std::ostream&
operator<<(std::ostream& os, const Geometry& gm)
{
	os << "Geometry";
	os << " x:" << gm.x << " y: " << gm.y;
	os << " width: " << gm.width << " height: " << gm.height;
	return os;
}

Geometry::Geometry(void)
	: x(0),
	  y(0),
	  width(1),
	  height(1)
{
}

Geometry::Geometry(int _x, int _y, unsigned int _width, unsigned int _height)
	: x(_x),
	  y(_y),
	  width(_width),
	  height(_height)
{
}

Geometry::Geometry(const Geometry &gm)
	: x(gm.x),
	  y(gm.y),
	  width(gm.width),
	  height(gm.height)
{
}

Geometry::~Geometry(void)
{
}

/**
 * Update provided gm to be centered within this geometry.
 */
Geometry
Geometry::center(Geometry gm) const
{
	gm.x = x + (width / 2) - (gm.width / 2);
	gm.y = y + (height / 2) - (gm.height / 2);
	return gm;
}

Geometry&
Geometry::operator=(const Geometry& gm)
{
	x = gm.x;
	y = gm.y;
	width = gm.width;
	height = gm.height;
	return *this;
}

bool
Geometry::operator==(const Geometry& gm)
{
	return ((x == gm.x) && (y == gm.y) &&
		(width == gm.width) && (height == gm.height));
}

bool
Geometry::operator!=(const Geometry& gm)
{
	return (x != gm.x) || (y != gm.y) ||
		(width != gm.width) || (height != gm.height);
}

int
Geometry::diffMask(const Geometry &old_gm)
{
	int mask = 0;
	if (x != old_gm.x) {
		mask |= X_VALUE;
	}
	if (y != old_gm.y) {
		mask |= Y_VALUE;
	}
	if (width != old_gm.width) {
		mask |= WIDTH_VALUE;
	}
	if (height != old_gm.height) {
		mask |= HEIGHT_VALUE;
	}
	return mask;
}

Strut::Strut(long l, long r, long t, long b, int nhead)
	: left(l),
	  right(r),
	  top(t),
	  bottom(b),
	  head(nhead)
{
}

Strut::~Strut(void)
{
}

bool
Strut::isSet(void) const
{
	return left != 0 || right != 0 || top != 0 || bottom != 0;
}

void
Strut::clear(void)
{
	left = 0;
	right = 0;
	top = 0;
	bottom = 0;
}

void
Strut::operator=(const long *s)
{
	left = s[0];
	right = s[1];
	top = s[2];
	bottom = s[3];
}

bool
Strut::operator==(const Strut& rhs) const
{
	return left == rhs.left
		&& right == rhs.right
		&& top == rhs.top
		&& bottom == rhs.bottom
		&& head == rhs.head;
}

bool
Strut::operator!=(const Strut& rhs) const
{
	return !operator==(rhs);
}

std::ostream& operator<<(std::ostream &stream, const Strut &strut)
{
	stream << "Strut l: " << strut.left << " r: " << strut.right;
	stream << " t: " << strut.top << " b: " << strut.bottom;
	stream << " head " << strut.head;
	return stream;
}

/**
 * Helper class for XColor.
 */
class X11::ColorEntry {
public:
	ColorEntry(const std::string &name) : _name(name), _ref(0) { }
	~ColorEntry(void) { }

	inline XColor *getColor(void) { return &_xc; }

	inline uint getRef(void) const { return _ref; }
	inline void incRef(void) { _ref++; }
	inline void decRef(void) { if (_ref > 0) { _ref--; } }

	inline bool operator==(const std::string &name) {
		return StringUtil::ascii_ncase_equal(_name, name);
	}

private:
	std::string _name;
	XColor _xc;
	uint _ref;
};

/**
 * Init X11 connection, must be called before any X11:: call is made.
 */
void
X11::init(Display *dpy, bool synchronous, bool honour_randr)
{
	if (_dpy) {
		throw std::string("X11, trying to create multiple instances");
	}

	_dpy = dpy;
	_honour_randr = honour_randr;

	if (synchronous) {
		XSynchronize(_dpy, True);
		XSetAfterFunction(_dpy, afterXlibCall);
	}
	XSetErrorHandler(handleXError);

	grabServer();

	_fd = ConnectionNumber(dpy);
	_screen = DefaultScreen(_dpy);
	_root = RootWindow(_dpy, _screen);

	_depth = DefaultDepth(_dpy, _screen);
	_visual = DefaultVisual(_dpy, _screen);
	_gc = DefaultGC(_dpy, _screen);
	XGCValues gv;
	gv.function = GXcopy;
	XChangeGC(_dpy, _gc, GCFunction, &gv);
	_colormap = DefaultColormap(_dpy, _screen);
	_modifier_map = XGetModifierMapping(_dpy);

	_screen_gm.width = WidthOfScreen(ScreenOfDisplay(_dpy, _screen));
	_screen_gm.height = HeightOfScreen(ScreenOfDisplay(_dpy, _screen));

	// create resize cursors
	_cursor_map[CURSOR_TOP_LEFT] = XCreateFontCursor(_dpy, XC_top_left_corner);
	_cursor_map[CURSOR_TOP] = XCreateFontCursor(_dpy, XC_top_side);
	_cursor_map[CURSOR_TOP_RIGHT] =
		XCreateFontCursor(_dpy, XC_top_right_corner);
	_cursor_map[CURSOR_LEFT] = XCreateFontCursor(_dpy, XC_left_side);
	_cursor_map[CURSOR_RIGHT] = XCreateFontCursor(_dpy, XC_right_side);
	_cursor_map[CURSOR_BOTTOM_LEFT] =
		XCreateFontCursor(_dpy, XC_bottom_left_corner);
	_cursor_map[CURSOR_BOTTOM] = XCreateFontCursor(_dpy, XC_bottom_side);
	_cursor_map[CURSOR_BOTTOM_RIGHT] =
		XCreateFontCursor(_dpy, XC_bottom_right_corner);
	// create other cursors
	_cursor_map[CURSOR_ARROW] = XCreateFontCursor(_dpy, XC_left_ptr);
	_cursor_map[CURSOR_MOVE]  = XCreateFontCursor(_dpy, XC_fleur);
	_cursor_map[CURSOR_RESIZE] = XCreateFontCursor(_dpy, XC_plus);

#ifdef PEKWM_HAVE_SHAPE
	{
		int dummy_error;
		_has_extension_shape =
			XShapeQueryExtension(_dpy, &_event_shape, &dummy_error);
	}
#endif // PEKWM_HAVE_SHAPE

#ifdef PEKWM_HAVE_XRANDR
	{
		int dummy_error;
		_has_extension_xrandr =
			XRRQueryExtension(_dpy, &_event_xrandr, &dummy_error);
	}
#endif // PEKWM_HAVE_XRANDR

#ifdef PEKWM_HAVE_X11_XKBLIB_H
	{
		int major = XkbMajorVersion;
		int minor = XkbMinorVersion;
		int ext_opcode, ext_ev, ext_err;
		_has_extension_xkb =
			XkbQueryExtension(_dpy, &ext_opcode, &ext_ev, &ext_err,
					  &major, &minor);
	}
#endif // PEKWM_HAVE_X11_XKBLIB_H

	// Now screen geometry has been read and extensions have been
	// looked for, read head information.
	initHeads();

	// initialize array values
	for (uint i = 0; i < (BUTTON_NO - 1); ++i) {
		_last_click_time[i] = 0;
	}

	// Figure out what keys the Num and Scroll Locks are
	setLockKeys();

	ungrabServer(true);

	_xc_default.pixel = BlackPixel(_dpy, _screen);
	_xc_default.red = _xc_default.green = _xc_default.blue = 0;

	assert(sizeof(atomnames)/sizeof(char*) == MAX_NR_ATOMS);
	if (! XInternAtoms(_dpy, const_cast<char**>(atomnames), MAX_NR_ATOMS,
			   0, _atoms)) {
		P_ERR("XInternAtoms did not return all requested atoms");
	}
}

//! @brief X11 destructor
void
X11::destruct(void) {
	if (_colors.size() > 0) {
		ulong *pixels = new ulong[_colors.size()];
		for (uint i=0; i < _colors.size(); ++i) {
			pixels[i] = _colors[i]->getColor()->pixel;
			delete _colors[i];
		}
		XFreeColors(_dpy, X11::getColormap(),
			    pixels, _colors.size(), 0);
		delete [] pixels;
	}

	if (_modifier_map) {
		XFreeModifiermap(_modifier_map);
	}

	for (Cursor i = CURSOR_0; i != CURSOR_NONE; i++) {
		XFreeCursor(_dpy, _cursor_map[i]);
	}

	// Under certain circumstances trying to restart pekwm can cause it to
	// use 100% of the CPU without making any progress with the restart.
	// This X11:sync() seems to be work around the issue (c.f. #300).
	X11::sync(True);

	XCloseDisplay(_dpy);
	_dpy = 0;
}

XColor *
X11::getColor(const std::string &color)
{
	if (StringUtil::ascii_ncase_equal(color.c_str(), "EMPTY")) {
		return &_xc_default;
	}

	std::vector<ColorEntry*>::iterator it = _colors.begin();
	for (; it != _colors.end(); ++it) {
		if (*(*it) == color) {
			(*it)->incRef();
			return (*it)->getColor();
		}
	}

	// create new entry
	ColorEntry *entry = new ColorEntry(color);

	// X alloc
	XColor dummy;
	if (XAllocNamedColor(_dpy, X11::getColormap(),
			     color.c_str(), entry->getColor(), &dummy) == 0) {
		P_ERR("failed to alloc color: " << color);
		delete entry;
		entry = 0;
	} else {
		_colors.push_back(entry);
		entry->incRef();
	}

	return entry ? entry->getColor() : &_xc_default;
}

void
X11::returnColor(XColor*& xc)
{
	if (&_xc_default == xc) { // no need to return default color
		return;
	}

	std::vector<ColorEntry*>::iterator it = _colors.begin();
	for (; it != _colors.end(); ++it) {
		if ((*it)->getColor() == xc) {
			(*it)->decRef();
			if (((*it)->getRef() == 0)) {
				ulong pixels[1] = { (*it)->getColor()->pixel };
				XFreeColors(X11::getDpy(), X11::getColormap(), pixels, 1, 0);

				delete *it;
				_colors.erase(it);
			}
			break;
		}
	}

	xc = nullptr;
}

ulong
X11::getWhitePixel(void)
{
	return WhitePixel(_dpy, _screen);
}

ulong
X11::getBlackPixel(void)
{
	return BlackPixel(_dpy, _screen);
}

void
X11::free(void* data)
{
	XFree(data);
}

void
X11::warpPointer(int x, int y)
{
	if (_dpy) {
		XWarpPointer(_dpy, None, _root, 0, 0, 0, 0, x, y);
	}
}

void
X11::moveWindow(Window win, int x, int y)
{
	if (_dpy) {
		XMoveWindow(_dpy, win, x, y);
	}
}
void
X11::resizeWindow(Window win,
                  unsigned int width, unsigned int height)
{
	if (_dpy) {
		XResizeWindow(_dpy, win, width, height);
	}
}

void
X11::moveResizeWindow(Window win, int x, int y,
                      unsigned int width, unsigned int height)
{
	if (_dpy) {
		XMoveResizeWindow(_dpy, win, x, y, width, height);
	}
}

/**
 * Remove state modifiers such as NumLock from state.
 */
void
X11::stripStateModifiers(unsigned int *state)
{
	*state &= ~(_num_lock | _scroll_lock | LockMask |
		    KbdLayoutMask1 | KbdLayoutMask2);
}

/**
 * Remove button modifiers from state.
 */
void
X11::stripButtonModifiers(unsigned int *state)
{
	*state &= ~(Button1Mask | Button2Mask | Button3Mask |
		    Button4Mask | Button5Mask);
}

/**
 * Figure out what keys the Num and Scroll Locks are
 */
void
X11::setLockKeys(void)
{
	_num_lock = getMaskFromKeycode(XKeysymToKeycode(_dpy, XK_Num_Lock));
	_scroll_lock = getMaskFromKeycode(XKeysymToKeycode(_dpy, XK_Scroll_Lock));
}

void
X11::flush(void)
{
	if (_dpy) {
		XFlush(_dpy);
	}
}

int
X11::pending(void)
{
	if (_dpy) {
		return XPending(_dpy);
	}
	return 0;
}

/**
 * Get next event using select to avoid signal blocking
 *
 * @param ev Event to fill in.
 * @return true if event was fetched, else false.
 */
bool
X11::getNextEvent(XEvent &ev, struct timeval *timeout)
{
	// A call to flush was previously used when no pending events was
	// found, however accoarding to the XFlush man page XPending does
	// flush by itself.
	//
	// This reportedly fixes lockups and the change was suggested
	// by Christian Zander
	if (pending()) {
		XNextEvent(_dpy, &ev);
		return true;
	}

	int ret;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(_fd, &rfds);

	ret = select(_fd + 1, &rfds, nullptr, nullptr, timeout);
	if (ret > 0) {
		XNextEvent(_dpy, &ev);
	}

	return ret > 0;
}

void
X11::allowEvents(int event_mode, Time time)
{
	if (_dpy) {
		XAllowEvents(_dpy, event_mode, time);
	}
}

//! @brief Grabs the server, counting number of grabs
bool
X11::grabServer(void)
{
	if (_server_grabs == 0) {
		P_TRACE("grabbing server");
		XGrabServer(_dpy);
		++_server_grabs;
	} else {
		++_server_grabs;
	}
	return _server_grabs == 1;
}

//! @brief Ungrabs the server, counting number of grabs
bool
X11::ungrabServer(bool sync)
{
	if (_server_grabs > 0) {
		if (--_server_grabs == 0) {
			if (sync) {
				P_TRACE("0 server grabs left, syncing and ungrab.");
				X11::sync(False);
			} else {
				P_TRACE("0 server grabs left, ungrabbing server.");
			}
			XUngrabServer(_dpy);
		}
	}
	return _server_grabs == 0;
}

//! @brief Grabs the keyboard
bool
X11::grabKeyboard(Window win)
{
	P_TRACE("grabbing keyboard");
	if (XGrabKeyboard(_dpy, win, false, GrabModeAsync, GrabModeAsync,
			  CurrentTime) == GrabSuccess) {
		return true;
	}
	P_ERR("failed to grab keyboard on " << win);
	return false;
}

//! @brief Ungrabs the keyboard
bool
X11::ungrabKeyboard(void)
{
	P_TRACE("ungrabbing keyboard");
	XUngrabKeyboard(_dpy, CurrentTime);
	return false;
}

//! @brief Grabs the pointer
bool
X11::grabPointer(Window win, uint event_mask, CursorType type)
{
	P_TRACE("grabbing pointer");
	Cursor cursor = type < CURSOR_NONE ? _cursor_map[type] : None;
	if (XGrabPointer(_dpy, win, false, event_mask, GrabModeAsync, GrabModeAsync,
			 None, cursor, CurrentTime) == GrabSuccess) {
		return true;
	}
	P_ERR("failed to grab pointer on " << win
	      << ", event_mask " << event_mask
	      << ", type " << type);
	return false;
}

//! @brief Ungrabs the pointer
bool
X11::ungrabPointer(void)
{
	P_TRACE("ungrabbing pointer");
	XUngrabPointer(_dpy, CurrentTime);
	return false;
}

/**
 * Refetches the root-window size.
 */
bool
X11::updateGeometry(uint width, uint height)
{
#ifdef PEKWM_HAVE_XRANDR
	if (! _honour_randr || ! _has_extension_xrandr) {
		return false;
	}

	// The screen has changed geometry in some way. To handle this the
	// head information is read once again, the root window is re sized
	// and strut information is updated.
	initHeads();

	_screen_gm.width = width;
	_screen_gm.height = height;
	return true;
#else // ! PEKWM_HAVE_XRANDR
	return false;
#endif // PEKWM_HAVE_XRANDR
}

bool
X11::selectXRandrInput(void)
{
#ifdef PEKWM_HAVE_XRANDR
	if (_honour_randr && _has_extension_xrandr) {
		XRRSelectInput(_dpy, _root, RRScreenChangeNotifyMask);
		return true;
	}
#endif // PEKWM_HAVE_XRANDR
	return false;
}

bool
X11::getScreenChangeNotification(XEvent *ev, ScreenChangeNotification &scn)
{
#ifdef PEKWM_HAVE_XRANDR
	if (_honour_randr
	    && _has_extension_xrandr
	    && ev->type == _event_xrandr + RRScreenChangeNotify) {
		XRRScreenChangeNotifyEvent* scr_ev =
			reinterpret_cast<XRRScreenChangeNotifyEvent*>(ev);
		if  (scr_ev->rotation == RR_Rotate_90
		     || scr_ev->rotation == RR_Rotate_270) {
			scn.width = scr_ev->height;
			scn.height = scr_ev->width;
		} else {
			scn.width = scr_ev->width;
			scn.height = scr_ev->height;
		}
		return true;
	}
#endif // PEKWM_HAVE_XRANDR
	return false;
}

//! @brief Searches for the head closest to the coordinates x,y.
//! @return The nearest head.  Head numbers are indexed from 0.
uint
X11::getNearestHead(int x, int y)
{
	if(_heads.size() > 1) {
		// set distance to the highest uint value
		uint min_distance = std::numeric_limits<uint>::max();
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

/**
 * Searches for the head that the pointer currently is on.
 *
 * @return Head number for the head that the cursor is on.
 */
uint
X11::getCursorHead(void)
{
	uint head = 0;

	if (_heads.size() > 1) {
		int x = 0, y = 0;
		getMousePosition(x, y);
		head = getNearestHead(x, y);
	}

	return head;
}


void
X11::addHead(const Head &head)
{
	_heads.push_back(head);
}

/**
 * Fills head_info with info about head nr head
 *
 * @param head Head number to examine
 * @param head_info Returning info about the head
 * @return true if xinerama is off or head exists.
 */
bool
X11::getHeadInfo(uint head, Geometry &head_info)
{
	if (head  < _heads.size()) {
		head_info.x = _heads[head].x;
		head_info.y = _heads[head].y;
		head_info.width = _heads[head].width;
		head_info.height = _heads[head].height;
		return true;
	} else {
		P_ERR("head " << head << " does not exist");
		return false;
	}
}

/**
 * Fill head_info with info about head at x/y.
 */
void
X11::getHeadInfo(int x, int y, Geometry &head_info)
{
	std::vector<Head>::iterator it = _heads.begin();
	for (; it != _heads.end(); ++it) {
		if (x >= it->x && x <= signed(it->x + it->width)
		    && y >= it->y && y <= signed(it->y + it->height)) {
			head_info.x = it->x;
			head_info.y = it->y;
			head_info.width = it->width;
			head_info.height = it->height;
			return;
		}
	}
	head_info = _screen_gm;
}

/**
 * Same as getHeadInfo but returns Geometry instead of filling it in.
 */
Geometry
X11::getHeadGeometry(uint head)
{
	Geometry gm(_screen_gm);
	getHeadInfo(head, gm);
	return gm;
}

int
X11::getNumHeads(void)
{
	return _heads.size();
}

const char*
X11::getAtomString(AtomName name)
{
	return atomnames[name];
}

std::string
X11::getAtomIdString(Atom id)
{
	std::string name;
	char *c_name = XGetAtomName(_dpy, id);
	if (c_name != nullptr) {
		name = c_name;
		XFree(c_name);
	}
	return name;
}

AtomName
X11::getAtomName(Atom atom)
{
	for (int i = 0; i < MAX_NR_ATOMS; i++) {
		if (_atoms[i] == atom) {
			return static_cast<AtomName>(i);
		}
	}
	return MAX_NR_ATOMS;
}

void
X11::setAtom(Window win, AtomName aname, AtomName value)
{
	changeProperty(win, _atoms[aname], XA_ATOM, 32,
		       PropModeReplace, (uchar *) &_atoms[value], 1);
}

void
X11::setAtoms(Window win, AtomName aname, Atom *values, int size)
{
	changeProperty(win, _atoms[aname], XA_ATOM, 32,
		       PropModeReplace, (uchar *) values, size);
}

void
X11::setEwmhAtomsSupport(Window win)
{
	changeProperty(win, _atoms[NET_SUPPORTED], XA_ATOM, 32,
		       PropModeReplace, (uchar *) _atoms, UTF8_STRING+1);
}

bool
X11::getWindow(Window win, AtomName aname, Window& value)
{
	uchar *udata = 0;
	if (getProperty(win, _atoms[aname], XA_WINDOW, 1L, &udata, 0)) {
		value = *reinterpret_cast<Window*>(udata);
		X11::free(udata);
		return true;
	}
	return false;
}

void
X11::setWindow(Window win, AtomName aname, Window value)
{
	changeProperty(win, _atoms[aname], XA_WINDOW, 32,
		       PropModeReplace, (uchar *) &value, 1);
}

void
X11::setWindows(Window win, AtomName aname, Window *values,
                int size)
{
	changeProperty(win, _atoms[aname], XA_WINDOW, 32,
		       PropModeReplace, (uchar *) values, size);
}

bool
X11::getCardinal(Window win, AtomName aname, Cardinal &value, long format)
{
	uchar *udata = nullptr;
	if (getProperty(win, _atoms[aname], format, 1L, &udata, 0)) {
		value = *reinterpret_cast<Cardinal*>(udata);
		X11::free(udata);
		return true;
	}
	return false;
}

void
X11::setCardinals(Window win, AtomName aname,
                  Cardinal *values, int num)
{
	changeProperty(win, _atoms[aname], XA_CARDINAL, 32, PropModeReplace,
		       reinterpret_cast<uchar*>(values), num);
}

bool
X11::getUtf8String(Window win, AtomName aname, std::string &value)
{
	return getUtf8StringId(win, _atoms[aname], value);
}

bool
X11::getUtf8StringId(Window win, Atom id, std::string &value)
{
	uchar *data = nullptr;
	if (getProperty(win, id, _atoms[UTF8_STRING], 0, &data, 0)) {
		value = std::string(reinterpret_cast<char*>(data));
		X11::free(data);
		return true;
	}
	return false;
}

void
X11::setUtf8String(Window win, AtomName aname,
                   const std::string &value)
{
	changeProperty(win, _atoms[aname], _atoms[UTF8_STRING], 8,
		       PropModeReplace,
		       reinterpret_cast<const uchar*>(value.c_str()),
		       value.size());
}

void
X11::setUtf8StringArray(Window win, AtomName aname,
                        unsigned char *values, uint length)
{
	changeProperty(win, _atoms[aname], _atoms[UTF8_STRING], 8,
		       PropModeReplace, values, length);
}

bool
X11::getString(Window win, AtomName aname, std::string &value)
{
	return X11::getStringId(win, _atoms[aname], value);
}

bool
X11::getStringId(Window win, Atom id, std::string &value)
{
	uchar *data = 0;
	long uint actual;
	if (getProperty(win, id, XA_STRING, 0, &data, &actual)) {
		value = std::string((const char*) data);
		while (value.size() < actual) {
			value += ',';
			value += ((const char*) data) + value.size();
		}
		X11::free(data);
		return true;
	}
	return false;
}

void
X11::setString(Window win, AtomName aname, const std::string &value)
{
	changeProperty(win, _atoms[aname], XA_STRING, 8, PropModeReplace,
		       (uchar*)value.c_str(), value.size());
}

bool
X11::listProperties(Window win, std::vector<Atom>& atoms)
{
	if (! _dpy) {
		return false;
	}

	int num_props;
	Atom *c_atoms = XListProperties(_dpy, win, &num_props);
	if (c_atoms) {
		for (int i = 0; i < num_props; i++) {
			atoms.push_back(c_atoms[i]);
		}
		XFree(c_atoms);
	}
	return c_atoms != nullptr;
}

bool
X11::getProperty(Window win, Atom atom, Atom type,
                 ulong expected, uchar **data_ret, ulong *actual)
{
	if (expected == 0) {
		expected = 1024;
	}

	uchar *data = nullptr;
	ulong read = 0, left = 0;
	do {
		if (data != nullptr) {
			X11::free(data);
			data = nullptr;
		}
		expected += left;

		Atom r_type;
		int r_format, status;
		status =
			XGetWindowProperty(_dpy, win, atom,
					   0L, expected, False, type,
					   &r_type, &r_format, &read, &left, &data);
		if (status != Success || type != r_type || read == 0) {
			if (data != nullptr) {
				X11::free(data);
				data = nullptr;
			}
			left = 0;
		}
	} while (left);

	if (actual) {
		*actual = read;
	}
	*data_ret = data;

	return data != nullptr;
}

bool
X11::getTextProperty(Window win, Atom atom, std::string &value)
{
	// Read text property, return if it fails.
	XTextProperty text_property;
	if (! XGetTextProperty(_dpy, win, &text_property, atom)
	    || ! text_property.value || ! text_property.nitems) {
		return false;
	}

	if (text_property.encoding == XA_STRING) {
		value = reinterpret_cast<const char*>(text_property.value);
	} else {
		char **mb_list;
		int num;

		XmbTextPropertyToTextList(_dpy, &text_property, &mb_list, &num);
		if (mb_list && num > 0) {
			value = *mb_list;
			XFreeStringList(mb_list);
		}
	}

	X11::free(text_property.value);

	return true;
}

void*
X11::getEwmhPropData(Window win, AtomName prop, Atom type, int &num)
{
	Atom type_ret;
	int format_ret;
	ulong items_ret, after_ret;
	uchar *prop_data = 0;

	XGetWindowProperty(_dpy, win, _atoms[prop], 0, 0x7fffffff,
			   False, type, &type_ret, &format_ret, &items_ret,
			   &after_ret, &prop_data);
	num = items_ret;
	return prop_data;
}

void
X11::unsetProperty(Window win, AtomName aname)
{
	if (_dpy) {
		XDeleteProperty(_dpy, win, _atoms[aname]);
	}
}

void
X11::getMousePosition(int &x, int &y)
{
	Window d_root, d_win;
	int win_x, win_y;
	uint mask;

	XQueryPointer(_dpy, _root, &d_root, &d_win, &x, &y, &win_x, &win_y, &mask);
}

uint
X11::getButtonFromState(uint state)
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

int
X11::sendEvent(Window dest, Window win, Atom atom, long mask,
               long v1, long v2, long v3, long v4, long v5)
{
	XEvent e;
	e.type = e.xclient.type = ClientMessage;
	e.xclient.serial = 0;
	e.xclient.send_event = True;
	e.xclient.message_type = atom;
	e.xclient.window = win;
	e.xclient.format = 32;

	e.xclient.data.l[0] = v1;
	e.xclient.data.l[1] = v2;
	e.xclient.data.l[2] = v3;
	e.xclient.data.l[3] = v4;
	e.xclient.data.l[4] = v5;

	return sendEvent(dest, False, mask, &e);
}

int
X11::sendEvent(Window dest, Bool propagate, long mask, XEvent *ev)
{
	if (_dpy) {
		return XSendEvent(_dpy, dest, propagate, mask, ev);
	}
	return BadValue;
}

void
X11::setCardinal(Window win, AtomName aname, Cardinal value, long format)
{
	changeProperty(win, _atoms[aname], format, 32,
		       PropModeReplace, reinterpret_cast<uchar*>(&value), 1);
}

int
X11::changeProperty(Window win, Atom prop, Atom type, int format,
                    int mode, const unsigned char *data, int num_e)
{
	if (_dpy) {
		return XChangeProperty(_dpy, win, prop, type, format, mode,
				       data, num_e);
	}
	return BadImplementation;
}

int
X11::getGeometry(Window win, unsigned *w, unsigned *h, unsigned *bw)
{
	Window wn;
	int x, y;
	unsigned int depth_return;
	if (_dpy) {
		return XGetGeometry(_dpy, win, &wn, &x, &y,
				    w, h, bw, &depth_return);
	}
	return BadImplementation;
}

bool
X11::getWindowAttributes(Window win, XWindowAttributes *wa)
{
	if (_dpy) {
		return XGetWindowAttributes(_dpy, win, wa);
	}
	return BadImplementation;
}

GC
X11::createGC(Drawable d, ulong mask, XGCValues *values)
{
	if (_dpy) {
		return XCreateGC(_dpy, d, mask, values);
	}
	return None;
}

void
X11::freeGC(GC gc)
{
	if (_dpy) {
		XFreeGC(_dpy, gc);
	}
}

Pixmap
X11::createPixmapMask(unsigned w, unsigned h)
{
	if (_dpy) {
		return XCreatePixmap(_dpy, _root, w, h, 1);
	}
	return None;
}

Pixmap
X11::createPixmap(unsigned w, unsigned h)
{
	if (_dpy) {
		return XCreatePixmap(_dpy, _root, w, h, _depth);
	}
	return None;
}

void
X11::freePixmap(Pixmap& pixmap)
{
	if (_dpy && pixmap != None) {
		XFreePixmap(_dpy, pixmap);
	}
	pixmap = None;
}

XImage*
X11::createImage(char *data, uint width, uint height)
{
	if (_dpy) {
		return XCreateImage(_dpy, _visual, 24, ZPixmap,
				    0, data, width, height, 32, 0);
	}
	return nullptr;
}

XImage*
X11::getImage(Drawable src, int x, int y, uint width, uint height,
              unsigned long plane_mask, int format)
{
	if (_dpy) {
		return XGetImage(_dpy, src, x, y, width, height,
				 plane_mask, format);
	}
	return nullptr;
}

void
X11::putImage(Drawable dest, GC gc, XImage *ximage,
              int src_x, int src_y, int dest_x, int dest_y,
              uint width, uint height)
{
	if (_dpy) {
		XPutImage(_dpy, dest, gc, ximage,
			  src_x, src_y, dest_x, dest_y, width, height);
	}
}

void
X11::destroyImage(XImage *ximage)
{
	if (ximage) {
		XDestroyImage(ximage);
	}
}

void
X11::setWindowBackground(Window window, ulong pixel)
{
	if (_dpy) {
		XSetWindowBackground(_dpy, window, pixel);
	}
}

void
X11::setWindowBackgroundPixmap(Window window, Pixmap pixmap)
{
	if (_dpy) {
		XSetWindowBackgroundPixmap(_dpy, window, pixmap);
	}
}

void
X11::clearWindow(Window window)
{
	if (_dpy) {
		XClearWindow(_dpy, window);
	}
}

void
X11::clearArea(Window window, int x, int y, uint width, uint height)
{
	if (_dpy) {
		XClearArea(_dpy, window, x, y, width, height, False);
	}
}

#ifdef PEKWM_HAVE_SHAPE
void
X11::shapeSelectInput(Window window, ulong mask)
{
	if (_dpy) {
		XShapeSelectInput(_dpy, window, mask);
	}
}

void
X11::shapeQuery(Window dst, int *bshaped)
{
	int foo; unsigned bar;
	XShapeQueryExtents(_dpy, dst, bshaped, &foo, &foo, &bar, &bar,
			   &foo, &foo, &foo, &bar, &bar);
}

void
X11::shapeCombine(Window dst, int kind, int x, int y,
                  Window src, int op)
{
	XShapeCombineShape(_dpy, dst, kind, x, y, src, kind, op);
}

void
X11::shapeSetRect(Window dst, XRectangle *rect)
{
	XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
				ShapeSet, YXBanded);
}

void
X11::shapeIntersectRect(Window dst, XRectangle *rect)
{
	XShapeCombineRectangles(_dpy, dst, ShapeBounding, 0, 0, rect, 1,
				ShapeIntersect, YXBanded);
}

void
X11::shapeSetMask(Window dst, int kind, Pixmap pix)
{
	XShapeCombineMask(_dpy, dst, kind, 0, 0, pix, ShapeSet);
}

XRectangle*
X11::shapeGetRects(Window win, int kind, int *num)
{
	int ordering;
	return XShapeGetRectangles(_dpy, win, kind, num, &ordering);
}
#else // ! PEKWM_HAVE_SHAPE
void
X11::shapeSelectInput(Window window, ulong mask)
{
}

void
X11::shapeQuery(Window dst, int *bshaped)
{
	*bshaped = 0;
}

void
X11::shapeCombine(Window dst, int kind, int x, int y,
                  Window src, int op)
{
}

void
X11::shapeSetRect(Window dst, XRectangle *rect)
{
}

void
X11::shapeIntersectRect(Window dst, XRectangle *rect)
{
}

void
X11::shapeSetMask(Window dst, int kind, Pixmap pix)
{
}

XRectangle*
X11::shapeGetRects(Window win, int kind, int *num)
{
	num = 0;
	return nullptr;
}
#endif // PEKWM_HAVE_SHAPE

//! @brief Initialize head information
void
X11::initHeads(void)
{
	_heads.clear();

	// Read head information, randr has priority over xinerama then
	// comes ordinary X11 information.

	initHeadsRandr();
	if (! _heads.size()) {
		initHeadsXinerama();

		if (! _heads.size()) {
			addHead(Head(0, 0, _screen_gm.width, _screen_gm.height));
		}
	}
}

//! @brief Initialize head information from Xinerama
void
X11::initHeadsXinerama(void)
{
#ifdef PEKWM_HAVE_XINERAMA
	// Check if there are heads already initialized from example Randr
	if (! XineramaIsActive(_dpy)) {
		return;
	}

	int num_heads = 0;
	XineramaScreenInfo *infos = XineramaQueryScreens(_dpy, &num_heads);

	for (int i = 0; i < num_heads; ++i) {
		addHead(Head(infos[i].x_org, infos[i].y_org,
			     infos[i].width, infos[i].height));
	}

	X11::free(infos);
#endif // PEKWM_HAVE_XINERAMA
}

//! @brief Initialize head information from RandR
void
X11::initHeadsRandr(void)
{
#ifdef PEKWM_HAVE_XRANDR
	if (! _honour_randr || ! _has_extension_xrandr) {
		return;
	}

	XRRScreenResources *resources = XRRGetScreenResources(_dpy, _root);
	if (! resources) {
		return;
	}

	for (int i = 0; i < resources->noutput; ++i) {
		XRROutputInfo* output =
			XRRGetOutputInfo(_dpy, resources, resources->outputs[i]);
		if (output->crtc) {
			XRRCrtcInfo* crtc = XRRGetCrtcInfo(_dpy, resources, output->crtc);
			addHead(Head(crtc->x, crtc->y, crtc->width, crtc->height));
			XRRFreeCrtcInfo (crtc);
		}
		XRRFreeOutputInfo (output);
	}

	XRRFreeScreenResources (resources);
#endif // PEKWM_HAVE_XRANDR
}

// gets the squared distance between 2 points
uint
X11::calcDistance(int x1, int y1, int x2, int y2)
{
	return (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2);
}

// gets the squared distance between 2 points with either x or y the same
uint
X11::calcDistance(int p1, int p2)
{
	return (p1 - p2) * (p1 - p2);
}

/**
 * Lookup mask from keycode.
 *
 * @param keycode KeyCode to lookup.
 * @return Mask for keycode, 0 if something fails.
 */
uint
X11::getMaskFromKeycode(KeyCode keycode)
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
X11::getKeycodeFromMask(uint mask)
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

/**
 * Wrapper for XKeycodeToKeysym and XkbKeycodeToKeysym depending on
 * which one is available.
 */
#ifdef __GNUG__
#ifdef PEKWM_HAVE_GCC_DIAGNOSTICS_PUSH
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif // PEKWM_HAVE_GCC_DIAGNOSTICS_PUSH
#else // ! __GNUG__
#ifdef __SUNPRO_CC
#pragma error_messages (off,symdeprecated)
#endif // __SUNPRO_CC
#endif // __GNUG__
KeySym
X11::getKeysymFromKeycode(KeyCode keycode)
{
#ifdef PEKWM_HAVE_X11_XKBLIB_H
	if (_has_extension_xkb)
		return XkbKeycodeToKeysym(_dpy, keycode, 0, 0);
	else
#endif
		return XKeycodeToKeysym(_dpy, keycode, 0);
}
#ifdef __GNUG__
#ifdef PEKWM_HAVE_GCC_DIAGNOSTICS_PUSH
#pragma GCC diagnostic pop
#endif // PEKWM_HAVE_GCC_DIAGNOSTICS_PUSH
#endif // __GNUG__

void
X11::removeMotionEvents(void)
{
	XEvent xev;
	while (XCheckMaskEvent(_dpy, PointerMotionMask, &xev))
		;
}

/**
 * Parse string and set on geometry, same format as XParseGeometry
 * however both size and position can be given in percent.
 *
 * Format: [=][<width>{xX}<height>][{+-}<xoffset>{+-}<yoffset>]
 */
int
X11::parseGeometry(const std::string& str, Geometry& gm)
{
	int mask = 0;
	if (str.size() < 3) {
		// no valid geometry can fit in less than 3 characters.
		return mask;
	}

	// skip initial = if given
	const char *cstr = str.c_str();
	std::string::size_type s_start = str[0] == '=' ? 1 : 0;
	std::string::size_type s_end = str.find_first_of("+-", s_start);

	int ret;
	if (s_end == std::string::npos) {
		s_end = str.size();
	} else {
		// position
		std::string::size_type y_start = str.find_first_of("+-", s_end + 1);
		if ((ret = parseGeometryVal(cstr + s_end + 1,
					    cstr + y_start, gm.x)) > 0) {
			mask |= X_VALUE;
			if (str[s_end] == '-') {
				mask |= X_NEGATIVE;
			}
			if (ret == 2) {
				mask |= X_PERCENT;
			}
		}
		if ((ret = parseGeometryVal(cstr + y_start + 1,
					    cstr + str.size(), gm.y)) > 0) {
			mask |= Y_VALUE;
			if (str[y_start] == '-') {
				mask |= Y_NEGATIVE;
			}
			if (ret == 2) {
				mask |= Y_PERCENT;
			}
		}
	}

	if (s_end > s_start) {
		// size
		int width, height;
		std::string::size_type h_start = str.find_first_of("xX", s_start);
		if ((ret = parseGeometryVal(cstr + s_start, cstr + h_start, width)) > 0
		    && width > 0) {
			gm.width = width;
			mask |= WIDTH_VALUE;
			if (ret == 2) {
				mask |= WIDTH_PERCENT;
			}
		}
		if ((ret = parseGeometryVal(cstr + h_start + 1,
					    cstr + s_end, height)) > 0
		    && height > 0) {
			gm.height = height;
			mask |= HEIGHT_VALUE;
			if (ret == 2) {
				mask |= HEIGHT_PERCENT;
			}
		}
	}

	return mask;
}

int
X11::parseGeometryVal(const char *cstr, const char *e_end, int &val)
{
	char *end = 0;
	val = strtoll(cstr, &end, 10);
	if (*end == '%') {
		if (val < 0 || val > 100) {
			return 0;
		}
		return 2;
	}
	return end == e_end ? 1 : 0;
}

void
X11::keepVisible(Geometry &gm)
{
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

Window
X11::createWindow(Window parent,
                  int x, int y, uint width, uint height,
                  uint border_width, uint depth, uint _class,
                  Visual* visual, ulong valuemask,
                  XSetWindowAttributes* attrs)
{
	if (_dpy) {
		return XCreateWindow(_dpy, parent,
				     x, y, width, height, border_width,
				     depth, _class, visual, valuemask, attrs);
	}
	return None;
}

Window
X11::createSimpleWindow(Window parent,
                        int x, int y, uint width, uint height,
                        uint border_width,
                        ulong border, ulong background)
{
	if (_dpy) {
		return XCreateSimpleWindow(_dpy, parent, x, y, width, height,
					   border_width, border, background);
	}
	return None;
}

void
X11::destroyWindow(Window win)
{
	if (_dpy) {
		XDestroyWindow(_dpy, win);
	}
}

void
X11::changeWindowAttributes(Window win, ulong mask,
                            XSetWindowAttributes &attrs)
{
	if (_dpy) {
		XChangeWindowAttributes(_dpy, win, mask, &attrs);
	}
}

void
X11::grabButton(unsigned b, unsigned int mod, Window win,
                unsigned mask, int mode)
{
	XGrabButton(_dpy, b, mod, win, true, mask, mode,
		    GrabModeAsync, None, None);
}

void
X11::mapWindow(Window w)
{
	if (_dpy) {
		XMapWindow(_dpy, w);
	}
}

void
X11::mapRaised(Window w)
{
	if (_dpy) {
		XMapRaised(_dpy, w);
	}
}

void
X11::unmapWindow(Window w)
{
	if (_dpy) {
		XUnmapWindow(_dpy, w);
	}
}

void
X11::reparentWindow(Window w, Window parent, int x, int y)
{
	if (_dpy) {
		XReparentWindow(_dpy, w, parent, x, y);
	}
}

void
X11::raiseWindow(Window w)
{
	if (_dpy) {
		XRaiseWindow(_dpy, w);
	}
}

void
X11::lowerWindow(Window w)
{
	if (_dpy) {
		XLowerWindow(_dpy, w);
	}
}

void
X11::ungrabButton(uint button, uint modifiers, Window win)
{
	XUngrabButton(_dpy, button, modifiers, win);
}

/**
 * Wrapper for XRestackWindows, windows go in top-to-bottom order.
 */
void
X11::stackWindows(Window *wins, unsigned len)
{
	if (len > 1) {
		XRestackWindows(_dpy, wins, len);
	}
}

bool
X11::checkTypedEvent(int type, XEvent *ev)
{
	return XCheckTypedEvent(_dpy, type, ev);
}

void
X11::sync(Bool discard)
{
	if (_dpy) {
		XSync(X11::getDpy(), discard);
	}
}

int
X11::selectInput(Window w, long mask)
{
	if (_dpy) {
		return XSelectInput(_dpy, w, mask);
	}
	return 0;
}

void
X11::setInputFocus(Window w)
{
	XSetInputFocus(_dpy, w, RevertToPointerRoot, CurrentTime);
}

Display *X11::_dpy;
bool X11::_honour_randr = false;
int X11::_fd = -1;
int X11::_screen = -1;
int X11::_depth = -1;
Geometry X11::_screen_gm;
Window X11::_root = None;
Visual *X11::_visual = 0;
GC X11::_gc = None;
Colormap X11::_colormap = None;
XModifierKeymap *X11::_modifier_map;
bool X11::_has_extension_shape = false;
int X11::_event_shape = -1;
bool X11::_has_extension_xkb = false;
bool X11::_has_extension_xinerama = false;
bool X11::_has_extension_xrandr = false;
int X11::_event_xrandr = -1;
uint X11::_num_lock;
uint X11::_scroll_lock;
std::vector<Head> X11::_heads;
uint X11::_server_grabs;
Time X11::_last_event_time;
Window X11::_last_click_id = None;
Time X11::_last_click_time[BUTTON_NO - 1];
std::vector<X11::ColorEntry*> X11::_colors;
XColor X11::_xc_default;
Cursor X11::_cursor_map[CURSOR_NONE];
