//
// button.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "button.hh"
#include "frame.hh"

#ifdef SHAPE
#include <X11/extensions/shape.h>
#endif // SHAPE


FrameButton::FrameButton(ScreenInfo *s, Frame *parent, ButtonData *data) :
scr(s),
m_parent(parent), m_data(data),
m_state(BUTTON_UNFOCUSED),
m_x(0), m_y(0-data->height),
m_width(data->width), m_height(data->height),
m_left(data->left)
{
	m_window =
		XCreateSimpleWindow(scr->getDisplay(), m_parent->getTitleWindow(),
												m_x, m_y, m_width, m_height,
												0, // no border
												scr->getWhitePixel(), scr->getBlackPixel());

	setState(m_state); // initial state

	XMapWindow(scr->getDisplay(), m_window);

	redraw(); // do a first redraw
}

FrameButton::~FrameButton()
{
	XDestroyWindow(scr->getDisplay(), m_window);
}

//! @fn    Actions getAction(unsigned int button) const
//! @brief Returns the action for the button.
//! @param button Button to match
Actions
FrameButton::getAction(unsigned int button) const
{
	switch (button) {
	case Button1:
		return m_data->action[BUTTON1].action;
		break;
	case Button2:
		return m_data->action[BUTTON2].action;
		break;
	case Button3:
		return m_data->action[BUTTON3].action;
		break;
	case Button4:
		return m_data->action[BUTTON4].action;
		break;
	case Button5:
		return m_data->action[BUTTON5].action;
		break;
	default:
		return NO_ACTION;
		break;
	}
}

void
FrameButton::show(void) const
{
	XMoveWindow(scr->getDisplay(), m_window, m_x, m_y);
}

void
FrameButton::hide(void) const
{
	XMoveWindow(scr->getDisplay(), m_window,  m_x, 0 - m_height);
}

//! @fn    void setState(ButtonState state)
//! @brief Sets the buttons state to state and then redraws it
//! @param state State to set the button too
void
FrameButton::setState(ButtonState state)
{
	if ((state < BUTTON_FOCUSED) || (state > BUTTON_PRESSED))
		return; // invalid state

  m_state = state;

	XSetWindowBackgroundPixmap(scr->getDisplay(), m_window,
														 m_data->pixmap[state]);
#ifdef SHAPE
	XShapeCombineMask(scr->getDisplay(), m_window, ShapeBounding, 0, 0,
										m_data->shape[state], ShapeSet);
#endif // SHAPE

  redraw();
}

//! @fn    void setPosition(int x, int y)
//! @brief Sets the buttons position to x,y, also moves the button window
//! @param x X position.
//! @param y Y position.
void
FrameButton::setPosition(int x, int y)
{
	m_x = x;
	m_y = y;

	XMoveWindow(scr->getDisplay(), m_window, m_x, m_y);
}

//! @fn    Action* getActionFromButton(unsigned int button)
//! @brief Returns the correct action for button button
Action*
FrameButton::getActionFromButton(unsigned int button)
{
	Action* action = NULL;

	switch(button) {
	case Button1:
		action = &m_data->action[BUTTON1];
		break;
	case Button2:
		action = &m_data->action[BUTTON2];
		break;
	case Button3:
		action = &m_data->action[BUTTON3];
		break;
	case Button4:
		action = &m_data->action[BUTTON4];
		break;
	case Button5:
		action = &m_data->action[BUTTON5];
		break;
	}

	return action;
}

//! @fn    void redraw(void) const
//! @brief Redraws the button.
void
FrameButton::redraw(void) const
{
  XClearWindow(scr->getDisplay(), m_window);
}
