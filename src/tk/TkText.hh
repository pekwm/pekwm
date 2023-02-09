#ifndef _PEKWM_TK_TEXT_HH_
#define _PEKWM_TK_TEXT_HH_

#include "TkWidget.hh"

class TkText : public TkWidget {
public:
	TkText(Theme::DialogData* data, PWinObj& parent,
	     const std::string& text, bool is_title);
	virtual ~TkText(void);

	virtual void place(int x, int y, uint width, uint tot_height)
	{
		if (width != _gm.width) {
			_lines.clear();
		}
		TkWidget::place(x, y, width, tot_height);
	}

	virtual uint heightReq(uint width) const
	{
		std::vector<std::string> lines;
		uint num_lines = getLines(width, lines);
		return _font->getHeight() * num_lines + _data->padVert();
	}

	virtual void render(Render &rend, PSurface &surface)
	{
		if (_lines.empty()) {
			getLines(_gm.width, _lines);
		}

		_font->setColor(_is_title ? _data->getTitleColor()
					  : _data->getTextColor());

		uint y = _gm.y + _data->getPad(PAD_UP);
		std::vector<std::string>::iterator line = _lines.begin();
		for (; line != _lines.end(); ++line) {
			_font->draw(&surface,
				    _gm.x + _data->getPad(PAD_LEFT), y, *line);
			y += _font->getHeight();
		}
	}

private:
	uint getLines(uint width, std::vector<std::string> &lines) const;

private:
	PFont *_font;
	std::string _text;
	std::vector<std::string> _words;
	std::vector<std::string> _lines;
	bool _is_title;
};

#endif // _PEKWM_TK_TEXT_HH_
