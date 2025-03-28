//
// X11.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén
// Copyright (C) 2009-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"
#include "Util.hh"

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
#include "Container.hh"
#include "Debug.hh"
#include "String.hh"
#include "pekwm_types.hh"

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
			P_TRACE("XError: " << error_buf
				<< " id: " << ev->resourceid);
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
	"_NET_RESTACK_WINDOW",
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
	"RESOURCE_MANAGER",

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
	"_PEKWM_THEME_VARIANT",
	"_PEKWM_THEME_SCALE",
	"_PEKWM_CLIENT_LIST",

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

	// xsettings
	"_XSETTINGS_SETTINGS",

	// xembed, systray
	"_XEMBED",
	"_XEMBED_INFO",
	"_NET_SYSTEM_TRAY_OPCODE",
	"_NET_SYSTEM_TRAY_MESSAGE_DATA",

	// misc
	"XTERM_CONTROL",

	nullptr
};

Strut::Strut(const long* s)
{
	*this = s;
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

typedef std::pair<std::string, std::string> string_pair;

static bool
_string_pair_key_comp(const string_pair &lhs, const string_pair &rhs)
{
	return lhs.first < rhs.first;
}

bool
XrmResourceCbCollect::visit(const std::string& key, const std::string& value)
{
	_items.push_back(string_pair(key, value));
	return true;
}

void
XrmResourceCbCollect::sort()
{
	std::sort(_items.begin(), _items.end(), _string_pair_key_comp);
}

X11_XImage::X11_XImage(int depth, uint width, uint height)
	: _ximage(XCreateImage(X11::getDpy(), X11::getVisual(), depth, ZPixmap,
			       0, nullptr, width, height, 32, 0))
{
	if (_ximage) {
		_ximage->data = new char[_ximage->bytes_per_line * height];
		if (! _ximage->data) {
			X11::destroyImage(_ximage);
			_ximage = nullptr;
		}
	}
}

X11_XImage::~X11_XImage()
{
	if (_ximage) {
		delete[] _ximage->data;
		_ximage->data = nullptr;
		X11::destroyImage(_ximage);
	}
}

X11_GC::X11_GC(Drawable d, ulong mask, XGCValues *values)
	: _gc(XCreateGC(X11::getDpy(), d, mask, values))
{
}

X11_GC::~X11_GC()
{
	if (_gc != None) {
		XFreeGC(X11::getDpy(), _gc);
	}
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
		return pekwm::ascii_ncase_equal(_name, name);
	}

private:
	std::string _name;
	XColor _xc;
	uint _ref;
};

/**
 * Open X11 connection and init X11, if open fails output error on the provided
 * output stream and return false.
 */
bool
X11::init(const char *display, std::ostream &os, bool synchronous,
	  bool honour_randr)
{
	Display *dpy = XOpenDisplay(display);
	if (! dpy) {
		std::string actual_display =
			display ? display : Util::getEnv("DISPLAY");
		os << "Can not open display!" << std::endl
		   << "Your DISPLAY variable currently is set to: "
		   << actual_display << std::endl;
		return false;
	}

	X11::init(dpy);
	return true;
}

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
	_max_request_size = XMaxRequestSize(_dpy) << 2;
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
	_cursor_map[CURSOR_TOP_LEFT] =
		XCreateFontCursor(_dpy, XC_top_left_corner);
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

	int major, minor, dummy_error;
#ifdef PEKWM_HAVE_SHAPE
	{
		_has_extension_shape =
			XShapeQueryExtension(_dpy, &_event_shape, &dummy_error);
	}
#endif // PEKWM_HAVE_SHAPE

#ifdef PEKWM_HAVE_XDBE
	{
		_has_extension_xdbe =
			XdbeQueryExtension(_dpy, &major, &minor);
	}
#endif // PEKWM_HAVE_XDBE

#ifdef PEKWM_HAVE_XRANDR
	if (XRRQueryExtension(_dpy, &_event_xrandr, &dummy_error)
	    && XRRQueryVersion(_dpy, &major, &minor)) {
		_xrandr_extension = major * 10 + minor;
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
	for (uint i = 0; i < BUTTON_NO; ++i) {
		_last_click_time[i] = 0;
	}

	// Figure out what keys the Num and Scroll Locks are
	setLockKeys();

	ungrabServer(true);

	_xc_default.pixel = BlackPixel(_dpy, _screen);
	_xc_default.red = _xc_default.green = _xc_default.blue = 0;

	assert(sizeof(atomnames)/sizeof(char*) == (MAX_NR_ATOMS + 1));
	if (! XInternAtoms(_dpy, const_cast<char**>(atomnames), MAX_NR_ATOMS,
			   0, _atoms)) {
		P_ERR("XInternAtoms did not return all requested atoms");
	}

	XrmInitialize();
	loadXrmResources();
}

//! @brief X11 destructor
void
X11::destruct(void) {
	if (_pixmap_checker != None) {
		XFreePixmap(_dpy, _pixmap_checker);
	}

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

	XrmDestroyDatabase(_xrm_db);

	// Under certain circumstances trying to restart pekwm can cause it to
	// use 100% of the CPU without making any progress with the restart.
	// This X11:sync() seems to be work around the issue (c.f. #300).
	X11::sync(True);

	XCloseDisplay(_dpy);
	_dpy = 0;
}

/**
 * Wrapper for XGetSelectionOwner.
 */
Window
X11::getSelectionOwner(Atom atom)
{
	if (_dpy) {
		return XGetSelectionOwner(_dpy, atom);
	}
	return None;
}

/**
 * Wrapper for XSetSelectionOwner with default display and default timestamp
 * from last registered timestamp.
 *
 * @param timestamp Use -1 for last event time (default)
 */
void
X11::setSelectionOwner(Atom atom, Window owner, Time timestamp)
{
	if (_dpy) {
		if (timestamp == static_cast<Time>(-1)) {
			timestamp = _last_event_time;
		}
		XSetSelectionOwner(_dpy, atom, owner, timestamp);
	}
}

XColor *
X11::getColor(const std::string &color)
{
	if (pekwm::ascii_ncase_equal(color.c_str(), "EMPTY")) {
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
	if (xc == nullptr || &_xc_default == xc) {
		xc = nullptr;
		return;
	}

	std::vector<ColorEntry*>::iterator it = _colors.begin();
	for (; it != _colors.end(); ++it) {
		if ((*it)->getColor() == xc) {
			(*it)->decRef();
			if (((*it)->getRef() == 0)) {
				ulong pixels[1] = { (*it)->getColor()->pixel };
				XFreeColors(X11::getDpy(), X11::getColormap(),
					    pixels, 1, 0);

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
X11::queryRootPointer(int &x, int &y)
{
	if (_dpy) {
		Window root, child;
		int child_x, child_y;
		uint mask;
		XQueryPointer(_dpy, _root, &root, &child, &x, &y,
			      &child_x, &child_y, &mask);
	} else {
		x = 0;
		y = 0;
	}
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
	KeyCode kc_num = XKeysymToKeycode(_dpy, XK_Num_Lock);
	KeyCode kc_scroll = XKeysymToKeycode(_dpy, XK_Scroll_Lock);
	_num_lock = getMaskFromKeycode(kc_num);
	_scroll_lock = getMaskFromKeycode(kc_scroll);
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
X11::getNextEvent(XEvent &ev)
{
	if (_dpy) {
		XNextEvent(_dpy, &ev);
		return true;
	}
	return false;
}

/**
 * Wrapper for XWindowEvent
 */
bool
X11::getWindowEvent(Window win, long event_mask, XEvent &ev)
{
	if (_dpy) {
		return XWindowEvent(_dpy, win, event_mask, &ev) == Success;
	}
	return false;
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
	if (_server_grabs == 0) {
		return true;
	}

	if (--_server_grabs == 0) {
		if (sync) {
			P_TRACE("0 server grabs left, syncing and ungrab.");
			X11::sync(False);
		} else {
			P_TRACE("0 server grabs left, ungrabbing server.");
		}
		XUngrabServer(_dpy);
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
	if (XGrabPointer(_dpy, win, false, event_mask,
			 GrabModeAsync, GrabModeAsync,
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
 * Translate coordinates relative to the root window getting the relative
 * coordinates and matching window.
 */
Window
X11::translateRootCoordinates(int x, int y, int *ret_x, int *ret_y)
{
	Window win = None;
	XTranslateCoordinates(_dpy, _root, _root, x, y, ret_x, ret_y,
			      &win);
	return win;
}

/**
 * Fill in XVisualInfo for the Screen default Visual.
 */
bool
X11::getVisualInfo(XVisualInfo &info)
{
	if (_dpy) {
		info.visualid = XVisualIDFromVisual(_visual);
		int nitems;
		XVisualInfo *info_ret =
			XGetVisualInfo(_dpy, VisualIDMask, &info, &nitems);
		if (info_ret) {
			info = *info_ret;
			XFree(info_ret);
			return true;
		}
	}
	return false;
}

XdbeBackBuffer
X11::xdbeAllocBackBuffer(Window win)
{
#ifdef PEKWM_HAVE_XDBE
	if (_has_extension_xdbe) {
		return XdbeAllocateBackBufferName(_dpy, win, XdbeCopied);
	}
#endif // PEKWM_HAVE_XDBE
	return None;
}

void
X11::xdbeFreeBackBuffer(XdbeBackBuffer buf)
{
#ifdef PEKWM_HAVE_XDBE
	if (buf != None) {
		XdbeDeallocateBackBufferName(_dpy, buf);
	}
#endif // PEKWM_HAVE_XDBE
}

void
X11::xdbeSwapBackBuffer(Window win)
{
#ifdef PEKWM_HAVE_XDBE
	if (_has_extension_xdbe) {
		XdbeSwapInfo swap_info;
		swap_info.swap_window = win;
		swap_info.swap_action = XdbeCopied;
		XdbeSwapBuffers(_dpy, &swap_info, 1);
	}
#endif // PEKWM_HAVE_XDBE
}

/**
 * Query root, parent and children of the given window.
 */
bool
X11::queryTree(Window win, Window &root, Window &parent,
	       std::vector<Window> &children)
{
	uint num_wins;
	Window *wins;
	if (!_dpy
	    || !XQueryTree(_dpy, win, &root, &parent, &wins, &num_wins)) {
		return false;
	}

	children.reserve(num_wins);
	std::copy(wins, wins + num_wins, std::back_inserter(children));
	X11::free(wins);
	return true;
}

void
X11::saveSetAdd(Window win)
{
	if (_dpy) {
		XAddToSaveSet(_dpy, win);
	}
}

void
X11::saveSetRemove(Window win)
{
	if (_dpy) {
		XRemoveFromSaveSet(_dpy, win);
	}
}

/**
 * Refetches the root-window size.
 */
bool
X11::updateGeometry(uint width, uint height)
{
#ifdef PEKWM_HAVE_XRANDR
	if (! _honour_randr || ! _xrandr_extension) {
		return false;
	}

	// The screen has changed geometry in some way. To handle this the
	// head information is read once again, the root window is re sized
	// and strut information is updated.
	initHeads();

	bool updated =
		_screen_gm.width != width || _screen_gm.height != height;
	_screen_gm.width = width;
	_screen_gm.height = height;
	return updated;
#else // ! PEKWM_HAVE_XRANDR
	return false;
#endif // PEKWM_HAVE_XRANDR
}

bool
X11::selectXRandrInput(void)
{
#ifdef PEKWM_HAVE_XRANDR
	if (_honour_randr && _xrandr_extension) {
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
	    && _xrandr_extension
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

void
X11::setHonourRandr(bool honour_randr)
{
	if (_honour_randr != honour_randr) {
		initHeads();
	}
	_honour_randr = honour_randr;

}

//! @brief Searches for the head closest to the coordinates x,y.
//! @return The nearest head.  Head numbers are indexed from 0.
uint
X11::getNearestHead(int x, int y)
{
	int nearest_head =
		getNearestHead(x, y, DIRECTION_IGNORED, DIRECTION_IGNORED);
	return nearest_head != -1 ? nearest_head : 0;
}

//! @brief Searches for the head closest to the coordinates x,y.
//!
//! If a direction is not DIRECTION_IGNORED, only nearest window in
//! that direction is selected.
//!
//! @return The nearest head or -1 if not found
int
X11::getNearestHead(int x, int y, DirectionType dx, DirectionType dy)
{
	if (_heads.size() < 2) {
		return -1;
	}

	// set distance to the highest uint value
	uint min_distance = std::numeric_limits<uint>::max();
	int nearest_head = -1;

	for(uint head = 0; head < _heads.size(); ++head) {
		int head_t = _heads[head].y;
		int head_b = _heads[head].y + _heads[head].height;
		int head_l = _heads[head].x;
		int head_r = _heads[head].x + _heads[head].width;

		int head_dx;
		int head_dy;
		if(y < head_t) {
			head_dy = DIRECTION_DOWN;
		} else if(y > head_b) {
			head_dy = DIRECTION_UP;
		} else {
			head_dy = DIRECTION_NO;
		}

		uint distance;
		if(x > head_r) {
			head_dx = DIRECTION_LEFT;
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
			head_dx = DIRECTION_RIGHT;
			if(y < head_t) {
				// above and left of the head
				distance =
					calcDistance(x, y,
						     head_l, head_t);
			} else if(y > head_b) {
				// below and left of the head
				distance =
					calcDistance(x, y,
						     head_l, head_b);
			} else {
				// left of the head
				distance = calcDistance(x, head_l);
			}
		} else {
			head_dx = DIRECTION_NO;
			if(y < head_t) {
				// above the head
				distance = calcDistance(y, head_t);
			} else if(y > head_b) {
				// below the head
				distance = calcDistance(y, head_b);
			} else {
				// on the head
				if(dx == DIRECTION_IGNORED
				   && dy == DIRECTION_IGNORED) {
					return head;
				}
				distance = 0;
			}
		}

		if((dx != DIRECTION_IGNORED && dx != head_dx) ||
		   (dy != DIRECTION_IGNORED && dy != head_dy)) {
			continue;
		}

		if(distance < min_distance) {
			min_distance = distance;
			nearest_head = head;
		}
	}

	return nearest_head;
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

/**
 * Find a head by name case-insensitively (xrandr only). If "name" is
 * "primary" then return the primary head. Return -1 if not matching.
 */
int
X11::findHeadByName(const std::string& name)
{
	bool find_primary = pekwm::ascii_ncase_equal(name, "PRIMARY");
	for (uint i = 0; i < _heads.size(); i++) {
		if (find_primary) {
			if (_heads[i].primary) {
				return i;
			}
		} else if (pekwm::ascii_ncase_equal(_heads[i].name,
							    name)) {
			return i;
		}
	}
	return -1;
}

int
X11::getNumHeads(void)
{
	return _heads.size();
}

AtomName
X11::getAtomName(const std::string& name)
{
	for (int i = 0; atomnames[i] != nullptr; ++i) {
		if (name == atomnames[i]) {
			return AtomName(i);
		}
	}
	return MAX_NR_ATOMS;
}

/**
 * Return Atom from provided string.
 */
Atom
X11::getAtomId(const std::string& str)
{
	if (_dpy) {
		return XInternAtom(_dpy, str.c_str(), False);
	}
	return 0;
}

/**
 * Lookup string representation of AtomId.
 */
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

const char*
X11::getEventTypeString(int type)
{
	switch (type) {
	case KeyPress:
		return "KeyPress";
	case KeyRelease:
		return "KeyRelease";
	case ButtonPress:
		return "ButtonPress";
	case ButtonRelease:
		return "ButtonRelease";
	case MotionNotify:
		return "MotionNotify";
	case EnterNotify:
		return "EnterNotify";
	case LeaveNotify:
		return "LeaveNotify";
	case FocusIn:
		return "FocusIn";
	case FocusOut:
		return "KeymapNotify";
	case KeymapNotify:
		return "KeymapNotify";
	case Expose:
		return "Expose";
	case GraphicsExpose:
		return "GraphicsExpose";
	case NoExpose:
		return "NoExpose";
	case VisibilityNotify:
		return "VisibilityNotify";
	case CreateNotify:
		return "CreateNotify";
	case DestroyNotify:
		return "DestroyNotify";
	case UnmapNotify:
		return "UnmapNotify";
	case MapNotify:
		return "MapNotify";
	case MapRequest:
		return "MapRequest";
	case ReparentNotify:
		return "ReparentNotify";
	case ConfigureNotify:
		return "ConfigureNotify";
	case ConfigureRequest:
		return "ConfigureRequest";
	case GravityNotify:
		return "GravityNotify";
	case ResizeRequest:
		return "ResizeRequest";
	case CirculateNotify:
		return "CirculateNotify";
	case CirculateRequest:
		return "CirculateRequest";
	case PropertyNotify:
		return "PropertyNotify";
	case SelectionClear:
		return "SelectionClear";
	case SelectionRequest:
		return "SelectionRequest";
	case SelectionNotify:
		return "SelectionNotify";
	case ColormapNotify:
		return "ColormapNotify";
	case ClientMessage:
		return "ClientMessage";
	case MappingNotify:
		return "MappingNotify";
#ifdef GenericEvent
	case GenericEvent:
		return "GenericEvent";
#endif // GenericEvent
	default:
		return "UNKNOWN";
	}
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
	changeProperty(win, _atoms[aname], XA_ATOM, 32, PropModeReplace,
		       reinterpret_cast<uchar*>(values), size);
}

void
X11::setEwmhAtomsSupport(Window win)
{
	changeProperty(win, _atoms[NET_SUPPORTED], XA_ATOM, 32,
		       PropModeReplace,
		       reinterpret_cast<uchar*>(_atoms), UTF8_STRING+1);
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
	changeProperty(win, _atoms[aname], XA_WINDOW, 32, PropModeReplace,
		       reinterpret_cast<uchar*>(&value), 1);
}

void
X11::setWindows(Window win, AtomName aname, const std::vector<Window> &windows)
{
	changeProperty(win, _atoms[aname], XA_WINDOW, 32, PropModeReplace,
		       Container::type_data<Window, const uchar*>(windows),
		       windows.size());
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
		value = std::string(reinterpret_cast<const char*>(data));
		while (value.size() < actual) {
			value += ',';
			value += reinterpret_cast<const char*>(data)
				+ value.size();
		}
		X11::free(data);
		return true;
	}
	return false;
}

bool
X11::setString(Window win, AtomName aname, const std::string &value)
{
	return changeProperty(win, _atoms[aname], XA_STRING, 8, PropModeReplace,
			      reinterpret_cast<const uchar*>(value.c_str()),
			      value.size());
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
	if (! _dpy) {
		return false;
	}

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
					   &r_type, &r_format, &read, &left,
					   &data);
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

/**
 * Get XClassHint for the given window.
 */
bool
X11::getClassHint(Window win, X11::ClassHint &class_hint)
{
	XClassHint xclass_hint;
	if (_dpy && XGetClassHint(_dpy, win, &xclass_hint)) {
		class_hint = xclass_hint;
		return true;
	}
	return false;
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

	XQueryPointer(_dpy, _root, &d_root, &d_win, &x, &y,
		      &win_x, &win_y, &mask);
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

/**
 * Set property on window, split up request if larger than max request size.
 */
bool
X11::changeProperty(Window win, Atom prop, Atom type, int format,
		    int mode, const unsigned char *data, int num_e)
{
	if (! _dpy) {
		return false;
	}

	size_t e_size = format / 8;
	size_t req_size = e_size * num_e;
	if (req_size < _max_request_size) {
		// simple path, fits within single request
		return XChangeProperty(_dpy, win, prop, type, format, mode,
				       data, num_e) == Success;
	}

	grabServer();
	size_t e_per_req = _max_request_size / e_size;
	size_t e_left = req_size / e_per_req;
	do {
		XChangeProperty(_dpy, win, prop, type, format, mode, data,
				std::min(e_left, e_per_req));
		data += std::min(e_left, e_per_req) * e_size;
		e_left -= std::min(e_left, e_per_req);
		mode = PropModeAppend;
	} while (e_left > 0);
	ungrabServer(false);

	return true;
}

/**
 * Remove property from window.
 */
int
X11::deleteProperty(Window win, Atom prop)
{
	if (_dpy) {
		return XDeleteProperty(_dpy, win, prop);
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

Status
X11::getWindowAttributes(Window win, XWindowAttributes &wa)
{
	if (_dpy) {
		return XGetWindowAttributes(_dpy, win, &wa);
	}
	return BadImplementation;
}

bool
X11::getWMHints(Window win, XWMHints &hints)
{
	if (_dpy) {
		XWMHints *hints_ptr = XGetWMHints(_dpy, win);
		if (hints_ptr) {
			hints = *hints_ptr;
			X11::free(hints_ptr);
			return true;
		}
	}
	return false;
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

/**
 * Return a 32x32 Pixmap with every other pixel set.
 *
 * +--------+
 * |X X X X |
 * | X X X X|
 * |X X X X |
 * | X X X X|
 * +--------+
 */
Pixmap
X11::getPixmapChecker()
{
	if (! _dpy || _pixmap_checker != None) {
		return _pixmap_checker;
	}

	X11_XImage ximage(1, 32, 32);
	if (! *ximage) {
		P_ERR("failed to create XImage(1, 32, 32)");
		return None;
	}

	ulong pixel[2][2] = { { getBlackPixel(), getWhitePixel() },
			      { getWhitePixel(), getBlackPixel() } };
	for (uint y = 0; y < 32; ++y) {
		int p = y % 2;
		for (uint x = 0; x < 32; x += 2) {
			XPutPixel(*ximage, x, y, pixel[p][0]);
			XPutPixel(*ximage, x + 1, y, pixel[p][1]);
		}
	}

	_pixmap_checker = createPixmapMask(32, 32);
	X11_GC gc(_pixmap_checker, 0, 0);
	putImage(_pixmap_checker, *gc, *ximage, 0, 0, 0, 0, 32, 32);

	return _pixmap_checker;
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
X11::copyArea(Drawable src, Drawable dst, int src_x, int src_y,
	      unsigned int width, unsigned int height,
	      int dest_x, int dest_y)
{
	if (_dpy && width > 0 && height > 0) {
		XCopyArea(_dpy, src, dst, getGC(),
			  src_x, src_y, width, height, dest_x, dest_y);
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
	if (_dpy) {
		XShapeCombineMask(_dpy, dst, kind, 0, 0, pix, ShapeSet);
	}
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
			addHead(Head(0, 0,
				     _screen_gm.width, _screen_gm.height));
		}
	}

	// Fallback to head 0 as primary
	bool has_primary = false;
	for (uint i = 0; i < _heads.size(); i++) {
		if (_heads[i].primary) {
			has_primary = true;
			break;
		}
	}
	if (!has_primary) {
		_heads[0].primary = true;
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
	if (! _honour_randr || ! _xrandr_extension) {
		return;
	}

	XRRScreenResources *resources = XRRGetScreenResources(_dpy, _root);
	if (! resources) {
		return;
	}

	RROutput primary_output = 0;
#ifdef PEKWM_HAVE_XRRGETOUTPUTPRIMARY
	if (_xrandr_extension > 12) {
		primary_output = XRRGetOutputPrimary(_dpy, _root);
	}
#endif // PEKWM_HAVE_XRRGETOUTPUTPRIMARY

	for (int i = 0; i < resources->noutput; ++i) {
		XRROutputInfo* output =
			XRRGetOutputInfo(_dpy, resources,
					 resources->outputs[i]);
		if (output->crtc) {
			XRRCrtcInfo* crtc =
				XRRGetCrtcInfo(_dpy, resources, output->crtc);
			addHead(Head(crtc->x, crtc->y,
				     crtc->width, crtc->height,
				     output->name,
				     resources->outputs[i] == primary_output));
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
	int mkpm = _modifier_map->max_keypermod;
	for (int i = 0; i < max_info; ++i) {
		if (_modifier_map->modifiermap[i] == keycode) {
			return MODIFIER_TO_MASK[i / mkpm];
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

	const int mkpm = _modifier_map->max_keypermod;
	for (int i = 0; i < 8; ++i) {
		if (MODIFIER_TO_MASK[i] == mask) {
			return _modifier_map->modifiermap[i * mkpm];
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
 * Parse string widthxheight
 */
bool
X11::parseSize(const std::string &str, uint &width, uint &height)
{
	std::vector<std::string> tok;
	if ((Util::splitString(str, tok, "x", 2, true)) != 2) {
		return false;
	}

	try {
		width = std::stoi(tok[0]);
		height = std::stoi(tok[1]);
	} catch (std::invalid_argument&) {
		return false;
	}
	return true;
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
		std::string::size_type y_start =
			str.find_first_of("+-", s_end + 1);
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
		std::string::size_type h_start =
			str.find_first_of("xX", s_start);
		if ((ret = parseGeometryVal(cstr + s_start,
					    cstr + h_start, width)) > 0
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

/**
 * Create window to be used by the window manager, having override redirect
 * set to true.
 */
Window
X11::createWmWindow(Window parent, int x, int y, uint width, uint height,
		    uint _class, ulong event_mask)
{
	XSetWindowAttributes attr;
	attr.event_mask = event_mask;
	attr.override_redirect = True;
	return createWindow(parent, x, y, width, height, 0,
			    CopyFromParent, _class, CopyFromParent,
			    CWEventMask|CWOverrideRedirect, &attr);
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
	XGrabButton(_dpy, b, mod, win, False, mask, mode,
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
X11::mapSubwindows(Window w)
{
	if (_dpy) {
		XMapSubwindows(_dpy, w);
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
X11::stackWindows(const std::vector<Window> &windows)
{
	if (_dpy && ! windows.empty()) {
		const Window *cwindows =
			Container::type_data<Window, const Window*>(windows);
		XRestackWindows(_dpy, const_cast<Window*>(cwindows),
				windows.size());
	}
}

bool
X11::maskEvent(long event_mask, XEvent *ev)
{
	return XMaskEvent(_dpy, event_mask, ev);
}

bool
X11::checkTypedEvent(int type, XEvent *ev)
{
	return XCheckTypedEvent(_dpy, type, ev);
}

bool
X11::checkTypedWindowEvent(Window w, int type, XEvent *ev)
{
	return XCheckTypedWindowEvent(_dpy, w, type, ev);
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

void
X11::loadXrmResources()
{
	XrmDestroyDatabase(_xrm_db);
	_xrm_db = nullptr;

	// not using XResourceManagerString as the result is cached
	// even if the resources on the display gets updated
	std::string xrm;
	// ignoring the result here, initialize an empty database if the
	// resource is missing
	getString(_root, RESOURCE_MANAGER, xrm);
	_xrm_db = XrmGetStringDatabase(xrm.c_str());
}

/**
 * Merge the currently set Xrm resources with the ones set on the display.
 */
void
X11::saveXrmResources()
{
	if (! _xrm_db) {
		return;
	}

	XrmResourceCbCollect resources;
	enumerateXrmResources(&resources);
	resources.sort();

	std::stringstream buf;
	XrmResourceCbCollect::vector::const_iterator it(resources.begin());
	for (; it != resources.end(); ++it) {
		buf << it->first << ":\t" << it->second << "\n";
	}
	const std::string &resources_str = buf.str();
	if (resources_str.empty()) {
		deleteProperty(_root, RESOURCE_MANAGER);
	} else {
		setString(_root, RESOURCE_MANAGER, buf.str());
	}
}

static const char*
buildXrmKeyBinding(int n, XrmBindingList bindings)
{
	if (n == 0) {
		return "";
	} else if (bindings[n] == XrmBindTightly) {
		return ".";
	} else {
		return "*";
	}
}

static std::string
buildXrmKey(XrmBindingList bindings, XrmQuarkList quarks)
{
	std::string key;
	for (int i = 0; quarks[i]; i++) {
		key += buildXrmKeyBinding(i, bindings);
		key += XrmQuarkToString(quarks[i]);
	}
	return key;
}

static Bool
enumerateXrmResourcesFun(XrmDatabase*,
			 XrmBindingList bindings, XrmQuarkList quarks,
			 XrmRepresentation*, XrmValue* xrm_value,
			 XPointer obj)
{
	XrmResourceCb *cb = reinterpret_cast<XrmResourceCb*>(obj);

	std::string key = buildXrmKey(bindings, quarks);
	std::string value;
	if (xrm_value->size > 1) {
		value.assign(xrm_value->addr, xrm_value->size - 1);
	}
	return !cb->visit(key, value);
}

void
X11::enumerateXrmResources(XrmResourceCb* cb)
{
	if (_xrm_db) {
		XrmName names[1] = {0};
		XrmClass classes[1] = {0};
		XrmEnumerateDatabase(_xrm_db, names, classes, XrmEnumAllLevels,
				     enumerateXrmResourcesFun,
				     reinterpret_cast<XPointer>(cb));
	}
}

/**
 * Lookup String resource from REOURCE_MANAGER.
 */
bool
X11::getXrmString(const std::string& name, std::string& val)
{
	if (_xrm_db) {
		char *type;
		XrmValue value;
		if (XrmGetResource(_xrm_db, name.c_str(), "String",
				   &type, &value)
		    && value.size > 0
		    && strcmp(type, "String") == 0) {
			val = std::string(value.addr, value.size - 1);
			return true;
		}
	}
	return false;
}

/**
 * Set String resource in REOURCE_MANAGER (not updating the X11 shared
 * resource)
 */
bool
X11::setXrmString(const std::string& name, const std::string& val)
{
	if (_dpy) {
		XrmPutStringResource(&_xrm_db, name.c_str(), val.c_str());
		return true;
	}
	return false;
}

/**
 * Clear referenced resources from previous lookups.
 */
void
X11::clearRefResources(void)
{
	_ref_resources.clear();
}

/**
 * Get map of referenced resources and their values.
 */
const std::map<std::string, std::string>&
X11::getRefResources(void)
{
	return _ref_resources;
}

/**
 * Add resource -> value to registered color resources.
 */
void
X11::registerRefResource(const std::string& res, const std::string& value)
{
	_ref_resources[res] = value;
}

const long XEMBED_VERSION = 0;

Display *X11::_dpy;
bool X11::_honour_randr = false;
int X11::_fd = -1;
int X11::_screen = -1;
int X11::_depth = -1;
size_t X11::_max_request_size = 4096; // protocol minimum
Geometry X11::_screen_gm;
Window X11::_root = None;
Visual *X11::_visual = 0;
GC X11::_gc = None;
Colormap X11::_colormap = None;
XModifierKeymap *X11::_modifier_map;
bool X11::_has_extension_shape = false;
int X11::_event_shape = -1;
bool X11::_has_extension_xdbe = false;
bool X11::_has_extension_xkb = false;
bool X11::_has_extension_xinerama = false;
int X11::_xrandr_extension = 0;
int X11::_event_xrandr = -1;
uint X11::_num_lock;
uint X11::_scroll_lock;
std::vector<Head> X11::_heads;
uint X11::_server_grabs;
Time X11::_last_event_time;
Window X11::_last_click_id = None;
Time X11::_last_click_time[BUTTON_NO];
Pixmap X11::_pixmap_checker = None;
std::vector<X11::ColorEntry*> X11::_colors;
XColor X11::_xc_default;
Cursor X11::_cursor_map[CURSOR_NONE];
XrmDatabase X11::_xrm_db = 0;
std::map<std::string, std::string> X11::_ref_resources =
	std::map<std::string, std::string>();
