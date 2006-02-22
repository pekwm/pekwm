//
// PScreen.cc for pekwm
// Copyright (C) 2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "PScreen.hh"
#include "Config.hh"

#include <string>
#include <iostream>
#include <cassert>

#ifdef HAVE_LIMITS
#include <limits>
using std::numeric_limits;
#endif // HAVE_LIMITS

extern "C" {
#ifdef HAVE_SHAPE
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif // HAVE_SHAPE
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#endif // HAVE_XRANDR
#include <X11/keysym.h> // For XK_ entries
}

using std::cerr;
using std::endl;
using std::string;
using std::list;
using std::map;

PScreen* PScreen::_instance = NULL;

//! @brief PScreen::Visual constructor.
//! @param x_visual X Visual to wrap.
PScreen::PVisual::PVisual(Visual *x_visual) : _x_visual(x_visual),
																							_r_shift(0), _r_prec(0),
																							_g_shift(0), _g_prec(0),
																							_b_shift(0), _b_prec(0)
{
	getShiftPrecFromMask(_x_visual->red_mask, _r_shift, _r_prec);
	getShiftPrecFromMask(_x_visual->green_mask, _g_shift, _g_prec);
	getShiftPrecFromMask(_x_visual->blue_mask, _b_shift, _b_prec);
}

//! @brief PScreen::Visual destructor.
PScreen::PVisual::~PVisual(void)
{
}

//! @brief Gets shift and prec from mask.
//! @param mask red,green,blue mask of Visual.
//! @param shift Set to the shift of mask.
//! @param prec Set to the prec of mask.
void
PScreen::PVisual::getShiftPrecFromMask(ulong mask, int &shift, int &prec)
{
	for (shift = 0; !(mask&0x1); ++shift)
		mask >>= 1;

	for (prec = 0; (mask&0x1); ++prec)
		mask >>= 1;
}

//! @brief PScreen constructor
PScreen::PScreen(Display *dpy) :
_dpy(dpy),
_num_lock(0), _scroll_lock(0),
#ifdef HAVE_SHAPE
_has_extension_shape(false), _event_shape(-1),
#endif // HAVE_XRANDR
#ifdef HAVE_XRANDR
_has_extension_xrandr(false), _event_xrandr(-1),
#endif // HAVE_XRANDR
#ifdef HAVE_XINERAMA
_has_xinerama(false), _xinerama_last_head(0), _xinerama_num_heads(0),
_xinerama_struts(NULL), _xinerama_infos(NULL),
#endif // HAVE_XINERAMA
_server_grabs(0), _last_click_id(None)
{
	if (_instance != NULL)
		throw string("PScreen, trying to create multiple instances");
	_instance = this;

	XGrabServer(_dpy);

	_screen = DefaultScreen(_dpy);
	_root = RootWindow(_dpy, _screen);

	_depth = DefaultDepth(_dpy, _screen);
	_visual = new PScreen::PVisual(DefaultVisual(_dpy, _screen));
	_colormap = DefaultColormap(_dpy, _screen);

	_width = WidthOfScreen(ScreenOfDisplay(_dpy, _screen));
	_height = HeightOfScreen(ScreenOfDisplay(_dpy, _screen));

#ifdef HAVE_SHAPE
	{
		int dummy_error;
		_has_extension_shape =
			XShapeQueryExtension(_dpy, &_event_shape, &dummy_error);
	}
#endif // HAVE_SHAPE

#ifdef HAVE_XRANDR
 {
	 int dummy_error;
	 _has_extension_xrandr =
		 XRRQueryExtension(_dpy, &_event_xrandr, &dummy_error);
 }
#endif // HAVE_XRANDR

#ifdef HAVE_XINERAMA
	// check if we have Xinerama extension enabled
	if (XineramaIsActive(_dpy)) {
		_has_xinerama = true;
		_xinerama_last_head = 0;
		_xinerama_infos = XineramaQueryScreens(_dpy, &_xinerama_num_heads);
	} else {
		_has_xinerama = false;
		_xinerama_num_heads = 1;
		_xinerama_infos = 0; // make sure we don't point anywhere we shouldn't
	}

	_xinerama_struts = new Strut[_xinerama_num_heads];
#endif // HAVE_XINERAMA

	// initialize array values
	for (uint i = 0; i < (BUTTON_NO - 1); ++i) {
		_last_click_time[i] = 0;
	}

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

	XSync(_dpy, false);
	XUngrabServer(_dpy);
}

//! @brief PScreen destructor
PScreen::~PScreen(void) {
#ifdef HAVE_XINERAMA
	if (_has_xinerama) { // only free if we first had it
		XFree(_xinerama_infos);
		_xinerama_infos = 0;
	}
	delete [] _xinerama_struts;
#endif // HAVE_XINERAMA

	delete _visual;

	_instance = NULL;
}

//! @brief Grabs the server, counting number of grabs
bool
PScreen::grabServer(void)
{
	if (_server_grabs == 0) {
		XGrabServer(_dpy);
	}

	++_server_grabs;
	return (_server_grabs == 1); // was actually grabbed
}

//! @brief Ungrabs the server, counting number of grabs
bool
PScreen::ungrabServer(bool sync)
{
	if (_server_grabs > 0) {
		--_server_grabs;

		if (_server_grabs == 0) { // no more grabs left
			if (sync) {
				XSync(_dpy, false);
			}
			XUngrabServer(_dpy);
		}
	}
	return (_server_grabs == 0); // is actually ungrabbed
}

//! @brief Grabs the keyboard
bool
PScreen::grabKeyboard(Window win)
{
	if (XGrabKeyboard(_dpy, win, false, GrabModeAsync, GrabModeAsync,
										CurrentTime) == GrabSuccess) {
		return true;
	}
#ifdef DEBUG
	cerr << __FILE__ << "@" << __LINE__ << ": "
			 << "PScreen(" << this << ")::grabKeyboard(" << win << ")" << endl
			 << " *** unable to grab keyboard." << endl;
#endif // DEBUG
	return false;
}

//! @brief Ungrabs the keyboard
bool
PScreen::ungrabKeyboard(void)
{
	XUngrabKeyboard(_dpy, CurrentTime);
	return true;
}

//! @brief Grabs the pointer
bool
PScreen::grabPointer(Window win, uint event_mask, Cursor cursor)
{
	if (XGrabPointer(_dpy, win, false, event_mask, GrabModeAsync, GrabModeAsync,
									 None, cursor, CurrentTime) == GrabSuccess) {
		return true;
	}
#ifdef DEBUG
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PScreen(" << this << ")::grabPointer(" << win << "," << event_mask << "," << cursor << ")" << endl
				 << " *** unable to grab pointer." << endl;
#endif // DEBUG
	return false;
}

//! @brief Ungrabs the pointer
bool
PScreen::ungrabPointer(void)
{
	XUngrabPointer(_dpy, CurrentTime);
	return true;
}

#ifdef HAVE_XRANDR
//! @brief Refetches the root-window size.
void
PScreen::updateGeometry(uint width, uint height)
{
	_width = width;
	_height = height;

	PWinObj::getRootPWinObj()->resize(width, height);
}
#endif // HAVE_XRANDR

#ifdef HAVE_XINERAMA

//! @brief Searches for the head closest to the coordinates x,y.
//! @return The nearest head.  Head numbers are indexed from 0.
uint
PScreen::getNearestHead(int x, int y)
{
	if(_has_xinerama) {
		// set distance to the highest uint value
#ifdef HAVE_LIMITS
		uint min_distance = numeric_limits<uint>::max();
#else //! HAVE_LIMITS
		uint min_distance = ~0;
#endif // HAVE_LIMITS
		uint nearest_head = 0;

		uint distance;
		int head_t, head_b, head_l, head_r;
		for(int head = 0; head < _xinerama_num_heads; head++) {
			head_t = _xinerama_infos[head].y_org;
			head_b = _xinerama_infos[head].y_org + _xinerama_infos[head].height;
			head_l = _xinerama_infos[head].x_org;
			head_r = _xinerama_infos[head].x_org + _xinerama_infos[head].width;

			if(x > head_r) {
				if(y < head_t) {
					// above and right of the head
					distance = calcDistance(x, y, head_r, head_t);
				}	else if(y > head_b) {
					// below and right of the head
					distance = calcDistance(x, y, head_r, head_b);
				} else {
					// right of the head
					distance = calcDistance(x, head_r);
				}
			} else if(x < head_l) {
				if(y < head_t) {
					// above and left of the head
					distance = calcDistance(x, y, head_l, head_t);
				} else if(y > head_b) {
					// below and left of the head
					distance = calcDistance(x, y, head_l, head_b);
				} else {
					// left of the head
					distance = calcDistance(x, head_l);
				}
			} else {
				if(y < head_t) {
					// above the head
					distance = calcDistance(y, head_t);
				} else if(y > head_b) {
					// below the head
					distance = calcDistance(y, head_b);
				} else {
					// on the head
					return head;
				}
			}
			if(distance < min_distance) {
				min_distance = distance;
				nearest_head = head;
			}
		}
		return nearest_head;
	} else {
		return 0;
	}
}

//! @brief Searches for the head that the pointer currently is on.
//! @return Active head number
uint
PScreen::getCurrHead(void)
{
	// is Xinerama extensions enabled?
	if (_has_xinerama) {
		int x = 0, y = 0;
		getMousePosition(x, y);
		return getNearestHead(x, y);
	}

	return 0;
}

//! @brief Fills head_info with info about head nr head
//! @param head Head number to examine
//! @param head_info Returning info about the head
//! @return true if xinerama is off or head exists.
bool
PScreen::getHeadInfo(uint head, Geometry &head_info)
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

#else //! HAVE_XINERAMA

//! @brief
uint
PScreen::getNearestHead(int x, int y)
{
	return 0;
}

//! @brief
uint
PScreen::getCurrHead(void)
{
	return 0;
}

//! @brief
bool
PScreen::getHeadInfo(uint head, Geometry &head_info)
{
	head_info.x = 0;
	head_info.y = 0;
	head_info.width = _width;
	head_info.height = _height;

	return true;
}

#endif // HAVE_XINERAMA

//! @brief
void
PScreen::getHeadInfoWithEdge(uint num, Geometry &head)
{
#ifdef HAVE_XINERAMA
	getHeadInfo(num, head);

	int strut_val;

	// remove the strut area from the head info
	if (head.x == 0) {
		strut_val = (_strut.left > _xinerama_struts[num].left)
			? _strut.left : _xinerama_struts[num].left;
		head.x += strut_val;
		head.width -= strut_val;
	}
	if ((head.x + head.width) == _width) {
		strut_val = (_strut.right > _xinerama_struts[num].right)
			? _strut.right : _xinerama_struts[num].right;
		head.width -= strut_val;
	}
	if (head.y == 0) {
		strut_val = (_strut.top > _xinerama_struts[num].top)
			? _strut.top : _xinerama_struts[num].top;
		head.y += strut_val;
		head.height -= strut_val;
	}
	if ((head.y + head.height) == _height) {
		strut_val = (_strut.bottom > _xinerama_struts[num].bottom)
			? _strut.bottom : _xinerama_struts[num].bottom;
		head.height -= strut_val;
	}
#else //! HAVE_XINERAMA
	head.x = _strut.left;
	head.y = _strut.top;
	head.width = _width - head.x - _strut.right;
	head.height = _height - head.y - _strut.bottom;
#endif // HAVE_XINERAMA
}

void
PScreen::getMousePosition(int &x, int &y)
{
	Window d_root, d_win;
	int win_x, win_y;
	uint mask;

	XQueryPointer(_dpy, _root, &d_root, &d_win, &x, &y, &win_x, &win_y, &mask);
}

uint
PScreen::getButtonFromState(uint state)
{
	uint button = 0;

	if (state&Button1Mask)
		button = BUTTON1;
	else if (state&Button2Mask)
		button = BUTTON2;
	else if (state&Button3Mask)
		button = BUTTON3;
	else if (state&Button4Mask)
		button = BUTTON4;
	else if (state&Button5Mask)
		button = BUTTON5;

	return button;
}

//! @brief Adds a strut to the strut list, updating max strut sizes
void
PScreen::addStrut(Strut *strut)
{
	assert(strut);
	_strut_list.push_back(strut);

	updateStrut();
}

//! @brief Removes a strut from the strut list
void
PScreen::removeStrut(Strut *strut)
{
	assert(strut);
	_strut_list.remove(strut);

	updateStrut();
}

//! @brief Updates strut max size.
void
PScreen::updateStrut(void)
{
#ifdef HAVE_XINERAMA
	// Reset strut data.
	_strut.left = 0;
	_strut.right = 0;
	_strut.top = 0;
	_strut.bottom = 0;

	for (int i = 0; i < _xinerama_num_heads; ++i) {
		_xinerama_struts[i].left = 0;
		_xinerama_struts[i].right = 0;
		_xinerama_struts[i].top = 0;
		_xinerama_struts[i].bottom = 0;
	}

	Strut *strut;
	list<Strut*>::iterator it(_strut_list.begin());
	for(; it != _strut_list.end(); ++it) {
		if ((*it)->head == -1) {
			strut = &_strut;
		} else if ((*it)->head < _xinerama_num_heads) {
			strut = &_xinerama_struts[(*it)->head];
		} else {
			continue;
		}

		if (strut->left < (*it)->left)
			strut->left = (*it)->left;
		if (strut->right < (*it)->right)
			strut->right = (*it)->right;
		if (strut->top < (*it)->top)
			strut->top = (*it)->top;
		if (strut->bottom < (*it)->bottom)
			strut->bottom = (*it)->bottom;
	}
#else // !HAVE_XINERAMA
	_strut.left = 0;
	_strut.right = 0;
	_strut.top = 0;
	_strut.bottom = 0;

	list<Strut*>::iterator it(_strut_list.begin());
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
#endif // HAVE_XINERAMA
}
