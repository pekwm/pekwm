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

// Atoms class
class Atoms {
public:
    static void init();

    static inline Atom getAtom(AtomName name) {
        std::map<AtomName, Atom>::const_iterator it = _atoms.find(name);
        if (it != _atoms.end()) {
            return it->second;
        }
        return None;
    }

    static void setEwmhAtomsSupport(Window win);

private:
    static std::map<AtomName, Atom> _atoms;
};

namespace AtomUtil {
    bool getProperty(Window win, Atom atom, Atom type,
                     ulong expected, uchar **data, ulong *actual);

    void setAtom(Window win, Atom atom, Atom value);
    void setAtoms(Window win, Atom atom, Atom *values, int size);

    void setWindow(Window win, Atom atom, Window value);
    void setWindows(Window win, Atom atom, Window *values, int size);

    bool getLong(Window win, Atom atom, long &value);
    void setLong(Window win, Atom atom, long value);
    void setLongs(Window win, Atom atom, long *values, int size);

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
