//
// FrameWidget.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _FRAMEWIDGET_HH_
#define _FRAMEWIDGET_HH_

#include "pekwm.hh"
#include "Image.hh" // border inlines need it
#include "Theme.hh" // border inlines need it

class ScreenInfo;
class Button;
class WindowObject;

#include <string>
#include <list>

class FrameWidget : public WindowObject
{
public:
	FrameWidget(ScreenInfo *scr, Theme *theme);
	~FrameWidget();

	// START - WindowObject interface.
	virtual void resize(unsigned int width, unsigned int height);

	virtual void setFocused(bool focused);
	// END - WindowObject interface.

	unsigned int getTitleHeight(void) const;
	unsigned int getTabWidth(void) const;
	inline unsigned int getButtonWidthL(void) const { return _button_wl; }
	inline unsigned int getButtonWidthR(void) const { return _button_wr; }

	// windows
	inline Window getTitleWindow(void) const { return _title_window; }

	// states
	inline bool hasBorder(void) const { return _border; }
	inline bool hasTitlebar(void) const { return _title; }

	Button* findButton(Window win);
	BorderPosition getBorderPosition(Window win) const;

	void shade(void);

	void draw(void);
	void drawOutline(const Geometry &gm);
	void setBorder(bool border);
	void setTitlebar(bool titlebar);
#ifdef SHAPE
	bool setShape(Window client, bool remove);
#endif // SHAPE

	inline void clearTitleList(void) { _title_list.clear(); }
	inline void addTitle(std::string* s) { _title_list.push_back(s); }
	void setActiveTitle(unsigned int a);

	void loadTheme(void);

	// operator ==
	inline bool operator == (const Window &win) {
		if ((win == _window) || (win == _title_window))
			return true;
		return false;
	}

	// border
	void raiseBorder(void);
	void lowerBorder(void);

	// border inlines
	inline unsigned int
	FrameWidget::borderTop(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_TOP]->getHeight()
					 : _theme->getWinUnfocusedBorder()[BORDER_TOP]->getHeight()) : 0);
	}
	inline unsigned int
	FrameWidget::borderTopLeft(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_TOP_LEFT]->getWidth()
					 : _theme->getWinUnfocusedBorder()[BORDER_TOP_LEFT]->getWidth()) : 0);
	}
	inline unsigned int
	FrameWidget::borderTopRight(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_TOP_RIGHT]->getWidth()
					 : _theme->getWinUnfocusedBorder()[BORDER_TOP_RIGHT]->getWidth()) : 0);
	}
	inline unsigned int
	FrameWidget::borderBottom(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_BOTTOM]->getHeight()
					 : _theme->getWinUnfocusedBorder()[BORDER_BOTTOM]->getHeight()) : 0);
	}
	inline unsigned int
	FrameWidget::borderBottomLeft(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_BOTTOM_LEFT]->getWidth()
					 : _theme->getWinUnfocusedBorder()[BORDER_BOTTOM_LEFT]->getWidth()) : 0);
	}
	inline unsigned int
	FrameWidget::borderBottomRight(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_BOTTOM_RIGHT]->getWidth()
					 : _theme->getWinUnfocusedBorder()[BORDER_BOTTOM_RIGHT]->getWidth()) : 0);
	}
	inline unsigned int
	FrameWidget::borderLeft(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_LEFT]->getWidth()
					 : _theme->getWinUnfocusedBorder()[BORDER_LEFT]->getWidth()) : 0);
	}
	inline unsigned int
	FrameWidget::borderRight(void) const {
		return (_border ? (_focused
					 ? _theme->getWinFocusedBorder()[BORDER_RIGHT]->getWidth()
					 : _theme->getWinUnfocusedBorder()[BORDER_RIGHT]->getWidth()) : 0);
	}
	// end border inlines

private:
	void setButtonFocus(void);
	void setBorderFocus(void);
	void placeBorder(void);

	void loadButtons(void);
	void unloadButtons(void);
	void hideButtons(void);
	void unhideButtons(void);
	void placeButtons(void);

private:
	ScreenInfo *_scr;
	Theme *_theme;

	Window _title_window;
	Window _border_window[BORDER_NO_POS]; // border windows

	Pixmap _title_pix;

	std::list<Button*> _button_list; // list of buttons
	unsigned int _button_wl, _button_wr; // button width left and right

	std::list<std::string*> _title_list; // list of client titles
	unsigned int _active_title; // client that's active.

	unsigned int _real_height; // cache real height when shading.
	bool _border, _title, _shaded; // toggles
};

#endif // _FRAMEWIDGET_HH_
