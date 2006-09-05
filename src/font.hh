//
// font.hh for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef _FONT_HH_
#define _FONT_HH_

#include "pekwm.hh"
#include "screeninfo.hh"
#include <string>

#ifdef XFT
#include <X11/Xft/Xft.h>
#endif // XFT

class PekFont
{
public:
	PekFont(ScreenInfo *s);
	~PekFont();

	unsigned int getWidth(const std::string &text, int max_chars = 0);
	unsigned int getHeight(const std::string &text);

	void draw(Drawable dest, int x, int y, const std::string &text,
						int max_width = 0, TextJustify justify = LEFT_JUSTIFY);

	bool load(const std::string &font_name);
	void unload(void);

	void setColor(const XColor &color);

private:
	ScreenInfo *scr;

#ifdef XFT
	XftDraw *m_xft_draw;
	XftFont *m_xft_font;
	XftColor *m_xft_color;

	XRenderColor m_xrender_color;
#else // !XFT
	XFontStruct *m_font;
	GC m_gc;
#endif // XFT
};

#endif // _FONT_HH_
