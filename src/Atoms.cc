//
// Atoms.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "Atoms.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::map;

PekwmAtoms::PekwmAtoms(Display *dpy)
{
	// pekwm atoms
	const unsigned int num = 1;
	char *names[num] = {
		"_PEKWM_FRAME_ID"
	};
	Atom atoms[num];

	XInternAtoms(dpy, names, num, 0, atoms);

	// Insert all atoms into the _atoms map
	for (unsigned int i = 0; i < num; ++i)
		_atoms[PekwmAtomName(i)] = atoms[i];
}

PekwmAtoms::~PekwmAtoms()
{
}

IcccmAtoms::IcccmAtoms(Display *dpy)
{
	// ICCCM atoms
	const unsigned int num = 6;
	char *names[num] = {
		"WM_STATE",
		"WM_CHANGE_STATE",
		"WM_PROTOCOLS",
		"WM_DELETE_WINDOW",
		"WM_COLORMAP_WINDOWS",
		"WM_TAKE_FOCUS"
	};
	Atom atoms[num];

	XInternAtoms(dpy, names, num, 0, atoms);

	// Insert all atoms into the _atoms map
	for (unsigned int i = 0; i < num; ++i)
		_atoms[IcccmAtomName(i)] = atoms[i];
}

IcccmAtoms::~IcccmAtoms()
{
}

EwmhAtoms::EwmhAtoms(Display *dpy)
{
	const unsigned int num = 46;
	char *names[num] = {
		"_NET_SUPPORTED",
		"_NET_CLIENT_LIST", "_NET_CLIENT_LIST_STACKING",
		"_NET_NUMBER_OF_DESKTOPS",
		"_NET_DESKTOP_GEOMETRY", "_NET_DESKTOP_VIEWPORT",
		"_NET_CURRENT_DESKTOP", "_NET_ACTIVE_WINDOW",
		"_NET_WORKAREA", "_NET_SUPPORTING_WM_CHECK",
		"_NET_CLOSE_WINDOW",
		"_NET_WM_NAME", "_NET_WM_DESKTOP",
		"_NET_WM_STRUT",

		"_NET_WM_WINDOW_TYPE",
		"_NET_WM_WINDOW_TYPE_DESKTOP", "_NET_WM_WINDOW_TYPE_DOCK",
		"_NET_WM_WINDOW_TYPE_TOOLBAR", "_NET_WM_WINDOW_TYPE_MENU",
		"_NET_WM_WINDOW_TYPE_UTILITY", "_NET_WM_WINDOW_TYPE_SPLASH",
		"_NET_WM_WINDOW_TYPE_DIALOG", "_NET_WM_WINDOW_TYPE_NORMAL",

		"_NET_WM_STATE",
		"_NET_WM_STATE_MODAL", "_NET_WM_STATE_STICKY",
		"_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_STATE_MAXIMIZED_HORZ",
		"_NET_WM_STATE_SHADED",
		"_NET_WM_STATE_SKIP_TOOLBAR", "_NET_WM_STATE_PAGER",
		"_NET_WM_STATE_HIDDEN", "_NET_WM_STATE_FULLSCREEN",
		"_NET_WM_STATE_ABOVE", "_NET_WM_STATE_BELOW",

		"_NET_WM_ALLOWED_ACTIONS",
		"_NET_WM_ACTION_MOVE", "_NET_WM_ACTION_RESIZE",
		"_NET_WM_ACTION_MINIMIZE", "_NET_WM_ACTION_SHADE",
		"_NET_WM_ACTION_STICK",
		"_NET_WM_ACTION_MAXIMIZE_VERT", "_NET_WM_ACTION_MAXIMIZE_HORZ",
		"_NET_WM_ACTION_FULLSCREEN", "_NET_WM_ACTION_CHANGE_DESKTOP",
		"_NET_WM_ACTION_CLOSE"
	};
	Atom atoms[num];

	XInternAtoms(dpy, names, num, 0, atoms);

	// Insert all items into the _atoms map
	for (unsigned int i = 0; i < num; ++i)
		_atoms[EwmhAtomName(i)] = atoms[i];
}

EwmhAtoms::~EwmhAtoms()
{
}

//! @fn    Atoms* getAtomArray(void) const
//! @brief Builds a array of all atoms in the map.
Atom*
EwmhAtoms::getAtomArray(void) const
{
	Atom *atoms = new Atom[_atoms.size()];

	map<EwmhAtomName, Atom>::const_iterator it = _atoms.begin();
	for (unsigned int i = 0; it != _atoms.end(); ++i, ++it)
		atoms[i] = it->second;

	return atoms;
}
