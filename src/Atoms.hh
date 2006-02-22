//
// Atoms.hh for pekwm
// Copyright (C) 2003-2004 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _ATOMS_HH_
#define _ATOMS_HH_

#include "Types.hh"

#include <string>
#include <map>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
}

// pekwm Atom Names
enum PekwmAtomName {
	PEKWM_FRAME_ID,
	PEKWM_FRAME_DECOR,
	PEKWM_FRAME_SKIP,
	PEKWM_FRAME_VPOS
};

// ICCCM Atom Names
enum IcccmAtomName {
	WM_STATE, WM_CHANGE_STATE,
	WM_PROTOCOLS, WM_DELETE_WINDOW,
	WM_COLORMAP_WINDOWS, WM_TAKE_FOCUS,
	WM_WINDOW_ROLE
};

// Ewmh Atom Names
enum EwmhAtomName {
	NET_SUPPORTED,
	NET_CLIENT_LIST, NET_CLIENT_LIST_STACKING,
	NET_NUMBER_OF_DESKTOPS,
	NET_DESKTOP_GEOMETRY, NET_DESKTOP_VIEWPORT,
	NET_CURRENT_DESKTOP, NET_ACTIVE_WINDOW,
	NET_WORKAREA, NET_DESKTOP_LAYOUT,
	NET_SUPPORTING_WM_CHECK,
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

	EWMH_ALLOWED_ACTIONS,
	EWMH_ACTION_MOVE, EWMH_ACTION_RESIZE,
	EWMH_ACTION_MINIMIZE, EWMH_ACTION_SHADE,
	EWMH_ACTION_STICK,
	EWHM_ACTION_MAXIMIZE_VERT, EWMH_ACTION_MAXIMIZE_HORZ,
	EWMH_ACTION_FULLSCREEN, ACTION_CHANGE_DESKTOP,
	EWMH_ACTION_CLOSE
};

// pekwm stuff
class PekwmAtoms {
public:
	PekwmAtoms(void);
	~PekwmAtoms(void);

	static PekwmAtoms* instance(void) { return _instance; }

	inline Atom getAtom(PekwmAtomName name) const {
		std::map<PekwmAtomName, Atom>::const_iterator it = _atoms.find(name);
		if (it != _atoms.end())
			return it->second;
		return None;
	}

private:
	std::map<PekwmAtomName, Atom> _atoms;

	static PekwmAtoms *_instance;
};

// ICCCM stuff
class IcccmAtoms {
public:
	IcccmAtoms(void);
	~IcccmAtoms(void);

	static IcccmAtoms* instance(void) { return _instance; }

	inline Atom getAtom(IcccmAtomName name) const {
		std::map<IcccmAtomName, Atom>::const_iterator it = _atoms.find(name);
		if (it != _atoms.end())
			return it->second;
		return None;
	}

private:
	std::map<IcccmAtomName, Atom> _atoms;

	static IcccmAtoms *_instance;
};

// Extended WM Hints
class EwmhAtoms {
public:
	EwmhAtoms(void);
	~EwmhAtoms();

	static EwmhAtoms* instance(void) { return _instance; }

	inline uint size(void) const { return _atoms.size(); }
	inline Atom getAtom(EwmhAtomName name) const {
		std::map<EwmhAtomName, Atom>::const_iterator it = _atoms.find(name);
		if (it != _atoms.end())
			return it->second;
		return None;
	}

	Atom* getAtomArray(void) const;

private:
	std::map<EwmhAtomName, Atom> _atoms;

	static EwmhAtoms *_instance;
};

namespace AtomUtil {

	ulong getProperty(Window win, Atom atom, Atom type, ulong expected, uchar** data);

	void setAtom(Window win, Atom atom, Atom value);

	void setAtoms(Window win, Atom atom, Atom *values, int size);

	void setWindow(Window win, Atom atom, Window value);

	void setWindows(Window win, Atom atom, Window *values, int size);

	bool getLong(Window win, Atom atom, long &value);
	void setLong(Window win, Atom atom, long value);

	bool getString(Window win, Atom atom, std::string &value);
	void setString(Window win, Atom atom, const std::string &value);

	bool getPosition(Window win, Atom atom, int &x, int &y);
	void setPosition(Window win, Atom atom, int x, int y);

	void *getEwmhPropData(Window win, Atom prop, Atom type, int &num);

	void unsetProperty(Window win, Atom prop);
}

#endif // _FONT_HH_
