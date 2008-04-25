//
// DockApp.hh for pekwm
// Copyright © 2003-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _DOCKAPP_HH_
#define _DOCKAPP_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HARBOUR

#include "pekwm.hh"
#include "AutoProperties.hh"

class PScreen;
class Theme;
class PWinObj;

//! @brief DockApp handling class.
class DockApp : public PWinObj
{
public:
    DockApp(PScreen *scr, Theme *theme, Window win);
    ~DockApp(void);

    // START - PWinObj interface.
    virtual void mapWindow(void);
    virtual void unmapWindow(void);
    // END - PWinObj interface.

    //! @brief Returns DockApp client width.
    inline uint getClientWidth(void) const { return _c_gm.width; }
    //! @brief Returns DockApp client height.
    inline uint getClientHeight(void) const { return _c_gm.height; }
    //! @brief Returns DockApp position.
    inline int getPosition(void) const { return _position; }
    //! @brief Sets alive state of DockApp.
    inline void setAlive(bool alive) { _is_alive = alive; }

    //! @brief Matches win against DockApp client window(s).
    inline bool findDockApp(Window win) {
      if ((win != None) && ((win == _client_window) || (win == _icon_window))) {
        return true;
      }
      return false;
    }
    //! @brief Matches win against DockApp window.
    inline bool findDockAppFromFrame(Window win) {
        if ((win != None) && (win == _window))
            return true;
        return false;
    }

    void kill(void);
    void resize(uint width, uint height);

    void loadTheme(void);

private:
    void repaint(void);

    void updateSize(void);
    void validateSize(void);

    void readClassHint(void);
    void readAutoProperties(void);

private:
    PScreen *_scr;
    Theme *_theme;

    Window _dockapp_window;
    Window _client_window, _icon_window;

    ClassHint _class_hint;

    Geometry _c_gm;
    int _position; // used in sorted mode

    Pixmap _background;

    bool _is_alive;
};

#endif // _DOCKAPP_HH_

#endif // HARBOUR
