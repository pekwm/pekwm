//
// Harbour.hh for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifndef _HARBOUR_HH_
#define _HARBOUR_HH_

#include "pekwm.hh"

class DockApp;
class Strut;

#include "Action.hh"

class Harbour
{
public:
    Harbour(void);
    ~Harbour(void);

    void addDockApp(DockApp* da);
    void removeDockApp(DockApp* da);
    void removeAllDockApps(void);

    DockApp* findDockApp(Window win);
    DockApp* findDockAppFromFrame(Window win);

    inline uint getSize(void) const { return _size; }

#ifdef HAVE_XRANDR
    void updateGeometry(void);
#endif // HAVE_XRANDR

    void restack(void);
    void rearrange(void);
    void loadTheme(void);
    void updateHarbourSize(void);

    void setStateHidden(StateAction sa);

    void handleButtonEvent(XButtonEvent* ev, DockApp* da);
    void handleMotionNotifyEvent(XMotionEvent* ev, DockApp* da);
    void handleConfigureRequestEvent(XConfigureRequestEvent* ev, DockApp* da);

private:
    void placeDockApp(DockApp *da);
    void placeDockAppsSorted(void);
    void placeDockAppInsideScreen(DockApp *da);

    void getPlaceStartPosition(DockApp *da, int &x, int &y, bool &inc_x);
    void insertDockAppSorted(DockApp *da);

    void updateStrutSize(void);

    vector<DockApp*> _dapps;
    bool _hidden;
    uint _size;
    Strut *_strut;
    int _last_button_x, _last_button_y;
};

#endif // _HARBOUR_HH_
