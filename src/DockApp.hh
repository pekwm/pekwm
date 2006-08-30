//
// dockapp.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HARBOUR

#ifndef _DOCKAPP_HH_
#define _DOCKAPP_HH_

#include "pekwm.hh"

class ScreenInfo;
class Theme;
class WindowObject;

class DockApp : public WindowObject
{
public:
	DockApp(ScreenInfo *s, Theme *t, Window win);
	~DockApp();

	// START - WindowObject interface.
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	// END - WindowObject interface.

	int getRX(void) const;
	int getRY(void) const;

	inline void setAlive(bool alive) { _is_alive = alive; }
	inline void setLayer(Layer layer) { _layer = layer; }

	inline bool findDockApp(Window win) {
		if ((win != None) &&
				(win == _client_window) || (win == _icon_window))
			return true;
		return false;
	}
	inline bool findDockAppFromFrame(Window win) {
		if ((win != None) && (win == _window))
			return true;
		return false;
	}

	void kill(void);
	void move(int x, int y);
	void resize(unsigned int width, unsigned int height);

	void loadTheme(void);

private:
	void repaint(void);

private:
	ScreenInfo *_scr;
	Theme *_theme;

	Window _dockapp_window;
	Window _client_window, _icon_window;

	Pixmap _background;

	bool _is_alive;
};

#endif // _DOCKAPP_HH_

#endif // HARBOUR
