//
// X11.hh for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2003-2020 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_X11_HH_
#define _PEKWM_X11_HH_

#include "config.h"

#include "Compat.hh"
#include "Geometry.hh"
#include "Types.hh"

#include <iostream>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xresource.h>
#ifdef PEKWM_HAVE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif // PEKWM_HAVE_XINERAMA
#ifdef PEKWM_HAVE_SHAPE
#include <X11/extensions/shape.h>
#else // ! PEKWM_HAVE_SHAPE
#define ShapeSet 0
#define ShapeIntersect 2

#define ShapeBounding 0
#define ShapeInput 2

#define ShapeNotifyMask 1
#endif // PEKWM_HAVE_SHAPE
#ifdef PEKWM_HAVE_XDBE
#include <X11/extensions/Xdbe.h>
#else // ! PEKWM_HAVE_XDBE
typedef int XdbeBackBuffer;
#endif // PEKWM_HAVE_XDBE

	extern bool xerrors_ignore; /**< If true, ignore X errors. */
	extern unsigned int xerrors_count; /**< Number of X errors occured. */

#define setXErrorsIgnore(X) xerrors_ignore = (X)

}

#define NET_WM_STICKY_WINDOW 0xffffffff
#define EWMH_OPAQUE_WINDOW 0xffffffff

typedef long Cardinal;

enum AtomName {
	// Ewmh Atom Names
	NET_SUPPORTED,
	NET_CLIENT_LIST, NET_CLIENT_LIST_STACKING,
	NET_NUMBER_OF_DESKTOPS,
	NET_DESKTOP_GEOMETRY, NET_DESKTOP_VIEWPORT,
	NET_CURRENT_DESKTOP, NET_DESKTOP_NAMES,
	NET_ACTIVE_WINDOW, NET_WORKAREA,
	NET_DESKTOP_LAYOUT, NET_SUPPORTING_WM_CHECK,
	NET_CLOSE_WINDOW,
	NET_WM_MOVERESIZE,
	NET_RESTACK_WINDOW,
	NET_REQUEST_FRAME_EXTENTS,
	NET_WM_NAME, NET_WM_VISIBLE_NAME,
	NET_WM_ICON_NAME, NET_WM_VISIBLE_ICON_NAME,
	NET_WM_ICON, NET_WM_DESKTOP,
	NET_WM_STRUT, NET_WM_PID,
	NET_WM_USER_TIME,
	NET_FRAME_EXTENTS,
	NET_WM_WINDOW_OPACITY,

	WINDOW_TYPE,
	WINDOW_TYPE_DESKTOP,
	WINDOW_TYPE_DOCK,
	WINDOW_TYPE_TOOLBAR,
	WINDOW_TYPE_MENU,
	WINDOW_TYPE_UTILITY,
	WINDOW_TYPE_SPLASH,
	WINDOW_TYPE_DIALOG,
	WINDOW_TYPE_DROPDOWN_MENU,
	WINDOW_TYPE_POPUP_MENU,
	WINDOW_TYPE_TOOLTIP,
	WINDOW_TYPE_NOTIFICATION,
	WINDOW_TYPE_COMBO,
	WINDOW_TYPE_DND,
	WINDOW_TYPE_NORMAL,

	STATE,
	STATE_MODAL, STATE_STICKY,
	STATE_MAXIMIZED_VERT, STATE_MAXIMIZED_HORZ,
	STATE_SHADED,
	STATE_SKIP_TASKBAR, STATE_SKIP_PAGER,
	STATE_HIDDEN, STATE_FULLSCREEN,
	STATE_ABOVE, STATE_BELOW,
	STATE_DEMANDS_ATTENTION,

	EWMH_ALLOWED_ACTIONS,
	EWMH_ACTION_MOVE, EWMH_ACTION_RESIZE,
	EWMH_ACTION_MINIMIZE, EWMH_ACTION_SHADE,
	EWMH_ACTION_STICK,
	EWHM_ACTION_MAXIMIZE_VERT, EWMH_ACTION_MAXIMIZE_HORZ,
	EWMH_ACTION_FULLSCREEN, ACTION_CHANGE_DESKTOP,
	EWMH_ACTION_CLOSE,

	// all EWMH atoms must be before this, see x11::setEwmhAtomsSupport
	UTF8_STRING,

	STRING,
	MANAGER,
	RESOURCE_MANAGER,

	// pekwm atom names
	PEKWM_FRAME_ID,
	PEKWM_FRAME_ORDER,
	PEKWM_FRAME_ACTIVE,
	PEKWM_FRAME_DECOR,
	PEKWM_FRAME_SKIP,
	PEKWM_TITLE,
	PEKWM_BG_PID,
	PEKWM_CMD,
	PEKWM_THEME,

	// ICCCM Atom Names
	WM_NAME,
	WM_ICON_NAME,
	WM_HINTS,
	WM_CLASS,
	WM_STATE,
	WM_CHANGE_STATE,
	WM_PROTOCOLS,
	WM_DELETE_WINDOW,
	WM_COLORMAP_WINDOWS,
	WM_TAKE_FOCUS,
	WM_WINDOW_ROLE,
	WM_CLIENT_MACHINE,

	// List of non PEKWM, ICCCM and EWMH atoms.
	MOTIF_WM_HINTS,

	XROOTPMAP_ID,
	XSETROOT_ID,

	// xembed, systray
	XEMBED,
	XEMBED_INFO,
	NET_SYSTEM_TRAY_OPCODE,
	NET_SYSTEM_TRAY_MESSAGE_DATA,

	MAX_NR_ATOMS
};

enum ButtonNum {
	BUTTON_ANY = 0,
	BUTTON1 = Button1,
	BUTTON2,
	BUTTON3,
	BUTTON4,
	BUTTON5,
	BUTTON6,
	BUTTON7,
	BUTTON8,
	BUTTON9,
	BUTTON10,
	BUTTON11,
	BUTTON12,
	BUTTON_NO
};

/**
 * Keep in sync with BorderPosition in pekwm.hh
 */
enum CursorType {
	CURSOR_0 = 0,
	CURSOR_TOP_LEFT = 0,
	CURSOR_TOP_RIGHT,
	CURSOR_BOTTOM_LEFT,
	CURSOR_BOTTOM_RIGHT,
	CURSOR_TOP,
	CURSOR_LEFT,
	CURSOR_RIGHT,
	CURSOR_BOTTOM,

	CURSOR_ARROW,
	CURSOR_MOVE,
	CURSOR_RESIZE,
	CURSOR_NONE
};

enum DirectionType {
	DIRECTION_UP,
	DIRECTION_DOWN,
	DIRECTION_LEFT,
	DIRECTION_RIGHT,
	DIRECTION_NO,
	DIRECTION_IGNORED
};


/**
 * Max XEMBED_VERSION supported.
 */
extern const long XEMBED_VERSION;

/**
 * Flags set on _XEMBED_INFO property (1).
 */
enum XEmbedFlags {
	XEMBED_FLAG_MAPPED = 1 << 0
};

/**
 * Class holding information about screen edge allocation.
 */
class Strut {
public:
	Strut(const long* s);
	Strut(long l=0, long r=0, long t=0, long b=0, int nhead=-1);
	~Strut(void);

	void clear(void);

public: // member variables
	long left; /**< Pixels allocated on the left of the head. */
	long right; /**<Pixels allocated on the right of the head. */
	long top; /**< Pixels allocated on the top of the head.*/
	long bottom; /**< Pixels allocated on the bottom of the head.*/
	int head; /**< Which head is the strut valid for */

	void operator=(const long *s);
	bool operator==(const Strut& rhs) const;
	bool operator!=(const Strut& rhs) const;

	friend std::ostream &operator<<(std::ostream &stream,
					const Strut &strut);
};

//! Output head, used to share same code with Xinerama and RandR
class Head {
public:
	Head(int nx, int ny, uint nwidth, uint nheight,
	     const char* nname = nullptr, bool nprimary = false) :
		name(nname ? nname : ""),
		primary(nprimary),
		x(nx),
		y(ny),
		width(nwidth),
		height(nheight)
	{
	};

public:
	std::string name;
	bool primary;
	int x;
	int y;
	uint width;
	uint height;
};

class XrmResourceCb {
public:
	virtual ~XrmResourceCb() { }
	virtual bool visit(const std::string& key,
			   const std::string& value) = 0;
};

/**
 * Wrapper for XRRScreenChangeNotifyEvent.
 */
struct ScreenChangeNotification
{
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
	static GC getGC(void) { return _gc; }
	static Colormap getColormap(void) { return _colormap; }
	static uint getNumLock(void) { return _num_lock; }
	static uint getScrollLock(void) { return _scroll_lock; }
	static bool hasExtensionShape(void) { return _has_extension_shape; }
	static int getEventShape(void) { return _event_shape; }
	static bool hasExtensionXdbe(void) {return _has_extension_xdbe; }
	static XdbeBackBuffer xdbeAllocBackBuffer(Window win);
	static void xdbeFreeBackBuffer(XdbeBackBuffer buf);
	static void xdbeSwapBackBuffer(Window win);

	static bool queryTree(Window win, Window &root, Window &parent,
			      std::vector<Window> &children);

	static bool updateGeometry(uint width, uint height);
	static Cursor getCursor(CursorType type) { return _cursor_map[type]; }

	static Time getLastEventTime(void) { return _last_event_time; }
	static void setLastEventTime(Time t) { _last_event_time = t; }

	static Window getSelectionOwner(Atom atom);
	static void setSelectionOwner(Atom atom, Window win,
				      Time timestamp = -1);

	static Window getLastClickID(void) { return _last_click_id; }
	static void setLastClickID(Window id) { _last_click_id = id; }
	static Time getLastClickTime(uint button) {
		if (button < BUTTON_NO) {
			return _last_click_time[button];
		}
		return 0;
	}
	static void setLastClickTime(uint button, Time time) {
		if (button < BUTTON_NO) {
			_last_click_time[button] = time;
		}
	}
	static bool isDoubleClick(Window id, uint button,
				  Time time, Time dc_time) {
		if ((_last_click_id == id)
		    && ((time - getLastClickTime(button)) < dc_time)) {
			return true;
		}
		return false;
	}

	static XColor *getColor(const std::string &color);
	static void returnColor(XColor*& xc);

	static ulong getWhitePixel(void);
	static ulong getBlackPixel(void);

	static void free(void* data);

	static void warpPointer(int x, int y);

	static void moveWindow(Window win, int x, int y);
	static void resizeWindow(Window win,
				 unsigned int width, unsigned int height);
	static void moveResizeWindow(Window win, int x, int y,
				     unsigned int width, unsigned int height);

	static void stripStateModifiers(unsigned int *state);
	static void stripButtonModifiers(unsigned int *state);
	static void setLockKeys(void);
	static bool selectXRandrInput(void);
	static bool getScreenChangeNotification(XEvent *ev,
						ScreenChangeNotification &scn);

	static void flush(void);
	static int pending(void);

	static bool getNextEvent(XEvent &ev, struct timeval *timeout = nullptr);
	static void allowEvents(int event_mode, Time time);
	static bool grabServer(void);
	static bool ungrabServer(bool sync);
	static bool grabKeyboard(Window win);
	static bool ungrabKeyboard(void);
	static bool grabPointer(Window win, uint event_mask, CursorType cursor);
	static bool ungrabPointer(void);

	static Window translateRootCoordinates(int x, int y,
					       int *ret_x, int *ret_y);

	static void setHonourRandr(bool honour_randr);
	static int getNearestHead(int x, int y,
				  DirectionType dx, DirectionType dy);
	static uint getNearestHead(int x, int y);
	static uint getCursorHead(void);
	static void addHead(const Head &head);
	static bool getHeadInfo(uint head, Geometry &head_info);
	static void getHeadInfo(int x, int y, Geometry &head_info);
	static Geometry getHeadGeometry(uint head);
	static int findHeadByName(const std::string& name);
	static int getNumHeads(void);

	static Atom getAtom(AtomName name) { return _atoms[name]; }
	static AtomName getAtomName(const std::string& str);
	static Atom getAtomId(const std::string& str);
	static std::string getAtomIdString(Atom id);
	static AtomName getAtomName(Atom id);
	static void setAtom(Window win, AtomName aname, AtomName value);
	static void setAtoms(Window win, AtomName aname, Atom *values,
			     int size);
	static void setEwmhAtomsSupport(Window win);
	static bool getWindow(Window win, AtomName aname, Window& value);
	static void setWindow(Window win, AtomName aname, Window value);
	static void setWindows(Window win, AtomName aname, Window *values,
			       int size);
	static bool getCardinal(Window win, AtomName aname, Cardinal &value,
				long format=XA_CARDINAL);
	static void setCardinal(Window win, AtomName aname, Cardinal value,
				long format=XA_CARDINAL);
	static void setCardinals(Window win, AtomName aname,
				 Cardinal *values, int num);
	static bool getUtf8String(Window win, AtomName aname,
				  std::string &value);
	static bool getUtf8StringId(Window win, Atom id, std::string &value);
	static void setUtf8String(Window win, AtomName aname,
				  const std::string &value);
	static void setUtf8StringArray(Window win, AtomName aname,
				       unsigned char *values, uint length);
	static bool getString(Window win, AtomName aname, std::string &value);
	static bool getStringId(Window win, Atom id, std::string &value);
	static void setString(Window win, AtomName aname,
			      const std::string &value);

	static bool listProperties(Window win, std::vector<Atom>& atoms);

	static bool getProperty(Window win, Atom atom, Atom type,
				ulong expected, uchar **data, ulong *actual);
	static bool getTextProperty(Window win, Atom atom, std::string &value);
	static void *getEwmhPropData(Window win, AtomName prop,
				     Atom type, int &num);
	static void unsetProperty(Window win, AtomName aname);

	static void getMousePosition(int &x, int &y);
	static uint getButtonFromState(uint state);

	static uint getMaskFromKeycode(KeyCode keycode);
	static KeyCode getKeycodeFromMask(uint mask);
	static KeySym getKeysymFromKeycode(KeyCode keycode);

	static void removeMotionEvents(void);

	/** Modifier from (XModifierKeymap) to mask table. */
	static const uint MODIFIER_TO_MASK[];
	/** Number of entries in MODIFIER_TO_MASK. */
	static const uint MODIFIER_TO_MASK_NUM;

	// helper functions

	static int parseGeometry(const std::string& str, Geometry& gm);
	static void keepVisible(Geometry &gm);

	// X11 function wrappers

	static Window createWindow(Window parent,
				   int x, int y, uint width, uint height,
				   uint border_width, uint depth, uint _class,
				   Visual* visual, ulong valuemask,
				   XSetWindowAttributes* attrs);
	static Window createWmWindow(Window parent,
				     int x, int y, uint width, uint height,
				     uint _class, ulong event_mask);
	static void destroyWindow(Window win);
	static void changeWindowAttributes(Window win, ulong mask,
					   XSetWindowAttributes &attrs);
	static void grabButton(unsigned b, unsigned int mod, Window win,
			       unsigned mask, int mode=GrabModeAsync);

	static void mapWindow(Window w);
	static void mapRaised(Window w);
	static void unmapWindow(Window w);
	static void reparentWindow(Window w, Window parent, int x, int y);

	static void raiseWindow(Window w);
	static void lowerWindow(Window w);

	static void ungrabButton(uint button, uint modifiers, Window win);

	static void stackWindows(const std::vector<Window> &windows);
	static bool maskEvent(long event_mask, XEvent *ev);
	static bool checkTypedEvent(int type, XEvent *ev);
	static bool checkTypedWindowEvent(Window win, int type, XEvent *ev);

	static void sync(Bool discard);

	static int selectInput(Window w, long mask);
	static void setInputFocus(Window w);

	static int sendEvent(Window dest, Window win, Atom atom, long mask,
			     long v1=0l, long v2=0l, long v3=0l,
			     long v4=0l, long v5=0l);
	static int sendEvent(Window dest, Bool propagate, long mask,
			     XEvent *ev);

	static int changeProperty(Window win, Atom prop, Atom type, int format,
				  int mode, const unsigned char *data,
				  int num_e);

	static int getGeometry(Window win, unsigned *w, unsigned *h,
			       unsigned *bw);

	static Status getWindowAttributes(Window win, XWindowAttributes &wa);
	static bool getWMHints(Window win, XWMHints &hints);

	static GC createGC(Drawable d, ulong mask, XGCValues *values);
	static void freeGC(GC gc);

	static Pixmap createPixmapMask(unsigned w, unsigned h);
	static Pixmap createPixmap(unsigned w, unsigned h);
	static void freePixmap(Pixmap& pixmap);
	static XImage *createImage(char *data, uint width, uint height);
	static XImage *getImage(Drawable src,
				int x, int y, uint width, uint height,
				unsigned long plane_mask, int format);
	static void putImage(Drawable dest, GC gc, XImage *ximage,
			     int src_x, int src_y, int dest_x, int dest_y,
			     uint width, uint height);
	static void destroyImage(XImage *ximage);
	static void copyArea(Drawable src, Drawable dst, int src_x, int src_y,
			     unsigned int width, unsigned int height,
			     int dest_x, int dest_y);

	static void setWindowBackground(Window window, ulong pixel);
	static void setWindowBackgroundPixmap(Window window, Pixmap pixmap);
	static void  clearWindow(Window window);

	static void shapeSelectInput(Window window, ulong mask);
	static void shapeQuery(Window dst, int *bshaped);
	static void shapeCombine(Window dst, int kind, int x, int y,
				 Window src, int op);
	static void shapeSetRect(Window dst, XRectangle *rect);
	static void shapeIntersectRect(Window dst, XRectangle *rect);
	static void shapeSetMask(Window dst, int kind, Pixmap pix);

	static void loadXrmResources(void);
	static void enumerateXrmResources(XrmResourceCb* cb);

	static bool getXrmString(const std::string& name, std::string& val);
	static bool setXrmString(const std::string& name,
				 const std::string& val);

	static void clearRefResources(void);
	static const std::map<std::string, std::string>& getRefResources(void);
	static void registerRefResource(const std::string& res,
					const std::string& value);

protected:
	static int parseGeometryVal(const char *c_str, const char *e_end,
				    int &val_ret);

private:
	static uint calcDistance(int x1, int y1, int x2, int y2);
	static uint calcDistance(int p1, int p2);

	static void initHeads(void);
	static void initHeadsRandr(void);
	static void initHeadsXinerama(void);

protected:
	X11(void) {}
	~X11(void) {}

private:
	static Display *_dpy;
	static bool _honour_randr; /**< If true, honor randr. */
	static int _fd;

	static int _screen, _depth;

	static Geometry _screen_gm; /**< Full screen geometry (not head). */

	static Window _root;
	static Visual *_visual;
	static GC _gc;
	static Colormap _colormap;
	static XModifierKeymap *_modifier_map; /**< Key to modifier mappings. */

	static uint _num_lock;
	static uint _scroll_lock;

	static bool _has_extension_shape;
	static int _event_shape;
	static bool _has_extension_xdbe;
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
	static Time _last_click_time[BUTTON_NO];

	static Cursor _cursor_map[CURSOR_NONE];

	class ColorEntry;
	static std::vector<ColorEntry*> _colors;
	static XColor _xc_default; // when allocating fails
	static XrmDatabase _xrm_db;
	static std::map<std::string, std::string> _ref_resources;

	static Atom _atoms[MAX_NR_ATOMS];
};

#endif // _PEKWM_X11_HH_
