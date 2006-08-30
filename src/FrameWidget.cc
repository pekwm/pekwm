//
// FrameWidget.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowObject.hh"
#include "FrameWidget.hh"

#include "ScreenInfo.hh"
#include "Theme.hh"
#include "Button.hh"
#include "PekwmFont.hh"
#include "Image.hh"

#include <functional>

extern "C" {
#ifdef SHAPE
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>
#endif // SHAPE
}

using std::string;
using std::list;
using std::mem_fun;
#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG

FrameWidget::FrameWidget(ScreenInfo *scr, Theme *theme) : WindowObject(scr->getDisplay()),
_scr(scr), _theme(theme),
_title_window(None), _title_pix(None),
_button_wl(0), _button_wr(0),
_active_title(0), _real_height(0),
_border(true), _title(true), _shaded(false)
{
	// WindowObject attributes
	_type = WO_FRAMEWIDGET;

	XSetWindowAttributes attr;
	attr.override_redirect = true;
	attr.do_not_propagate_mask =
		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask;
	attr.event_mask =
 		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask|
		SubstructureRedirectMask|SubstructureNotifyMask;
		
	// create parent and title window
	_window =
		XCreateWindow(_dpy, _scr->getRoot(),
									_gm.x, _gm.y, _gm.width, _gm.height, 0,
									_scr->getDepth(), CopyFromParent, CopyFromParent,
									CWOverrideRedirect|CWDontPropagate|CWEventMask, &attr);

	attr.override_redirect = false;
	attr.event_mask =
 		ButtonPressMask|ButtonReleaseMask|ButtonMotionMask|EnterWindowMask;

	_title_window =
		XCreateWindow(_dpy, _window,
									-1, -1, 1, 1, 0,
									_scr->getDepth(), CopyFromParent, CopyFromParent,
									CWOverrideRedirect|CWDontPropagate|CWEventMask, &attr);

	// create border windows
	attr.event_mask = ButtonMotionMask;
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		attr.cursor = _scr->getCursor(ScreenInfo::CursorType(i));

		_border_window[i] =
			XCreateWindow(_dpy, _window,
										-1, -1, 1, 1, 0,
										_scr->getDepth(), InputOutput, CopyFromParent,
										CWEventMask|CWCursor, &attr);
	}

	// grab buttons so that we can reply them.
	for (unsigned int i = 0; i <= NUM_BUTTONS; ++i) {
		XGrabButton(_dpy, i, AnyModifier, _window,
								True, ButtonPressMask|ButtonReleaseMask,
								GrabModeSync, GrabModeAsync, None, None);
	}

	loadButtons();

	setBorderFocus();

	// map all sub-windows
	XMapSubwindows(_dpy, _window);
}

FrameWidget::~FrameWidget()
{
	unloadButtons();

	// free windows
	XDestroyWindow(_dpy, _title_window);
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i)
		XDestroyWindow(_dpy, _border_window[i]);
	XDestroyWindow(_dpy, _window);

	// free the titlebar pixmap
	if (_title_pix != None)
		XFreePixmap(_dpy, _title_pix);
}

// START - WindowObject interface.

//! @fn    void resize(unsigned int width, unsigned int height)
//! @brief Resizes the window
void
FrameWidget::resize(unsigned int width, unsigned int height)
{
	if (width == 0)
		width = 1;
	if (height == 0)
		height = 1;

	// special case when resizing a shaded window
	if (_shaded) {
		_real_height = height;
		height = _gm.height;
	}
	WindowObject::resize(width, height);

	if (getTitleHeight()) {
		if (borderTop()) {
			XMoveResizeWindow(_dpy, _title_window,
												borderLeft(), borderTop(),
												_gm.width - borderLeft() - borderRight(),
												getTitleHeight());
		} else {
			XMoveResizeWindow(_dpy, _title_window,
												0, 0, _gm.width, getTitleHeight());
		}

		placeButtons();
		draw();
	}

	if (_border)
		placeBorder();
}

//! @fn    void setFocus(bool focus)
//! @brief Redraws the FrameWidget
//! @param focus Wheter the FrameWidget should be drawed focus or unfocused
void
FrameWidget::setFocused(bool focus)
{
	_focused = focus;

	draw();
	setButtonFocus();
	setBorderFocus();
}

// END - WindowObject interface.

//! @fn    unsigned int getTitleHeight(void) const
//! @brief
unsigned int
FrameWidget::getTitleHeight(void) const {
	if (_title)
		return _theme->getWinTitleHeight();
	return 0;
}

//! @fn    unsigned int getTabWidth(void) const
//! @brief
unsigned int
FrameWidget::getTabWidth(void) const
{
	unsigned int ew = _gm.width - _button_wl - _button_wr;
	if (borderTop())
		ew -= borderLeft() - borderRight();
	ew /= _title_list.size();

	return ew;
}

//! @fn    Button* findButton(Window win)
//! @brief
Button*
FrameWidget::findButton(Window win) {
	std::list<Button*>::iterator it = _button_list.begin();
	for (; it != _button_list.end(); ++it) {
		if (*(*it) == win)
			return (*it);
	}
	return NULL;
}

//! @fn    BorderPosition getBorderPosition(Window win) const
//! @brief Gets the correct border position from a Window
BorderPosition
FrameWidget::getBorderPosition(Window win) const
{
	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		if (_border_window[i] == win)
			return (BorderPosition) i;
	}

	return BORDER_NO_POS;
}

//! @fn    void shade(void)
//! @brief Toggles the window's shade state.
void
FrameWidget::shade(void)
{
	if (_shaded) {
		_gm.height = _real_height; // restore fake height

		XResizeWindow(_dpy, _window, _gm.width, _gm.height);
		lowerBorder();
	} else {
		_real_height = _gm.height; // cache height
		_gm.height = getTitleHeight() + borderTop() + borderBottom();

		XResizeWindow(_dpy, _window, _gm.width, _gm.height);
		raiseBorder();
	}

	_shaded = !_shaded;
	placeBorder();
}

//! @fn    void draw(void)
//! @brief Redraws the title of the FrameWidget
void
FrameWidget::draw(void)
{
	if (!getTitleHeight() || !_title_list.size())
		return;

	PekwmFont *font = _theme->getWinFont(); // convenience
	bool multi = (_title_list.size() > 1); // convenience
	unsigned int width = (_gm.width > 4096) ? 4096 : _gm.width;

	// TO-DO: Implement ImageHandler handling Pixmap cache
	if (_title_pix != None)
		XFreePixmap(_dpy, _title_pix);
	_title_pix = XCreatePixmap(_dpy, _scr->getRoot(),
														  width, getTitleHeight(), _scr->getDepth());

	Image *base = NULL, *separator = NULL;

	// Setup title colors and figure which background image we want
	if (_focused) {
		if (multi) {
			font->setColor(_theme->getWinTextFo());
			base = _theme->getWinImageFo();
		} else {
			font->setColor(_theme->getWinTextFoSe());
			base = _theme->getWinImageFoSe()->getPixmap() ?
				_theme->getWinImageFoSe() : _theme->getWinImageFo();
		}
		separator = _theme->getWinSepFo();
	} else {
		if (multi) {
			font->setColor(_theme->getWinTextUn());
			base = _theme->getWinImageUn();
		} else {
			font->setColor(_theme->getWinTextUnSe());
			base = _theme->getWinImageUnSe()->getPixmap() ?
				_theme->getWinImageUnSe() : _theme->getWinImageUn();
		}
		separator = _theme->getWinSepUn();
	}

	base->draw(_title_pix, 0, 0, width, getTitleHeight());

	unsigned int x = _button_wl;
	unsigned int ew = getTabWidth();

	list<string*>::iterator it = _title_list.begin();
	for (unsigned int i = 0; it != _title_list.end(); ++i, ++it, x += ew) {
		if (multi && (_active_title == i)) {
			if (_focused) {
				_theme->getWinImageFoSe()->draw(_title_pix, x, 0, ew, getTitleHeight());
				font->setColor(_theme->getWinTextFoSe());
			} else {
				_theme->getWinImageUnSe()->draw(_title_pix, x, 0, ew, getTitleHeight());
				font->setColor(_theme->getWinTextUnSe());
			}
		}

		if (i) // draw separator
			separator->draw(_title_pix, x - separator->getWidth(), 0);

		// if the active client is a transient one, the title will be too small
		// to show any names, therefore we don't draw any
		if (*it) {
			font->draw(_title_pix, x, _theme->getWinTitlePadding(),
								 (*it)->c_str(), ew,
								 (TextJustify) _theme->getWinFontJustify());
		}

		// Restore the text color
		if (multi && (_active_title == i)) {
			if (_focused)
				font->setColor(_theme->getWinTextFo());
			else
				font->setColor(_theme->getWinTextUn());
		}
	}

	XSetWindowBackgroundPixmap(_dpy, _title_window, _title_pix);
	XClearWindow(_dpy, _title_window);
}

//! @fn    void drawOutline(const Geometry &gm)
//! @brief Draw's a wire frame.
void
FrameWidget::drawOutline(const Geometry &gm)
{
	if (_shaded) {
		XDrawRectangle(_dpy, _scr->getRoot(), _theme->getInvertGC(),
									 gm.x, gm.y,
									 gm.width, getTitleHeight() + borderTop() + borderBottom());
	} else { // not shaded
		XDrawRectangle(_dpy, _scr->getRoot(), _theme->getInvertGC(),
									 gm.x, gm.y,
									 gm.width, gm.height);
	}
}



//! @fn    void setBorder(bool border)
//! @brief Sets the FrameWidgets border state
void
FrameWidget::setBorder(bool border)
{
	_border = border;

	if (_border)
		raiseBorder();
	else
		lowerBorder();
}

//! @fn    void setTitlebar(bool titlebar)
//! @brief Sets the FrameWidgets titlebar state
void
FrameWidget::setTitlebar(bool titlebar)
{
	_title = titlebar;

	if (_title) {
		unhideButtons();
		XMapRaised(_dpy, _title_window);
	} else {
		hideButtons();
		XUnmapWindow(_dpy, _title_window);
	}

	placeBorder();
}

//! @fn    bool setShape(Window client, bool remove)
//! @brief Shapes the frame.
#ifdef SHAPE
bool
FrameWidget::setShape(Window client, bool remove)
{
	int num, t;
	XRectangle *sh;

	sh = XShapeGetRectangles(_dpy, client, ShapeBounding, &num, &t);

	bool status = false;
	if (num > 1) {
		XShapeCombineShape(_dpy, _window, ShapeBounding,
											 0, 0, client, ShapeBounding, ShapeSet);
		status = true;

	} else if (remove) {
		XRectangle temp;

		temp.x = 0;
		temp.y = 0;
		temp.width = _gm.width;
		temp.height = _gm.height;

		XShapeCombineRectangles(_dpy, _window, ShapeBounding,
														0, 0, &temp, 1, ShapeSet, YXBanded);
	}

	XFree(sh);

	return status;
}
#endif // SHAPE

//! @fn    void setActiveTitle(unsigned int a)
//! @brief Sets the active client and redraws the title.
void
FrameWidget::setActiveTitle(unsigned int a)
{
	_active_title = a;
	if (_active_title >= _title_list.size())
		_active_title = 0;

	draw();
}

//! @fn    void loadTheme(void)
//! @brief Re decorates the frame
void
FrameWidget::loadTheme(void)
{
	// make sure the title is (in)visible
	if (getTitleHeight())
		XRaiseWindow(_dpy, _title_window);
	else
		XLowerWindow(_dpy, _title_window);

	// make sure the top border is (in)visible
	if (!borderTop()) {
		XLowerWindow(_dpy, _border_window[BORDER_TOP]);
		XLowerWindow(_dpy, _border_window[BORDER_TOP_LEFT]);
		XLowerWindow(_dpy, _border_window[BORDER_TOP_RIGHT]);
	} else {
		XRaiseWindow(_dpy, _border_window[BORDER_TOP]);
		XRaiseWindow(_dpy, _border_window[BORDER_TOP_LEFT]);
		XRaiseWindow(_dpy, _border_window[BORDER_TOP_RIGHT]);
	}

	loadButtons();
	setFocused(_focused);
}

//! @fn    void raiseBorder(void)
//! @brief Raises all the border windows.
void
FrameWidget::raiseBorder(void)
{
	if (_border) {
		if (borderTop()) {
			XRaiseWindow(_dpy, _border_window[BORDER_TOP_LEFT]);
			XRaiseWindow(_dpy, _border_window[BORDER_TOP]);
			XRaiseWindow(_dpy, _border_window[BORDER_TOP_RIGHT]);
		}
		if (borderLeft())
			XRaiseWindow(_dpy, _border_window[BORDER_LEFT]);
		if (borderRight())
			XRaiseWindow(_dpy, _border_window[BORDER_RIGHT]);
		if (borderBottom()) {
			XRaiseWindow(_dpy, _border_window[BORDER_BOTTOM_LEFT]);
			XRaiseWindow(_dpy, _border_window[BORDER_BOTTOM]);
			XRaiseWindow(_dpy, _border_window[BORDER_BOTTOM_RIGHT]);
		}
	}
}

//! @fn    void lowerBorder(void)
//! @brief Lower all the border windows.
void
FrameWidget::lowerBorder(void)
{
	if (!_border) {
		for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
			XLowerWindow(_dpy, _border_window[i]);
		}
	}
}

//! @fn    void setButtonFocus(void)
//! @brief Redraws the buttons.
void
FrameWidget::setButtonFocus(void)
{
	list<Button*>::iterator it = _button_list.begin();
	for (; it != _button_list.end(); ++it) {
		(*it)->setState(_focused ? BUTTON_FOCUSED : BUTTON_UNFOCUSED);
	}
}

//! @fn    void setBorderFocus(void)
//! @brief Redraws the border.
void
FrameWidget::setBorderFocus(void)
{
	if (!_border)
		return;

	Image **data = NULL;

	if (_focused)
		data = _theme->getWinFocusedBorder();
	else
		data = _theme->getWinUnfocusedBorder();

	for (unsigned int i = 0; i < BORDER_NO_POS; ++i) {
		XSetWindowBackgroundPixmap(_dpy,
															 _border_window[i], data[i]->getPixmap());
		XClearWindow(_dpy, _border_window[i]);
	}
}

//! @fn    void hideButtons(void)
//! @brief
void
FrameWidget::hideButtons(void)
{
	for_each(_button_list.begin(), _button_list.end(),
					 mem_fun(&Button::unmapWindow));
}

//! @fn    void unhideButtons(void)
//! @brief
void
FrameWidget::unhideButtons(void)
{
	for_each(_button_list.begin(), _button_list.end(),
					 mem_fun(&Button::mapWindow));
}

//! @fn    void placeBorder(void)
//! @brief Moves and resizes the border windows
void
FrameWidget::placeBorder(void)
{
	if (borderTop() > 0) {
		XMoveResizeWindow(_dpy, _border_window[BORDER_TOP],
											borderTopLeft(), 0,
											_gm.width - borderTopLeft() - borderTopRight(),
											borderTop());

		XMoveResizeWindow(_dpy, _border_window[BORDER_TOP_LEFT],
											0, 0,
											borderTopLeft(), borderTop());
		XMoveResizeWindow(_dpy, _border_window[BORDER_TOP_RIGHT],
											_gm.width - borderTopRight(), 0,
											borderTopRight(), borderTop());

		if (borderLeft()) {
			XMoveResizeWindow(_dpy, _border_window[BORDER_LEFT],
												0, borderTop(),
												borderLeft(),
												_gm.height - borderTop() - borderBottom());
		}

		if (borderRight()) {
			XMoveResizeWindow(_dpy, _border_window[BORDER_RIGHT],
												_gm.width - borderRight(), borderTop(),
												borderRight(),
												_gm.height - borderTop() - borderBottom());
		}
	} else {
		if (borderLeft()) {
			XMoveResizeWindow(_dpy, _border_window[BORDER_LEFT],
												0, getTitleHeight(),
												borderLeft(),
												_gm.height - getTitleHeight() - borderBottom());
		}

		if (borderRight()) {
			XMoveResizeWindow(_dpy, _border_window[BORDER_RIGHT],
												_gm.width - borderRight(), getTitleHeight(),
												borderRight(),
												_gm.height - getTitleHeight() - borderBottom());
		}
	}


	if (borderBottom()) {
		if (_shaded) {
			XMoveResizeWindow(_dpy, _border_window[BORDER_BOTTOM],
												borderBottomLeft(), getTitleHeight() + borderTop(),
												_gm.width - borderBottomLeft() - borderBottomRight(), borderBottom());
			XMoveResizeWindow(_dpy, _border_window[BORDER_BOTTOM_LEFT],
												0, getTitleHeight() + borderTop(),
												borderBottomLeft(), borderBottom());
			XMoveResizeWindow(_dpy, _border_window[BORDER_BOTTOM_RIGHT],
												_gm.width - borderBottomRight(), getTitleHeight() + borderTop(),
												borderBottomRight(), borderBottom());
		} else {
			XMoveResizeWindow(_dpy, _border_window[BORDER_BOTTOM],
												borderBottomLeft(), _gm.height - borderBottom(),
												_gm.width - borderBottomLeft() - borderBottomRight(), borderBottom());
			XMoveResizeWindow(_dpy, _border_window[BORDER_BOTTOM_LEFT],
												0, _gm.height - borderBottom(),
												borderBottomLeft(), borderBottom());
			XMoveResizeWindow(_dpy, _border_window[BORDER_BOTTOM_RIGHT],
												_gm.width - borderBottomRight(), _gm.height - borderBottom(),
												borderBottomRight(), borderBottom());
		}
	}
}

//! @fn    void loadButtons(void)
//! @brief Loads the title buttons.
void
FrameWidget::loadButtons(void)
{
	if (_button_list.size())
		unloadButtons(); // unload old buttons

	_button_wl = _button_wr = 0;

	// add buttons to the frame
	list<ButtonData*> *b_list = _theme->getButtonList();
	list<ButtonData*>::iterator it = b_list->begin();
	for (; it != b_list->end(); ++it) {
		_button_list.push_back(new Button(_scr, _title_window, *it));
		_button_list.back()->mapWindow();

		if ((*it)->left)
			_button_wl += (*it)->width;
		else
			_button_wr += (*it)->width;
	}
}

//! @fn    void unloadButtons(void)
//! @brief Unloads title buttons.
void
FrameWidget::unloadButtons(void)
{
	if (!_button_list.size())
		return;
	list<Button*>::iterator it = _button_list.begin();
	for (; it != _button_list.end(); ++it)
		delete *it;
	_button_list.clear();
}

//! @fn    void placeButtons(void)
//! @brief Updates the position of buttons in the title.
void
FrameWidget::placeButtons(void)
{
	int left = 0, right = _gm.width;
	if (borderTop())
		right -= borderLeft() + borderRight();

	list<Button*>::iterator it = _button_list.begin();
	for (; it != _button_list.end(); ++it) {
		if ((*it)->isLeft()) {
			(*it)->move(left, 0);
			left += (*it)->getWidth();
		} else {
			right -= (*it)->getWidth();
			(*it)->move(right, 0);
		}
	}
}
