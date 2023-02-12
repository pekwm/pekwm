//
// TkButtonRow.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "TkButtonRow.hh"

TkButtonRow::TkButtonRow(Theme::DialogData* data, PWinObj& parent,
			 stop_fun stop,
			 const std::vector<std::string>& options)
	: TkWidget(data, parent)
{
	int i = 0;
	std::vector<std::string>::const_iterator it = options.begin();
	for (; it != options.end(); ++it) {
		TkButton* button = new TkButton(_data, parent, stop, i++, *it);
		_buttons.push_back(button);
	}
}

TkButtonRow::~TkButtonRow(void)
{
	std::vector<TkButton*>::iterator it = _buttons.begin();
	for (; it != _buttons.end(); ++it) {
		delete *it;
	}
}

bool
TkButtonRow::setState(Window window, ButtonState state)
{
	std::vector<TkButton*>::iterator it = _buttons.begin();
	for (; it != _buttons.end(); ++it) {
		if ((*it)->setState(window, state)) {
			return true;
		}
	}
	return false;
}

bool
TkButtonRow::click(Window window)
{
	std::vector<TkButton*>::iterator it = _buttons.begin();
	for (; it != _buttons.end(); ++it) {
		if ((*it)->click(window)) {
			return true;
		}
	}
	return false;

}

void
TkButtonRow::place(int x, int y, uint width, uint tot_height)
{
	TkWidget::place(x, y, width, tot_height);

	if (_buttons.empty()) {
		return;
	}

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

uint
TkButtonRow::heightReq(uint width) const
{
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

void
TkButtonRow::render(Render &rend, PSurface &surface)
{
	std::vector<TkButton*>::iterator it = _buttons.begin();
	for (; it != _buttons.end(); ++it) {
		(*it)->render(rend, surface);
	}
}
