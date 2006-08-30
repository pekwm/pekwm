//
// ScreenInfo.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "ScreenInfo.hh"

#include <iostream>

#ifdef SHAPE
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif // SHAPE
#include <X11/keysym.h> // For XK_ entries
#include <X11/cursorfont.h>

using std::cerr;
using std::endl;
using std::list;
using std::map;

ScreenInfo::ScreenEdge::ScreenEdge(Display *dpy, Window root, ScreenEdgeType edge) :
WindowObject(dpy),
_edge(edge)
{
	_type = WO_SCREEN_EDGE;
	_layer = LAYER_NONE; // hack, goes over LAYER_MENU
	_sticky = true; // don't map/unmap
	_iconified = true; // hack, to be ignored when placing


	XSetWindowAttributes sattr;
	sattr.override_redirect = true;
	sattr.event_mask = EnterWindowMask|LeaveWindowMask;

	_window =
		XCreateWindow(_dpy, root,
									0, 0, 1, 1, 0,
									CopyFromParent, InputOnly, CopyFromParent,
									CWOverrideRedirect|CWEventMask, &sattr);
}

ScreenInfo::ScreenEdge::~ScreenEdge()
{
	XDestroyWindow(_dpy, _window);
}

//! @fn    void mapWindow(void)
//! @brief Overleaded mapWindow to make iconified always true
void
ScreenInfo::ScreenEdge::mapWindow(void)
{
	if (_mapped)
		return;

	WindowObject::mapWindow();
	_iconified = true;
}

ScreenInfo::ScreenInfo(Display *d) :
_dpy(d),
_num_lock(0), _scroll_lock(0),
_server_grabs(0)
{
	XGrabServer(_dpy);

	_screen = DefaultScreen(_dpy);
	_root = RootWindow(_dpy, _screen);

	_depth = DefaultDepth(_dpy, _screen);
	_visual = DefaultVisual(_dpy, _screen);
	_colormap = DefaultColormap(_dpy, _screen);

	_width = WidthOfScreen(ScreenOfDisplay(_dpy, _screen));
	_height = HeightOfScreen(ScreenOfDisplay(_dpy, _screen));

#ifdef SHAPE
	int dummy_error;
	_has_shape = XShapeQueryExtension(_dpy, &_shape_event, &dummy_error);
#endif // SHAPE

#ifdef XINERAMA
	// check if we have Xinerama extension enabled
	if (XineramaIsActive(_dpy)) {
		_has_xinerama = true;
		_xinerama_last_head = 0;
		_xinerama_infos = XineramaQueryScreens(_dpy, &_xinerama_num_heads);
	} else {
		_has_xinerama = false;
		_xinerama_infos = 0; // make sure we don't point anywhere we shouldn't
	}
#endif // XINERAMA

	// Figure out what keys the Num and Scroll Locks are

	// This code is strongly based on code from WindowMaker
	int mask_table[8] = {
		ShiftMask,LockMask,ControlMask,Mod1Mask,
		Mod2Mask, Mod3Mask, Mod4Mask, Mod5Mask
	};

	KeyCode num_lock = XKeysymToKeycode(_dpy, XK_Num_Lock);
	KeyCode scroll_lock = XKeysymToKeycode(_dpy, XK_Scroll_Lock);

	XModifierKeymap *modmap = XGetModifierMapping(_dpy);
	if (modmap && (modmap->max_keypermod > 0)) {

		for (int i = 0; i < (8*modmap->max_keypermod); ++i) {
	    if (num_lock && (modmap->modifiermap[i] == num_lock)) {
				_num_lock = mask_table[i/modmap->max_keypermod];
			} else if (scroll_lock && (modmap->modifiermap[i] == scroll_lock)) {
				_scroll_lock = mask_table[i/modmap->max_keypermod];
			}
		}
	}

	if (modmap)
		XFreeModifiermap(modmap);
	// Stop WindowMaker like code

	// Create resize cursors
	_cursor_map[CURSOR_TOP_LEFT] = XCreateFontCursor(_dpy, XC_top_left_corner);
	_cursor_map[CURSOR_TOP] = XCreateFontCursor(_dpy, XC_top_side);
	_cursor_map[CURSOR_TOP_RIGHT] = XCreateFontCursor(_dpy, XC_top_right_corner);
	_cursor_map[CURSOR_LEFT] = XCreateFontCursor(_dpy, XC_left_side);
	_cursor_map[CURSOR_RIGHT] = XCreateFontCursor(_dpy, XC_right_side);
	_cursor_map[CURSOR_BOTTOM_LEFT] =
		XCreateFontCursor(_dpy, XC_bottom_left_corner);
	_cursor_map[CURSOR_BOTTOM] = XCreateFontCursor(_dpy, XC_bottom_side);
	_cursor_map[CURSOR_BOTTOM_RIGHT] =
		XCreateFontCursor(_dpy, XC_bottom_right_corner);
	// Create other cursors
	_cursor_map[CURSOR_ARROW] = XCreateFontCursor(_dpy, XC_left_ptr);
	_cursor_map[CURSOR_MOVE] =  XCreateFontCursor(_dpy, XC_fleur);
	_cursor_map[CURSOR_RESIZE] = XCreateFontCursor(_dpy, XC_plus);

	XSync(_dpy, false);
	XUngrabServer(_dpy);
}

ScreenInfo::~ScreenInfo(void) {
#ifdef XINERAMA
	if (_has_xinerama) { // only free if we first had it
		XFree(_xinerama_infos);
		_xinerama_infos = 0;
	}
#endif // XINERAMA

	list<ScreenEdge*>::iterator s_it = _edge_list.begin();
	for (; s_it != _edge_list.end(); ++s_it)
		delete *s_it;

	map<CursorType, Cursor>::iterator c_it = _cursor_map.begin();
	for (; c_it != _cursor_map.end(); ++c_it)
		XFreeCursor(_dpy, c_it->second);
}

//! @fn    bool grabServer(void)
//! @brief
bool
ScreenInfo::grabServer(void)
{
	if (!_server_grabs)
		XGrabServer(_dpy);
	++_server_grabs;
	return true;
}

//! @fn    bool ungrabServer(bool sync)
//! @brief 
bool
ScreenInfo::ungrabServer(bool sync)
{
	if (_server_grabs) {
		--_server_grabs;

		if (!_server_grabs) {
			if (sync)
				XSync(_dpy, false);
			XUngrabServer(_dpy);
		}
	}
	return true;
}

//! @fn    bool grabKeyboard(Window win)
//! @brief
bool
ScreenInfo::grabKeyboard(Window win)
{
	if (XGrabKeyboard(_dpy, win, false, GrabModeAsync, GrabModeAsync,
										CurrentTime) == GrabSuccess)
		return true;
	return false;
}

//! @fn    bool ungrabKeyboard(void)
//! @brief
bool
ScreenInfo::ungrabKeyboard(void)
{
	XUngrabKeyboard(_dpy, CurrentTime);
	return true;
}

//! @fn    bool grabPointer(Window win, unsigned int event_mask, Cursor cursor)
//! @brief
bool
ScreenInfo::grabPointer(Window win, unsigned int event_mask, Cursor cursor)
{
	if (XGrabPointer(_dpy, win, false, event_mask, GrabModeAsync, GrabModeAsync,
									 None, cursor, CurrentTime) == GrabSuccess)
		return true;
	return false;
}

//! @fn    bool ungrabPointer(void)
//! @brief
bool
ScreenInfo::ungrabPointer(void)
{
	XUngrabPointer(_dpy, CurrentTime);
	return true;
}


#ifdef XINERAMA

//! @fn    unsigned int getHead(int x, int y)
//! @brief Searches for the head at the coordinates x,y.
//! @return If it fails or Xinerama isn't on, return 0.
unsigned int
ScreenInfo::getHead(int x, int y) {
	// is Xinerama extensions enabled?
	if (_has_xinerama) {
		// check if last head is still active
		if ((_xinerama_infos[_xinerama_last_head].x_org <= x) &&
				((_xinerama_infos[_xinerama_last_head].x_org +
				_xinerama_infos[_xinerama_last_head].width) > x) &&
				(_xinerama_infos[_xinerama_last_head].y_org <= y) &&
				((_xinerama_infos[_xinerama_last_head].y_org +
				_xinerama_infos[_xinerama_last_head].height) > y)) {
			return _xinerama_last_head;

		} else {
			// go trough all the heads, and search
			for (int i = 0; i < (signed) _xinerama_num_heads; i++) {
				if ((_xinerama_infos[i].x_org <= x) &&
						((_xinerama_infos[i].x_org + _xinerama_infos[i].width) > x) &&
						(_xinerama_infos[i].y_org <= y) &&
						((_xinerama_infos[i].y_org + _xinerama_infos[i].height) > y)) {
					return _xinerama_last_head = i;
				}
			}
		}
	}

	return 0;
}

//! @fn    unsigned int getCurrHead(void)
//! @brief Searches for the head that the pointer currently is on.
//! @return Active head number
unsigned int
ScreenInfo::getCurrHead(void)
{
	// is Xinerama extensions enabled?
	if (_has_xinerama) {
		int x = 0, y = 0;
		getMousePosition(x, y);
		return getHead(x, y);
	}

	return 0;
}

//! @fn    bool getHeadInfo(unsigned int head, Geometry &head_info)
//! @brief Fills head_info with info about head nr head
//! @param head Head number to examine
//! @param head_info Returning info about the head
//! @return true if xinerama is off or head exists.
bool
ScreenInfo::getHeadInfo(unsigned int head, Geometry &head_info)
{
	if (_has_xinerama) {
		if ((signed) head  < _xinerama_num_heads) {
			head_info.x = _xinerama_infos[head].x_org;
			head_info.y = _xinerama_infos[head].y_org;
			head_info.width = _xinerama_infos[head].width;
			head_info.height = _xinerama_infos[head].height;
		} else {
#ifdef DEBUG
			cerr << __FILE__ << "@" << __LINE__ << ": "
					 << "Head: " << head << " doesn't exist!" << endl;
#endif // DEBUG
			return false;
		}
	} else { // fill it up with "ordinary info"
		head_info.x = 0;
		head_info.y = 0;
		head_info.width = _width;
		head_info.height = _height;
	}

	return true;
}

#endif // XINERAMA

//! @fn    void getMousePosition(int &x, int &y)
//! @brief Sets x and y to the current mouse position.
//! @param x Pointer to int where to store x position
//! @param y Pointer to int where to store y position
void
ScreenInfo::getMousePosition(int &x, int &y)
{
	Window d_root, d_win;
	int win_x, win_y;
	unsigned int mask;

	XQueryPointer(_dpy, _root, &d_root, &d_win, &x, &y, &win_x, &win_y, &mask);
}

//! @fn    void addStrut(Strut *strut)
//! @brief Adds a strut to the strut list
void
ScreenInfo::addStrut(Strut *strut)
{
	if (!strut)
		return;

	_strut_list.push_back(strut);

	list<Strut*>::iterator it = _strut_list.begin();
	for(; it != _strut_list.end(); ++it) {
		if (_strut.left < (*it)->left)
			_strut.left = (*it)->left;
		if (_strut.right < (*it)->right)
			_strut.right = (*it)->right;
		if (_strut.top < (*it)->top)
			_strut.top = (*it)->top;
		if (_strut.bottom < (*it)->bottom)
			_strut.bottom = (*it)->bottom;
	}
}

//! @fn    void removeStrut(Strut *strut)
//! @brief Removes a strut from the strut list
void
ScreenInfo::removeStrut(Strut *strut)
{
	if (!strut)
		return;

	_strut_list.remove(strut);

	_strut.left = 0;
	_strut.right = 0;
	_strut.top = 0;
	_strut.bottom = 0;

	if (_strut_list.size()) {
		list<Strut*>::iterator it = _strut_list.begin();
		for(; it != _strut_list.end(); ++it) {
			if (_strut.left < (*it)->left)
				_strut.left = (*it)->left;
			if (_strut.right < (*it)->right)
				_strut.right = (*it)->right;
			if (_strut.top < (*it)->top)
				_strut.top = (*it)->top;
			if (_strut.bottom < (*it)->bottom)
				_strut.bottom = (*it)->bottom;
		}
	}
}

