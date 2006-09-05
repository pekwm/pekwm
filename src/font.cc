//
// font.cc for pekwm
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

#include "font.hh"
#include "util.hh"

#include <cstring>

using std::string;
using std::vector;

const char *FALLBACK_FONT = "fixed";

PekFont::PekFont(ScreenInfo *s) :
scr(s),
#ifdef XFT
m_xft_draw(NULL), m_xft_font(NULL), m_xft_color(NULL)
#else // !XFT
m_font(NULL), m_gc(None)
#endif // XFT
{
#ifdef XFT
	m_xft_draw = XftDrawCreate(scr->getDisplay(), scr->getRoot(),
														 scr->getVisual(), scr->getColormap());
#endif // XFT
}

PekFont::~PekFont()
{
	unload();
#ifdef XFT
	if (m_xft_draw)
		XftDrawDestroy(m_xft_draw);
	if (m_xft_color)
		XftColorFree(scr->getDisplay(), scr->getVisual(), scr->getColormap(),
								 m_xft_color);
#endif // XFT
}

//! @fn    bool load(const string &font_name)
//! @breif Loads a font makes sure no old font data is left
//!
//! @param  font_name Font name specified in Xlfd format
//! @return true on success, else false
bool
PekFont::load(const string &font_name)
{
#ifdef XFT
	if (m_xft_font)
		unload();

	vector<string> font_info;
	if ((Util::splitString(font_name, font_info, "#", 2)) == 2) {
		if (!strcasecmp(font_info[1].c_str(), "xft")) {
			m_xft_font = XftFontOpenName(scr->getDisplay(), scr->getScreenNum(),
																	 font_info[0].c_str());
		} else {
			m_xft_font = XftFontOpenXlfd(scr->getDisplay(), scr->getScreenNum(),
																	 font_name.c_str());
		}
	} else {
		m_xft_font = XftFontOpenXlfd(scr->getDisplay(), scr->getScreenNum(),
																 font_name.c_str());
	}

	if (m_xft_font)
		return true;
	return false;
#else // !XFT
	if (m_font)
		unload();

	m_font = XLoadQueryFont(scr->getDisplay(), font_name.c_str());

	if (!m_font) {
		m_font = XLoadQueryFont(scr->getDisplay(), FALLBACK_FONT);
		return false;
	}
	return true;
#endif // !XFT
}

//! @fn    void unload(void)
//! @breif Unloads the currently loaded font if any
void
PekFont::unload(void)
{
#ifdef XFT
	if (m_xft_font) {
		XftFontClose(scr->getDisplay(), m_xft_font);
		m_xft_font = NULL;
	}
#else // !XFT
	if (m_font) {
		XFreeFont(scr->getDisplay(), m_font);
		m_font = NULL;
	}
	if (m_gc != None) {
		XFreeGC(scr->getDisplay(), m_gc);
		m_gc = None;
	}
#endif // XFT
}

//! @fn    void setColor(const XColor &color)
//! @breif Set the color the font is going to have when drawing
//!
//! @param color XColor object
//! @todo  Add support for specifying alpha when using Xft
void
PekFont::setColor(const XColor &color)
{
#ifdef XFT
	if (m_xft_font) {
		m_xrender_color.red = color.red;
		m_xrender_color.green = color.green;
		m_xrender_color.blue = color.blue;
		m_xrender_color.alpha = 65535;

		if (m_xft_color) {
			XftColorFree(scr->getDisplay(), scr->getVisual(), scr->getColormap(),
									 m_xft_color);
		}

		m_xft_color = new XftColor;

		XftColorAllocValue(scr->getDisplay(), scr->getVisual(), scr->getColormap(),
											 &m_xrender_color, m_xft_color);
	}
#else // !XFT
	if (m_font) {
		if (m_gc == None) {
			XGCValues gv;

			gv.function = GXcopy;
			gv.foreground = color.pixel;
			gv.font = m_font->fid;
			m_gc = XCreateGC(scr->getDisplay(), scr->getRoot(),
											 GCFunction|GCForeground|GCFont, &gv);
		} else {
			XSetForeground(scr->getDisplay(), m_gc, color.pixel);
		}
	}
#endif // XFT
}

//! @fn    unsigned int getWidth(const string &text, int max_chars)
//! @breif Calculates the width in pixels the string text would take if drawing it
//!
//! @param  text text to be measured
//! @param  max_chars maximum number of chars to use of the text, defaults to 0
//! @return the width in pixels if the string text would span if drawing it
unsigned int
PekFont::getWidth(const string &text, int max_chars)
{
	if (!text.size())
		return 0;

	unsigned int width = 0;

	// make sure the max_chars has a decent value, if it's less than 1 wich it
	// defaults to set it to the numbers of chars in the string text
	if ((max_chars > (signed) text.size()) || (max_chars < 1))
		max_chars = text.size();

#ifdef XFT
	if (m_xft_font) {
		XGlyphInfo extents;
		XftTextExtents8(scr->getDisplay(), m_xft_font,
										(XftChar8 *) text.c_str(), max_chars, &extents);

		width = extents.width;
	}
#else // !XFT
	if (m_font)
		width = XTextWidth(m_font, text.c_str(), max_chars);
#endif // XFT
	return width;
}

//! @fn    unsigned int getWidth(const string &text)
//! @breif Calculates the height in pixels the string text would take if drawing it
//!
//! @param  text text to be measured
//! @return the height in pixels if the string text would span if drawing it
unsigned int
PekFont::getHeight(const string &text)
{
	unsigned int height = 0;

#ifdef XFT
	if (m_xft_font) {
		XGlyphInfo extents;
		XftTextExtents8(scr->getDisplay(), m_xft_font,
										(XftChar8 *) text.c_str(), text.size(), &extents);
		height = extents.height;
	}
#else // !XFT
	if (m_font)
		height = m_font->ascent + m_font->descent;
#endif // XFT

	return height;
}

//! @fn    void draw(Drawable dest, int x, int y, const string &text, int max_width, TextJustify justify)
//! @breif Draws the string text on the Drawable draw
//!
//! @param dest Specifies the drawable to draw the text on
//! @param x Offset
//! @param y Offset
//! @param text String to draw
//! @param max_width Max width the text is allowed to occypy, defaults to 0
//! @param justify Text justify when max_width > 0
void
PekFont::draw(Drawable dest, int x, int y,
							const string &text, int max_width, TextJustify justify)
{
	if (!text.size())
		return;

	unsigned int chars = text.size();
	unsigned int offset = x;
	unsigned int width = getWidth(text);

	// if we have max width set, make sure our text isn't longer
	if (max_width != 0) {
		for (unsigned int i = chars; i > 0; --i) {
			width = getWidth(text, i);
			if ((signed) width < max_width) {
				chars = i;
				break;
			}
		}

		switch(justify) {
		case CENTER_JUSTIFY:
			offset += (max_width - width) / 2;
			break;
		case RIGHT_JUSTIFY:
			offset += max_width - width;
			break;
		default:
			// Do nothing
			break;
		}
	}

#ifdef XFT
	if (m_xft_font && m_xft_color) {
		XftDrawChange(m_xft_draw, dest);

		XGlyphInfo extents;
		XftTextExtents8(scr->getDisplay(), m_xft_font,
										(XftChar8 *) text.c_str(), chars, &extents);

		XftDrawString8(m_xft_draw, m_xft_color, m_xft_font,
									 offset + extents.x, y + extents.y,
									 (XftChar8 *) text.c_str(), chars);
	}
#else // !XFT
	if (m_font && (m_gc != None)) {
		XDrawString(scr->getDisplay(), dest, m_gc,
								offset, y + m_font->ascent, text.c_str(), chars);
	}
#endif // XFT
}
