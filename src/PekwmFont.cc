//
// PekwmFont.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "PekwmFont.hh"

#include "ScreenInfo.hh"
#include "Util.hh"

#include <iostream>
#include <cstring>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

const char *FALLBACK_FONT = "fixed";

PekwmFont::PekwmFont(ScreenInfo *s) :
_scr(s),
_font(NULL), _fontset(NULL), _gc(None),
_height(0), _ascent(0), _descent(0)
{
}

PekwmFont::~PekwmFont()
{
	unload();
}

//! @fn    bool load(const string &font_name)
//! @breif Loads a font makes sure no old font data is left
//!
//! @param  font_name Font name specified in Xlfd format
//! @return true on success, else false
bool
PekwmFont::load(const string &font_name)
{
	unload();

	if (false) {
		char **missing_list, **font_names;
		char *def_string;
		int missing_count, fnum;
		XFontStruct **fonts;

		string basename(font_name);
		basename.append(",*");

		_fontset =
			XCreateFontSet(_scr->getDisplay(), basename.c_str(),
										 &missing_list, &missing_count, &def_string);
		if (!_fontset)
			return false;

		for (int i = 0; i < missing_count; ++i)
			cerr <<  "PekwmFont::load(): Missing charset" <<  missing_list[i] << endl;

		_ascent = _descent = 0;

		fnum = XFontsOfFontSet (_fontset, &fonts, &font_names);
		for (int i = 0; i < fnum; ++i) {
			if (signed(_ascent) < fonts[i]->ascent)
				_ascent = fonts[i]->ascent;
			if (signed(_descent) < fonts[i]->descent)
				_descent = fonts[i]->descent;
		}

		_height = _ascent + _descent;

	} else {
		_font = XLoadQueryFont(_scr->getDisplay(), font_name.c_str());
		if (!_font) {
			_font = XLoadQueryFont(_scr->getDisplay(), FALLBACK_FONT);
			if (!_font)
				return false;
		}

		_height = _font->ascent + _font->descent;
		_ascent = _font->ascent;
		_descent = _font->descent;
	}

	return true;
}

//! @fn    void unload(void)
//! @breif Unloads the currently loaded font if any
void
PekwmFont::unload(void)
{
	if (_fontset) {
		XFreeFontSet(_scr->getDisplay(), _fontset);
		_fontset = NULL;
	}
	if (_font) {
		XFreeFont(_scr->getDisplay(), _font);
		_font = NULL;
	}
	if (_gc != None) {
		XFreeGC(_scr->getDisplay(), _gc);
		_gc = None;
	}
}

//! @fn    void setColor(const XColor &color)
//! @breif Set the color the font is going to have when drawing
//!
//! @param color XColor object
//! @todo  Add support for specifying alpha when using Xft
void
PekwmFont::setColor(const XColor &color)
{
	if (_gc == None) {
		XGCValues gv;

		unsigned int mask = GCFunction|GCForeground;
		gv.function = GXcopy;
		gv.foreground = color.pixel;
		if (_font) {
			gv.font = _font->fid;
			mask |= GCFont;
		}

		_gc = XCreateGC(_scr->getDisplay(), _scr->getRoot(), mask, &gv);
	} else {
		XSetForeground(_scr->getDisplay(), _gc, color.pixel);
	}
}

//! @fn    unsigned int getWidth(const char *text, unsigned int max_chars)
//! @breif Calculates the width in pixels the string text would take if drawing it
//!
//! @param  text text to be measured
//! @param  max_chars maximum number of chars to use of the text, defaults to 0
//! @return the width in pixels if the string text would span if drawing it
unsigned int
PekwmFont::getWidth(const char *text, unsigned int max_chars)
{
	if (!text)
		return 0;

	// make sure the max_chars has a decent value, if it's less than 1 wich it
	// defaults to set it to the numbers of chars in the string text
	if (!max_chars || (max_chars > strlen(text)))
		max_chars = strlen(text);

	unsigned int width = 0;
	if (_fontset) {
		XRectangle ink, logical;
		XmbTextExtents(_fontset, text, max_chars, &ink, &logical);
		width = logical.width;
	} else if (_font)
		width = XTextWidth(_font, text, max_chars);

	return width;
}

//! @fn    unsigned int getWidth(const char *text)
//! @breif Calculates the height in pixels the string text would take if drawing it
//!
//! @param  text text to be measured
//! @return the height in pixels if the string text would span if drawing it
unsigned int
PekwmFont::getHeight(const char *text)
{
	return _height;
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
PekwmFont::draw(Drawable dest, int x, int y,
							const char *text, int max_width, TextJustify justify)
{
 	if (!text)
 		return;

 	unsigned int chars = strlen(text);
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

	if (_gc != None) {
		if (_fontset) {
			XmbDrawString(_scr->getDisplay(), dest, _fontset, _gc,
										offset, y + _ascent, text, chars);
		} else if (_font) {
			XDrawString(_scr->getDisplay(), dest, _gc, 
									offset, y + _ascent, text, chars);
		}
	}
}
