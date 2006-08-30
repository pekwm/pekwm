//
// ScreenInfo.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _SCREENINFO_HH_
#define _SCREENINFO_HH_

#include "pekwm.hh"
#include "WindowObject.hh"

extern "C" {
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif // XINERAMA
}

#include <list>
#include <map>

class Strut {
public:
	Strut() : left(0), right(0), top(0), bottom(0) { };
	~Strut() { };
public: // member variables
	CARD32 left, right;
	CARD32 top, bottom;

	inline void operator=(const CARD32 *s) {
		if (sizeof(s) > 3) {
			left = s[0];
			right = s[1];
			top = s[2];
			bottom = s[3];
		}
	}
};

class ScreenInfo
{
public:
	class ScreenEdge : public WindowObject {
	public:
		ScreenEdge(Display *dpy, Window root, ScreenEdgeType edge);
		virtual ~ScreenEdge();

		virtual void mapWindow(void);

		inline ScreenEdgeType getEdge(void) const { return _edge; }

	private:
		ScreenEdgeType _edge;
	};

	enum CursorType {
		CURSOR_TOP_LEFT = BORDER_TOP_LEFT,
		CURSOR_TOP = BORDER_TOP,
		CURSOR_TOP_RIGHT = BORDER_TOP_RIGHT,
		CURSOR_LEFT = BORDER_LEFT,
		CURSOR_RIGHT = BORDER_RIGHT,
		CURSOR_BOTTOM_LEFT = BORDER_BOTTOM_LEFT,
		CURSOR_BOTTOM = BORDER_BOTTOM,
		CURSOR_BOTTOM_RIGHT = BORDER_BOTTOM_RIGHT,
		CURSOR_ARROW, CURSOR_MOVE, CURSOR_RESIZE
	};

	ScreenInfo(Display *d);
	~ScreenInfo();

	inline Display *getDisplay(void) const { return _dpy; }
	inline int getScreenNum(void) const { return _screen; }
	inline Window getRoot(void) const { return _root; }
	inline unsigned int getWidth(void) const { return _width; }
	inline unsigned int getHeight(void) const { return _height; }

	inline int getDepth(void) const { return _depth; }
	inline Visual *getVisual(void) { return _visual; }
	inline Colormap getColormap(void) const { return _colormap; }

	inline unsigned long getWhitePixel(void)
		const { return WhitePixel(_dpy, _screen); }
	inline unsigned long getBlackPixel(void)
		const { return BlackPixel(_dpy, _screen); }

	inline unsigned int getNumLock(void) const { return _num_lock; }
	inline unsigned int getScrollLock(void) const { return _scroll_lock; }

#ifdef SHAPE
	inline bool hasShape(void) const { return _has_shape; }
	inline int getShapeEvent(void) const { return _shape_event; }
#endif // SHAPE

	bool grabServer(void);
	bool ungrabServer(bool sync);
	bool grabKeyboard(Window win);
	bool ungrabKeyboard(void);
	bool grabPointer(Window win, unsigned int event_mask, Cursor cursor);
	bool ungrabPointer(void);

#ifdef XINERAMA
	inline bool hasXinerama(void) const { return _has_xinerama; }
	inline int getNumHeads(void) const { return _xinerama_num_heads; }

	unsigned int getHead(int x, int y);
	unsigned int getCurrHead(void);
	bool getHeadInfo(unsigned int head, Geometry &head_info);
#endif // XINERAMA

	void getMousePosition(int &x, int &y);

	void addStrut(Strut *strut);
	void removeStrut(Strut *rem_strut);
	inline Strut *getStrut(void) { return &_strut; }

	inline Cursor getCursor(CursorType type) { return _cursor_map[type]; }

private:
	Display *_dpy;

	int _screen, _depth;
	unsigned int _width, _height;

	Window _root;
	Visual *_visual;
	Colormap _colormap;

	unsigned int _num_lock;
	unsigned int _scroll_lock;

#ifdef SHAPE
	bool _has_shape;
	int _shape_event;
#endif // SHAPE

	unsigned int _server_grabs;

#ifdef XINERAMA
	bool _has_xinerama;
	unsigned int _xinerama_last_head;
	int _xinerama_num_heads;
	XineramaScreenInfo *_xinerama_infos;
#endif // XINERAMA

	std::list<ScreenEdge*> _edge_list;

	Strut _strut;
	std::list<Strut*> _strut_list;
	std::map<CursorType, Cursor> _cursor_map;
};

#endif // _SCREENINFO_HH_
