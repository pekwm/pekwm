//
// Button.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _BUTTON_HH_
#define _BUTTON_HH_

#include "pekwm.hh"
#include "Action.hh"

class ScreenInfo;
class WindowObject;

class ButtonData {
public:
	ButtonData() {
		for (unsigned int i = 0; i < BUTTON_NO_STATE; ++i) {
			pixmap[i] = None;
			shape[i] = None;
		}
	}
	Pixmap pixmap[BUTTON_NO_STATE];
	Pixmap shape[BUTTON_NO_STATE];

	std::list<ActionEvent> ae_list;

	unsigned int width, height;

	bool left; // left or right side of the bar
};

class Button : public WindowObject
{
public:
	Button(ScreenInfo* scr, Window parent, ButtonData* data);
	~Button();

	inline bool isLeft(void) const { return _left; }

	ActionEvent* findAction(XButtonEvent* ev);

	void redraw(void) const;
	void setState(ButtonState state);

private:
	ScreenInfo *_scr;
	Window _parent;
	ButtonData *_data;

	ButtonState _state;
	bool _left;
};

#endif // _BUTTON_HH_
