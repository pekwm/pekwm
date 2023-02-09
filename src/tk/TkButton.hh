#ifndef _PEKWM_TK_BUTTON_HH_
#define _PEKWM_TK_BUTTON_HH_

#include "TkWidget.hh"
#include "PPixmapSurface.hh"

#include <string>

class TkButton : public TkWidget {
public:
	TkButton(Theme::DialogData* data, PWinObj& parent,
	       stop_fun stop, int retcode, const std::string& text);
	virtual ~TkButton(void);

	virtual bool setState(Window window, ButtonState state) {
		if (window != _window) {
			return false;
		}
		_state = state;
		render();
		return true;
	}

	virtual bool click(Window window) {
		if (window != _window) {
			return false;
		}
		if (_state == BUTTON_STATE_HOVER
		    || _state == BUTTON_STATE_PRESSED) {
			_stop(_retcode);
		}
		return true;
	}

	virtual void place(int x, int y, uint, uint tot_height) {
		TkWidget::place(x, y, _gm.width, tot_height);
		X11::moveWindow(_window, _gm.x, _gm.y);
	}

	virtual uint widthReq(void) const {
		return _font->getWidth(_text) + _data->padVert();
	}

	virtual uint heightReq(uint) const {
		return _font->getHeight() + _data->padHorz();
	}

	virtual void render(Render&, PSurface&) {
		render();
	}

private:
	void render(void) {
		_data->getButton(_state)->render(&_background, 0, 0,
						 _gm.width, _gm.height);
		_font->setColor(_data->getButtonColor());
		_font->draw(&_background,
			    _data->getPad(PAD_LEFT), _data->getPad(PAD_UP),
			    _text);

		X11::clearWindow(_window);
	}

private:
	stop_fun _stop;
	int _retcode;
	std::string _text;
	PFont *_font;

	PPixmapSurface _background;
	ButtonState _state;
};

#endif // _PEKWM_TK_BUTTON_HH_
