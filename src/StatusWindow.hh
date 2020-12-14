//
// StatusWindow.hh for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//


#ifndef _STATUS_WINDOW_HH_
#define _STATUS_WINDOW_HH_

#include "config.h"

#include "pekwm.hh"

class PDecor;

//! @brief Status display window.
class StatusWindow : public PDecor {
public:
    StatusWindow(Theme *theme);
    virtual ~StatusWindow(void);

    //! @brief Returns the StatusWindow instance pointer.
    static StatusWindow *instance(void) { return _instance; }

    void draw(const std::wstring &text, bool do_center = false, Geometry *gm = 0);

private:
    // BEGIN - PDecor interface
    virtual void loadTheme(void);
    // END - PDecor interface
    void unloadTheme(void);

    void render(void);

private:
    PWinObj *_status_wo;
    Pixmap _bg;

    static StatusWindow *_instance;
};

#endif // _STATUS_WINDOW_HH_
