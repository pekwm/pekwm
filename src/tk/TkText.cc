#include "TkText.hh"

TkText::TkText(Theme::DialogData* data, PWinObj& parent,
	   const std::string& text, bool is_title)
	: TkWidget(data, parent),
	  _font(is_title ? data->getTitleFont() : data->getTextFont()),
	  _text(text),
	  _is_title(is_title)
{
	Util::splitString(text, _words, " \t");
}

TkText::~TkText(void)
{
}

uint
TkText::getLines(uint width, std::vector<std::string> &lines) const
{
	width -= _data->getPad(PAD_LEFT) + _data->getPad(PAD_RIGHT);

	std::string line;
	std::vector<std::string>::const_iterator word = _words.begin();
	for (; word != _words.end(); ++word) {
		if (! line.empty()) {
			line += " ";
		}
		line += *word;

		uint l_width = _font->getWidth(line);
		if (l_width > width) {
			if (line == *word) {
				lines.push_back(line);
			} else {
				size_t len =
					line.size() - word->size() - 1;
				line.resize(len);
				lines.push_back(line);
				line = *word;
			}
		}
	}

	if (! line.empty()) {
		lines.push_back(line);
	}

	return lines.size();
}
