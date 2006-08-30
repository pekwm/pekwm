//
// Atoms.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _ATOMS_HH_
#define _ATOMS_HH_

#include <map>

extern "C" {
#include <X11/Xlib.h>
}

// pekwm Atom Names
enum PekwmAtomName {
	PEKWM_FRAME_ID
};

// ICCCM Atom Names
enum IcccmAtomName {
	WM_STATE, WM_CHANGE_STATE,
	WM_PROTOCOLS, WM_DELETE_WINDOW,
	WM_COLORMAP_WINDOWS, WM_TAKE_FOCUS
};

// Ewmh Atom Names
enum EwmhAtomName {
	NET_SUPPORTED,
	NET_CLIENT_LIST, NET_CLIENT_LIST_STACKING,
	NET_NUMBER_OF_DESKTOPS,
	NET_DESKTOP_GEOMETRY, NET_DESKTOP_VIEWPORT,
	NET_CURRENT_DESKTOP, NET_ACTIVE_WINDOW,
	NET_WORKAREA, NET_SUPPORTING_WM_CHECK,
	NET_CLOSE_WINDOW,
	NET_WM_NAME, NET_WM_DESKTOP,
	NET_WM_STRUT,

	WINDOW_TYPE,
	WINDOW_TYPE_DESKTOP, WINDOW_TYPE_DOCK,
	WINDOW_TYPE_TOOLBAR, WINDOW_TYPE_MENU,
	WINDOW_TYPE_UTILITY, WINDOW_TYPE_SPLASH,
	WINDOW_TYPE_DIALOG, WINDOW_TYPE_NORMAL,

	STATE,
	STATE_MODAL, STATE_STICKY,
	STATE_MAXIMIZED_VERT, STATE_MAXIMIZED_HORZ,
	STATE_SHADED,
	STATE_SKIP_TASKBAR, STATE_SKIP_PAGER,
	STATE_HIDDEN, STATE_FULLSCREEN,
	STATE_ABOVE, STATE_BELOW,

	ALLOWED_ACTIONS,
	ACTION_MOVE, ACTION_RESIZE,
	ACTION_MINIMIZE, ACTION_SHADE,
	ACTION_STICK,
	ACTION_MAXIMIZE_VERT, ACTION_MAXIMIZE_HORZ,
	ACTION_FULLSCREEN, ACTION_CHANGE_DESKTOP,
	ACTION_CLOSE
};

// pekwm stuff
class PekwmAtoms {
public:
	PekwmAtoms(Display *dpy);
	~PekwmAtoms();

	inline Atom getAtom(PekwmAtomName name) const {
		std::map<PekwmAtomName, Atom>::const_iterator it = _atoms.find(name);
		if (it != _atoms.end())
			return it->second;
		return None;
	}

private:
	std::map<PekwmAtomName, Atom> _atoms;
};

// ICCCM stuff
class IcccmAtoms {
public:
	IcccmAtoms(Display *dpy);
	~IcccmAtoms();

	inline Atom getAtom(IcccmAtomName name) const {
		std::map<IcccmAtomName, Atom>::const_iterator it = _atoms.find(name);
		if (it != _atoms.end())
			return it->second;
		return None;
	}

private:
	std::map<IcccmAtomName, Atom> _atoms;
};

// Extended WM Hints
class EwmhAtoms {
public:
	EwmhAtoms(Display *dpy);
	~EwmhAtoms();

	inline unsigned int size(void) const { return _atoms.size(); }
	inline Atom getAtom(EwmhAtomName name) const {
		std::map<EwmhAtomName, Atom>::const_iterator it = _atoms.find(name);
		if (it != _atoms.end())
			return it->second;
		return None;
	}

	Atom* getAtomArray(void) const;

private:
	std::map<EwmhAtomName, Atom> _atoms;
};

#endif // _FONT_HH_
