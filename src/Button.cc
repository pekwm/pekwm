//
// Button.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowObject.hh"
#include "Button.hh"

#include "ScreenInfo.hh"

extern "C" {
#ifdef SHAPE
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif // SHAPE
}

using std::list;

Button::Button(ScreenInfo *s, Window parent, ButtonData *data) :
WindowObject(s->getDisplay()),
_scr(s),
_parent(parent), _data(data),
_state(BUTTON_UNFOCUSED),
_left(data->left)
{
	_gm.x = 0;
	_gm.y -= data->height;
	_gm.width = data->width;
	_gm.height = data->height;

	XSetWindowAttributes attr;
	attr.event_mask = NoEventMask;

	_window =
		XCreateWindow(_dpy, _parent,
									_gm.x, _gm.y, _gm.width, _gm.height, 0, // no border
									_scr->getDepth(), CopyFromParent, CopyFromParent,
									CWEventMask, &attr);

	setState(_state); // initial state

	XMapWindow(_dpy, _window);

	redraw(); // do a first redraw
}

Button::~Button()
{
	XDestroyWindow(_dpy, _window);
}

//! @fn
//! @brief
ActionEvent*
Button::findAction(XButtonEvent* ev)
{
	list<ActionEvent>::iterator it = _data->ae_list.begin();
	for (; it != _data->ae_list.end(); ++it) {
		if (it->mod == ev->state && it->sym == ev->button)
			return &*it;
	}

	return NULL;
}

//! @fn    void setState(ButtonState state)
//! @brief Sets the buttons state to state and then redraws it
//! @param state State to set the button too
void
Button::setState(ButtonState state)
{
  _state = state;

	XSetWindowBackgroundPixmap(_dpy, _window, _data->pixmap[state]);
#ifdef SHAPE
	XShapeCombineMask(_dpy, _window, ShapeBounding, 0, 0,
										_data->shape[state], ShapeSet);
#endif // SHAPE

  redraw();
}

//! @fn    void redraw(void) const
//! @brief Redraws the button.
void
Button::redraw(void) const
{
  XClearWindow(_dpy, _window);
}
