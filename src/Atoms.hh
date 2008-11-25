//
// Atoms.hh for pekwm
// Copyright © 2003-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _ATOMS_HH_
#define _ATOMS_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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
    PEKWM_TITLE
};

// ICCCM Atom Names
enum IcccmAtomName {
    WM_NAME,
    WM_ICON_NAME,
    WM_STATE,
    WM_CHANGE_STATE,
    WM_PROTOCOLS,
    WM_DELETE_WINDOW,
    WM_COLORMAP_WINDOWS,
    WM_TAKE_FOCUS,
    WM_WINDOW_ROLE,
    WM_CLIENT_MACHINE
};

// Ewmh Atom Names
enum EwmhAtomName {
    NET_SUPPORTED,
    NET_CLIENT_LIST, NET_CLIENT_LIST_STACKING,
    NET_NUMBER_OF_DESKTOPS,
    NET_DESKTOP_GEOMETRY, NET_DESKTOP_VIEWPORT,
    NET_CURRENT_DESKTOP, NET_DESKTOP_NAMES,
    NET_ACTIVE_WINDOW, NET_WORKAREA, 
    NET_DESKTOP_LAYOUT, NET_SUPPORTING_WM_CHECK,
    NET_CLOSE_WINDOW,
    NET_WM_NAME, NET_WM_VISIBLE_NAME,
    NET_WM_ICON_NAME, NET_WM_VISIBLE_ICON_NAME,
    NET_WM_ICON, NET_WM_DESKTOP,
    NET_WM_STRUT, NET_WM_PID,

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
    EWMH_ACTION_CLOSE,

    UTF8_STRING
};

/**
 * List of non PEKWM, ICCCM and EWMH atoms.
 */
enum MiscAtomName {
    MOTIF_WM_HINTS
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
    ~EwmhAtoms(void);

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

/**
 * Misc atoms.
 */
class MiscAtoms {
public:
    MiscAtoms(void);
    ~MiscAtoms(void);

    /** Get singleton instance. */
    static MiscAtoms *instance(void) { return _instance; }

    /**
     * Get atom from enum type. 
     */
    Atom getAtom(MiscAtomName name) const {
        std::map<MiscAtomName, Atom>::const_iterator it(_atoms.find(name));
        return (it == _atoms.end()) ? None : it->second;
    }

private:
    std::map<MiscAtomName, Atom> _atoms; /**< Map from atom type to atom. */
    static MiscAtoms *_instance; /**< Singleton instance pointer. */
};

namespace AtomUtil {
    bool getProperty(Window win, Atom atom, Atom type, ulong expected, uchar **data, ulong *actual = 0);

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
