//
// X11Util.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PWinObj.hh"
#include "X11Util.hh"
#include "Util.hh"

#if defined(PEKWM_HAVE_XFT) || defined(PEKWM_HAVE_PANGO_XFT)

PXftColor::PXftColor()
	: _is_set(false)
{
}

PXftColor::~PXftColor()
{
	unset();
}

void
PXftColor::set(ushort r, ushort g, ushort b, uint alpha)
{
	unset();

	XRenderColor xrender_color;
	xrender_color.red = r;
	xrender_color.green = g;
	xrender_color.blue = b;
	xrender_color.alpha = alpha;

	XftColorAllocValue(X11::getDpy(), X11::getVisual(),
			   X11::getColormap(), &xrender_color,
			   &_color);
	_is_set = true;
}

void
PXftColor::unset()
{
	if (_is_set) {
		XftColorFree(X11::getDpy(), X11::getVisual(),
			     X11::getColormap(), &_color);
		_is_set = false;
	}
}

#endif // defined(PEKWM_HAVE_XFT) || defined(PEKWM_HAVE_PANGO_XFT)

NetWMStates::NetWMStates(void)
	: modal(false),
	  sticky(false),
	  max_vert(false),
	  max_horz(false), shaded(false),
	  skip_taskbar(false),
	  skip_pager(false),
	  hidden(false),
	  fullscreen(false),
	  above(false),
	  below(false),
	  demands_attention(false)
{
}

NetWMStates::~NetWMStates(void)
{
}

static void
readEwmhState(Atom atom, NetWMStates& win_states)
{
	if (atom == X11::getAtom(STATE_MODAL)) {
		win_states.modal = true;
	} else if (atom == X11::getAtom(STATE_STICKY)) {
		win_states.sticky = true;
	} else if (atom == X11::getAtom(STATE_MAXIMIZED_VERT)) {
		win_states.max_vert = true;
	} else if (atom == X11::getAtom(STATE_MAXIMIZED_HORZ)) {
		win_states.max_horz = true;
	} else if (atom == X11::getAtom(STATE_SHADED)) {
		win_states.shaded = true;
	} else if (atom == X11::getAtom(STATE_SKIP_TASKBAR)) {
		win_states.skip_taskbar = true;
	} else if (atom == X11::getAtom(STATE_SKIP_PAGER)) {
		win_states.skip_pager = true;
	} else if (atom == X11::getAtom(STATE_DEMANDS_ATTENTION)) {
		win_states.demands_attention = true;
	} else if (atom == X11::getAtom(STATE_HIDDEN)) {
		win_states.hidden = true;
	} else if (atom == X11::getAtom(STATE_FULLSCREEN)) {
		win_states.fullscreen = true;
	} else if (atom == X11::getAtom(STATE_ABOVE)) {
		win_states.above = true;
	} else if (atom == X11::getAtom(STATE_BELOW)) {
		win_states.below = true;
	}
}

namespace X11Util {

	/**
	 * Get geometry for the given head, -1 will return the screen
	 * geometry.
	 */
	Geometry getHeadGeometry(int head)
	{
		if (head < 0) {
			return X11::getScreenGeometry();
		}
		return X11::getHeadGeometry(head);
	}

	/**
	 * Get current head, depending on configuration it is the same
	 * head as the cursor is on OR the head where the focused window
	 * is on.
	 */
	uint getCurrHead(CurrHeadSelector chs)
	{
		PWinObj *wo;
		switch (chs) {
		case CURR_HEAD_SELECTOR_FOCUSED_WINDOW:
			wo = PWinObj::getFocusedPWinObj();
			if (wo != nullptr) {
				int mx = wo->getX() + (wo->getWidth() / 2);
				int my = wo->getY() + (wo->getHeight() / 2);
				return X11::getNearestHead(mx, my);
			}
		case CURR_HEAD_SELECTOR_CURSOR:
		case CURR_HEAD_SELECTOR_NO:
			return X11::getCursorHead();
		}

		return 0;
	}

	/**
	 * Get head closest to center of provided PWinObj.
	 */
	uint getNearestHead(const PWinObj& wo)
	{
		return X11::getNearestHead(wo.getX() + (wo.getWidth() / 2),
					   wo.getY() + (wo.getHeight() / 2));
	}

	int getNearestHead(const PWinObj& wo,
			   DirectionType dx, DirectionType dy)
	{
		return X11::getNearestHead(wo.getX() + (wo.getWidth() / 2),
					   wo.getY() + (wo.getHeight() / 2),
					   dx, dy);
	}

	/**
	 * Grabs the button button, with the mod mod and mask mask on the
	 * window win and cursor curs with "all" possible extra modifiers
	 */
	void
	grabButton(int button, int mod, int mask, Window win, int mode)
	{
		uint num_lock = X11::getNumLock();
		uint scroll_lock = X11::getScrollLock();

		X11::grabButton(button, mod, win, mask, mode);
		X11::grabButton(button, mod|LockMask, win, mask, mode);

		if (num_lock) {
			X11::grabButton(button, mod|num_lock,
					win, mask, mode);
			X11::grabButton(button, mod|num_lock|LockMask,
					win, mask, mode);
		}
		if (scroll_lock) {
			X11::grabButton(button, mod|scroll_lock,
					win, mask, mode);
			X11::grabButton(button, mod|scroll_lock|LockMask,
					win, mask, mode);
		}
		if (num_lock && scroll_lock) {
			X11::grabButton(button,
					mod|num_lock|scroll_lock,
					win, mask, mode);
			X11::grabButton(button,
					mod|num_lock|scroll_lock|LockMask,
					win, mask, mode);
		}
	}

	/**
	 * Reads MWM hints from a client.
	 *
	 * @return true if the hint was read succesfully.
	 */
	bool
	readMwmHints(Window win, MwmHints &hints)
	{
		Atom atom = X11::getAtom(MOTIF_WM_HINTS);
		uchar *data;
		ulong items_read;
		if (! X11::getProperty(win, atom, atom, 20L,
				       &data, &items_read)) {
			return false;
		}

		if (items_read >= MWM_HINTS_NUM) {
			hints = *reinterpret_cast<MwmHints*>(data);
		}

		X11::free(data);
		return items_read >= MWM_HINTS_NUM;
	}

	/**
	 * Read EWMH state atoms on window.
	 */
	bool
	readEwmhStates(Window win, NetWMStates &win_states)
	{
		int num = 0;
		void *data = X11::getEwmhPropData(win, STATE, XA_ATOM, num);
		if (data) {
			Atom *states = static_cast<Atom*>(data);
			for (int i = 0; i < num; ++i) {
				readEwmhState(states[i], win_states);
			}
			X11::free(data);
			return true;
		}
		return false;
	}
}

#ifndef PEKWM_HAVE_XUTF8

void
Xutf8SetWMProperties(Display *dpy, Window win,
		     const char* window_name, const char* icon_name,
		     char** argv, int argc,
		     XSizeHints* normal_hints, XWMHints* wm_hints,
		     XClassHint* class_hints)
{
	X11::setUtf8String(win, WM_CLIENT_MACHINE, Util::getHostname());
	X11::setUtf8String(win, WM_NAME, window_name);
	X11::setUtf8String(win, WM_ICON_NAME, icon_name);

	if (normal_hints) {
		XSetNormalHints(dpy, win, normal_hints);
	}
	if (wm_hints) {
		XSetWMHints(dpy, win, wm_hints);
	}
	if (class_hints) {
		XSetClassHint(dpy, win, class_hints);
	}
}

#endif // PEKWM_HAVE_XUTF8
