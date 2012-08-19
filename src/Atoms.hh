//
// Atoms.hh for pekwm
// Copyright © 2003-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _ATOMS_HH_
#define _ATOMS_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "pekwm.hh"
#include "Types.hh"

#include <string>
#include <map>

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xatom.h>
}

namespace AtomUtil {
    bool getTextProperty(Window win, Atom atom, std::string &value);

    void *getEwmhPropData(Window win, Atom prop, Atom type, int &num);
}

#endif // _FONT_HH_
