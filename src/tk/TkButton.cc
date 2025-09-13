//
// TkButton.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "TkButton.hh"

TkButton::TkButton(Theme::DialogData* data, PWinObj& parent,
		   stop_fun stop, int retcode, const std::string& text)
	: TkWidget(data, parent),
	  _stop(stop),
	  _retcode(retcode),
	  _text(text),
	  _state(BUTTON_STATE_FOCUSED)
{
	_gm.width = TkButton::widthReq();
	_gm.height = TkButton::heightReq(_gm.width);
	_background.resize(_gm.width, _gm.height);

	XSetWindowAttributes attr;
	attr.background_pixmap = _background.getDrawable();
	attr.override_redirect = True;
	attr.event_mask =
		ButtonPressMask|ButtonReleaseMask|
		EnterWindowMask|LeaveWindowMask;

	setWindow(X11::createWindow(_parent.getWindow(),
				    0, 0, _gm.width, _gm.height, 0,
				    CopyFromParent, InputOutput,
				    CopyFromParent,
				    CWEventMask|CWOverrideRedirect|
				    CWBackPixmap, &attr));
	X11::mapWindow(_window);
}

TkButton::~TkButton(void)
{
}


void
TkButton::setHeight(uint height)
{
	if (_gm.height == height) {
		return;
	}
	_gm.width = TkButton::widthReq();
	_gm.height = height;
	resized();
}

bool
TkButton::setState(Window window, ButtonState state)
{
	if (window != _window) {
		return false;
	}
	_state = state;
	render();
	return true;
}

bool
TkButton::click(Window window)
{
	if (window != _window) {
		return false;
	}
	if (_state == BUTTON_STATE_HOVER
	    || _state == BUTTON_STATE_PRESSED) {
		_stop(_retcode);
	}
	return true;
}

void
TkButton::place(int x, int y, uint, uint tot_height)
{
	TkWidget::place(x, y, _gm.width, tot_height);
	X11::moveWindow(_window, _gm.x, _gm.y);
}

uint
TkButton::widthReq() const
{
	return font()->getWidth(_text) + _data->padVert();
}

uint
TkButton::heightReq(uint) const
{
	return font()->getHeight() + _data->padHorz();
}

void
TkButton::render(Render&, PSurface&)
{
	render();
}

void
TkButton::render()
{
	_data->getButton(_state)->render(&_background, 0, 0,
					 _gm.width, _gm.height);
	font()->setColor(_data->getButtonColor());
	uint width_used;
	font()->draw(&_background,
		    _data->getPad(PAD_LEFT), _data->getPad(PAD_UP),
		    _text, width_used);

	X11::clearWindow(_window);
}

void
TkButton::resized()
{
	X11::resizeWindow(_window, _gm.width, _gm.height);
	_background.resize(_gm.width, _gm.height);
	X11::setWindowBackgroundPixmap(_window, _background.getDrawable());
}
