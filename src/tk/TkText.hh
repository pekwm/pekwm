//
// TkText.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_TK_TEXT_HH_
#define _PEKWM_TK_TEXT_HH_

#include "TkWidget.hh"

class TkText : public TkWidget {
public:
	TkText(Theme::DialogData* data, PWinObj& parent,
	       const std::string& text, bool is_title);
	virtual ~TkText();

	virtual void place(int x, int y, uint width, uint tot_height);
	virtual uint widthReq(void) const;
	virtual uint heightReq(uint width) const;

	virtual void render(Render &rend, PSurface &surface);

private:
	PFont* font() const
	{
		return _is_title ? _data->getTitleFont()
				 : _data->getTextFont();
	}
	PFont::Color* fontColor() const
	{
		return _is_title ? _data->getTitleColor()
				 : _data->getTextColor();
	}

	uint getLines(uint width, std::vector<std::string> &lines) const;

	std::string _text;
	std::vector<std::string> _lines;
	bool _is_title;
};

#endif // _PEKWM_TK_TEXT_HH_
