//
// PekwmFont.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _PEKWM_FONT_HH_
#define _PEKWM_FONT_HH_

#include "pekwm.hh"

class ScreenInfo;

#include <string>

class PekwmFont
{
public:
	PekwmFont(ScreenInfo *s);
	~PekwmFont();

	unsigned int getWidth(const char *text, unsigned int max_chars = 0);
	unsigned int getHeight(const char *text);

	void draw(Drawable dest, int x, int y, const char *text,
						int max_width = 0, TextJustify justify = LEFT_JUSTIFY);

	bool load(const std::string &font_name);
	void unload(void);

	void setColor(const XColor &color);

private:
	ScreenInfo *_scr;

	XFontStruct *_font;
	XFontSet _fontset;
	GC _gc;

	unsigned int _height, _ascent, _descent;
};

#endif // _PEKWM_FONT_HH_
