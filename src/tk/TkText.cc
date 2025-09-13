//
// TkText.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Tokenizer.hh"
#include "TkText.hh"

TkText::TkText(Theme::DialogData* data, PWinObj& parent,
	   const std::string& text, bool is_title)
	: TkWidget(data, parent),
	  _text(text),
	  _is_title(is_title)
{
}

TkText::~TkText()
{
}

void
TkText::place(int x, int y, uint width, uint tot_height)
{
	if (width != _gm.width) {
		_lines.clear();
	}
	TkWidget::place(x, y, width, tot_height);
}

uint
TkText::widthReq(void) const
{
	// ordinary text will adapt to the available space, title request
	// to have full title width available.
	if (! _is_title) {
		return 0;
	}

	return font()->getWidth(_text) + _data->padHorz();
}

uint
TkText::heightReq(uint width) const
{
	std::vector<std::string> lines;
	uint num_lines = getLines(width, lines);
	uint height = font()->getHeight() * num_lines + _data->padVert();
	if (_is_title) {
		height += _data->padVert();
	}
	return height;
}

void
TkText::render(Render &rend, PSurface &surface)
{
	if (_lines.empty()) {
		getLines(_gm.width, _lines);
	}

	font()->setColor(fontColor());

	uint y = _gm.y;
	if (_is_title) {
		y += _data->padVert();
	} else {
		y += _data->getPad(PAD_UP);
	}
	std::vector<std::string>::iterator line = _lines.begin();
	for (; line != _lines.end(); ++line) {
		uint width_used;
		font()->draw(&surface,
			     _gm.x + _data->getPad(PAD_LEFT), y, *line,
			     width_used);
		y += font()->getHeight();
	}
}

uint
TkText::getLines(uint width, std::vector<std::string> &lines) const
{
	width -= _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT);

	std::string line;
	Tokenizer tok(_text);
	while (tok.next()) {
		if (tok.isBreak()) {
			lines.push_back(line);
			line = "";
			continue;
		}

		line += *tok;
		uint l_width = font()->getWidth(line);
		if (l_width > width) {
			if (line == *tok) {
				// single token, full line, keep as is.
				lines.push_back(line);
			} else {
				// drop current token from line and
				// start a new line with the token
				size_t len = line.size() - (*tok).size();
				line.resize(len);
				lines.push_back(line);
				line = *tok;
			}
		}
	}

	if (! line.empty()) {
		lines.push_back(line);
	}

	return lines.size();
}
