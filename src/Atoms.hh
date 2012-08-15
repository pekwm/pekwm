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
    bool getProperty(Window win, Atom atom, Atom type,
                     ulong expected, uchar **data, ulong *actual);

    void setWindow(Window win, Atom atom, Window value);
    void setWindows(Window win, Atom atom, Window *values, int size);

    bool getString(Window win, Atom atom, std::string &value);
    void setString(Window win, Atom atom, const std::string &value);

    bool getUtf8String(Window win, Atom atom, std::wstring &value);
    void setUtf8String(Window win, Atom atom, const std::wstring &value);
    void setUtf8StringArray(Window win, Atom atom, unsigned char *values, unsigned int length);

    bool getTextProperty(Window win, Atom atom, std::string &value);

    void *getEwmhPropData(Window win, Atom prop, Atom type, int &num);

    void unsetProperty(Window win, Atom prop);
}

#endif // _FONT_HH_
