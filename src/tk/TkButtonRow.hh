#ifndef _PEKWM_TK_BUTTON_ROW_HH_
#define _PEKWM_TK_BUTTON_ROW_HH_

#include "TkWidget.hh"
#include "TkButton.hh"

#include <vector>

class TkButtonRow : public TkWidget
{
public:
	TkButtonRow(Theme::DialogData* data, PWinObj& parent,
		    stop_fun stop, std::vector<std::string> options);
	virtual ~TkButtonRow(void);

	virtual bool setState(Window window, ButtonState state) {
		std::vector<TkButton*>::iterator it = _buttons.begin();
		for (; it != _buttons.end(); ++it) {
			if ((*it)->setState(window, state)) {
				return true;
			}
		}
		return false;
	}

	virtual bool click(Window window) {
		std::vector<TkButton*>::iterator it = _buttons.begin();
		for (; it != _buttons.end(); ++it) {
			if ((*it)->click(window)) {
				return true;
			}
		}
		return false;

	}

	virtual void place(int x, int y, uint width, uint tot_height)
	{
		TkWidget::place(x, y, width, tot_height);

		// place buttons centered on available width
		uint buttons_width = 0;
		std::vector<TkButton*>::iterator it = _buttons.begin();
		for (; it != _buttons.end(); ++it) {
			buttons_width += (*it)->widthReq();
		}
		buttons_width += _buttons.size() * _data->padHorz();

		x = (width - buttons_width) / 2;
		if (tot_height) {
			y = tot_height - _data->getPad(PAD_DOWN)
				- _buttons[0]->heightReq(width);
		} else {
			y += _data->getPad(PAD_UP);
		}
		it = _buttons.begin();
		for (; it != _buttons.end(); ++it) {
			(*it)->place(x, y, width, tot_height);
			x += (*it)->widthReq() + _data->padHorz();
		}
	}

	virtual uint heightReq(uint width) const {
		uint height = 0;
		std::vector<TkButton*>::const_iterator it = _buttons.begin();
		for (; it != _buttons.end(); ++it) {
			uint height_req = (*it)->heightReq(width);
			if (height_req > height) {
				height = height_req;
			}
		}
		return height + _data->padVert();
	}

	virtual void render(Render &rend, PSurface &surface) {
		std::vector<TkButton*>::iterator it = _buttons.begin();
		for (; it != _buttons.end(); ++it) {
			(*it)->render(rend, surface);
		}
	}

private:
	std::vector<TkButton*> _buttons;
};

#endif // _PEKWM_TK_BUTTON_ROW_HH_
