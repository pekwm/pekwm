//
// button.hh for pekwm
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

#ifndef _BUTTON_HH_
#define _BUTTON_HH_

#include "pekwm.hh"
#include "screeninfo.hh"

class Frame;

class FrameButton
{
public:
	enum ButtonState {
		BUTTON_FOCUSED = 0, BUTTON_UNFOCUSED,
		BUTTON_PRESSED, BUTTON_NO_STATE
	};
	enum ButtonNum { BUTTON1, BUTTON2, BUTTON3, BUTTON4, BUTTON5, NUM_BUTTONS };

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
		Action action[NUM_BUTTONS];

		unsigned int width, height;

		bool left; // left or right side of the bar
	};

	FrameButton(ScreenInfo *s, Frame *parent, ButtonData *data);
	~FrameButton();

	Window getWindow(void) const { return m_window; }

	void show(void) const;
	void hide(void) const;

	Action* getActionFromButton(unsigned int button);

	Actions getAction(unsigned int button) const;
	inline unsigned int getWidth(void) const { return m_width; }
	inline unsigned int getHeight(void) const { return m_height; }
	inline int getX(void) const { return m_x; }
	inline int getY(void) const { return m_y; }
	inline bool isLeft(void) const { return m_left; }

	void redraw(void) const;
	void setState(ButtonState state);
	void setPosition(int x, int y);

private:
	ScreenInfo *scr;
	Frame *m_parent;
	ButtonData *m_data;

	Window m_window;

	ButtonState m_state;
	int m_x, m_y;
	unsigned int m_width, m_height;
	bool m_left;
};

#endif // _BUTTON_HH_
