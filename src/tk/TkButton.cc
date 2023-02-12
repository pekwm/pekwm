#include "TkButton.hh"

TkButton::TkButton(Theme::DialogData* data, PWinObj& parent,
		   stop_fun stop, int retcode, const std::string& text)
	: TkWidget(data, parent),
	  _stop(stop),
	  _retcode(retcode),
	  _text(text),
	  _font(data->getButtonFont()),
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
