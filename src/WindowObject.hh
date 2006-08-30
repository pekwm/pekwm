//
// WindowObject.hh for pekwm
// Copyright (C) 2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _WINDOW_OBJECT_HH_
#define _WINDOW_OBJECT_HH_

#include "pekwm.hh"

class WindowObject
{
public:
	enum Type {
		WO_FRAME = (1<<1),
		WO_FRAMEWIDGET = (1<<2),
		WO_MENU = (1<<3),
		WO_DOCKAPP = (1<<4),
		WO_SCREEN_EDGE = (1<<5),
		WO_NO_TYPE = 0
	};

	WindowObject(Display *dpy);
	virtual ~WindowObject();

	inline Window getWindow(void) { return _window; }
	inline Type getType(void) { return _type; }

	inline int getX(void) { return _gm.x; }
	inline int getY(void) { return _gm.y; }

	inline unsigned int getWidth(void) { return _gm.width; }
	inline unsigned int getHeight(void) { return _gm.height; }

	inline unsigned int getWorkspace(void) { return _workspace; }
	inline unsigned int getLayer(void) { return _layer; }

	inline bool isMapped(void) { return _mapped; }
	inline bool isIconified(void) { return _iconified; }
	inline bool isFocused(void) { return _focused; }
	inline bool isSticky(void) { return _sticky; }

	// interface
	virtual void mapWindow(void);
	virtual void unmapWindow(void);
	virtual void iconify(void);

	virtual void move(int x, int y);
	virtual void resize(unsigned int width, unsigned int height);

	virtual void setWorkspace(unsigned int workspace);
	virtual void setLayer(unsigned int layer);
	virtual void setFocused(bool focused);
	virtual void setSticky(bool sticky);

	virtual void giveInputFocus(void);

	// operators
	virtual bool operator == (const Window &window) {
		return (_window == window);
	}
	virtual bool operator != (const Window &window) {
		return (_window != window);
	}

protected:
	Display *_dpy;
	Window _window;

	Type _type;

	Geometry _gm;
	unsigned int _workspace, _layer;
	bool _mapped, _iconified, _focused, _sticky;
};

#endif // _WINDOW_OBJECT_HH_
