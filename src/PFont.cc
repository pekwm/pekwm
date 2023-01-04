//
// PFont.cc for pekwm
// Copyright (C) 2003-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>
#include <cstring>

#include "Charset.hh"
#include "Debug.hh"
#include "PFont.hh"
#include "Util.hh"
#include "X11.hh"

std::string PFont::_trim_string = std::string();
const char *FALLBACK_FONT = "fixed";

// PFont::Color

PFont::Color::Color(void)
	: _fg(nullptr),
	  _bg(nullptr),
	  _fg_alpha(65535),
	  _bg_alpha(65535)
{
}

PFont::Color::~Color(void)
{
	if (_fg) {
		X11::returnColor(_fg);
	}
	if (_bg) {
		X11::returnColor(_bg);
	}
}

// PFont

//! @brief PFont constructor
PFont::PFont(void) :
	_height(0), _ascent(0), _descent(0),
	_offset_x(0), _offset_y(0), _justify(FONT_JUSTIFY_LEFT)
{
}

//! @brief PFont destructor
PFont::~PFont(void)
{
	PFont::unload();
}

/**
 * Draws the text on the drawable.
 *
 * @param dest destination Drawable
 * @param x x start position
 * @param y y start position
 * @param text text to draw
 * @param max_chars max nr of chars, defaults to 0 == strlen(text)
 * @param max_width max nr of pixels, defaults to 0 == infinity
 * @param trim_type how to trim title if not enough space, defaults
 *        to FONT_TRIM_END
 */
int
PFont::draw(Drawable dest, int x, int y, const std::string &text,
	    uint max_chars, uint max_width, PFont::TrimType trim_type)
{
	if (! text.size()) {
		return 0;
	}

	uint offset = x, chars = max_chars;
	std::string real_text(text);

	if (max_width > 0) {
		// If max width set, make sure text fits in max_width pixels.
		trim(real_text, trim_type, max_width);

		offset += justify(real_text, max_width, 0,
				  (chars == 0) ? real_text.size() : chars);
	} else if (chars == 0) {
		// Just set to complete string.
		chars = real_text.size();
	}

	// Draw shadowed font if x or y offset is specified
	if (_offset_x || _offset_y) {
		drawText(dest, offset + _offset_x, y + _ascent + _offset_y,
			 real_text, chars, false /* bg */);
	}

	// Draw main font
	drawText(dest, offset, y + _ascent, real_text, chars, true /* fg */);

	return offset;
}

/**
 * Trims the text making it max max_width wide
 */
void
PFont::trim(std::string &text, PFont::TrimType trim_type, uint max_width)
{
	if (! text.size()) {
		return;
	}

	if (getWidth(text) > max_width) {
		if (_trim_string.size() > 0
		    && trim_type == FONT_TRIM_MIDDLE
		    && trimMiddle(text, max_width)) {
			return;
		}

		trimEnd(text, max_width);
	}
}

/**
 * Figures how many charachters to have before exceding max_width
 */
void
PFont::trimEnd(std::string &text, uint max_width)
{
	Charset::Utf8Iterator it(text, text.size());
	--it;
	--it;
	for (; ! it.begin(); --it) {
		if (getWidth(text, it.pos()) <= max_width) {
			text.resize(it.pos());
			return;
		}
	}
	text = "";
}

/**
 * Replace middle of string with _trim_string making it max_width wide
 */
bool
PFont::trimMiddle(std::string &text, uint max_width)
{
	bool trimmed = false;

	// Get max and separator width
	uint max_side = (max_width / 2);
	uint sep_width = getWidth(_trim_string);

	uint pos = 0;
	std::string dest;

	// If the trim string is too large, do nothing and let trimEnd handle
	// this.
	if (sep_width > max_width) {
		return false;
	}
	// Add space for the trim string
	max_side -= sep_width / 2;

	// Get numbers of chars before trim string (..)
	Charset::Utf8Iterator it(text, text.size());
	for (--it; ! it.begin(); --it) {
		if (getWidth(text, it.pos()) <= max_side) {
			pos = it.pos();
			dest.insert(0, text.substr(0, it.pos()));
			break;
		}
	}

	// get numbers of chars after ...
	if (pos < text.size()) {
		for (++it; ! it.end(); ++it) {
			std::string second_part(text.substr(it.pos(),
						text.size() - it.pos()));
			if (getWidth(second_part, 0) <= max_side) {
				dest.insert(dest.size(), second_part);
				break;
			}
		}

		// Got a char after and before, if not do nothing and trimEnd
		// will handle trimming after this call.
		if (dest.size() > 1) {
			if ((getWidth(dest) + getWidth(_trim_string))
			    <= max_width) {
				dest.insert(pos, _trim_string);
				trimmed = true;
			}

			// Update original string
			text = dest;
		}
	}

	return trimmed;
}

void
PFont::setTrimString(const std::string &text) {
	_trim_string = text;
}

//! @brief Justifies the string based on _justify property of the Font
uint
PFont::justify(const std::string &text, uint max_width,
	       uint padding, uint chars)
{
	uint x;
	uint width = getWidth(text, chars);

	switch(_justify) {
	case FONT_JUSTIFY_CENTER:
		x = (max_width - width) / 2;
		break;
	case FONT_JUSTIFY_RIGHT:
		x = max_width - width - padding;
		break;
	default:
		x = padding;
		break;
	}

	return x;
}

// PFontX11

//! @brief PFontX11 constructor
PFontX11::PFontX11(void)
	: PFont(),
	  _font(0), _gc_fg(None), _gc_bg(None)
{
}

//! @brief PFontX11 destructor
PFontX11::~PFontX11(void)
{
	PFontX11::unload();
}

//! @brief Loads the X11 font font_name
bool
PFontX11::load(const std::string &font_name)
{
	unload();

	_font = XLoadQueryFont(X11::getDpy(), font_name.c_str());
	if (! _font) {
		_font = XLoadQueryFont(X11::getDpy(), FALLBACK_FONT);
		if (! _font) {
			return false;
		}
	}

	_height = _font->ascent + _font->descent;
	_ascent = _font->ascent;
	_descent = _font->descent;

	// create GC's
	XGCValues gv;

	uint mask = GCFunction|GCFont;
	gv.function = GXcopy;
	gv.font = _font->fid;
	_gc_fg = XCreateGC(X11::getDpy(), X11::getRoot(), mask, &gv);
	_gc_bg = XCreateGC(X11::getDpy(), X11::getRoot(), mask, &gv);

	return true;
}

//! @brief Unloads font resources
void
PFontX11::unload(void)
{
	if (_font) {
		XFreeFont(X11::getDpy(), _font);
		_font = 0;
	}
	if (_gc_fg != None) {
		XFreeGC(X11::getDpy(), _gc_fg);
		_gc_fg = None;
	}
	if (_gc_bg != None) {
		XFreeGC(X11::getDpy(), _gc_bg);
		_gc_bg = None;
	}
}

//! @brief Gets the width the text would take using this font
uint
PFontX11::getWidth(const std::string &text, uint max_chars)
{
	if (! text.size()) {
		return 0;
	}

	// Make sure the max_chars has a decent value, if it's less than 1 wich
	// it defaults to set it to the numbers of chars in the string text
	if (! max_chars || (max_chars > text.size())) {
		max_chars = text.size();
	}

	uint width = 0;
	if (_font) {
		// No UTF8 support, convert to locale encoding.
		std::string mb_text =
			Charset::toSystem(text.substr(0, max_chars));
		width = XTextWidth(_font, mb_text.c_str(), mb_text.size());
	}

	return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontX11::drawText(Drawable dest, int x, int y,
		   const std::string &text, uint chars, bool fg)
{
	GC gc = fg ? _gc_fg : _gc_bg;

	if (_font && (gc != None)) {
		std::string mb_text;
		if (chars != 0) {
			mb_text = Charset::toSystem(text.substr(0, chars));
		} else {
			mb_text = Charset::toSystem(text);
		}

		XDrawString(X11::getDpy(), dest, gc, x, y,
			    mb_text.c_str(), mb_text.size());
	}
}

//! @brief Sets the color that should be used when drawing
void
PFontX11::setColor(PFont::Color *color)
{
	if (color->hasFg()) {
		XSetForeground(X11::getDpy(), _gc_fg, color->getFg()->pixel);
	}

	if (color->hasBg()) {
		XSetForeground(X11::getDpy(), _gc_bg, color->getBg()->pixel);
	}
}

// PFontXmb

const char* PFontXmb::DEFAULT_FONTSET = "fixed*";

//! @brief PFontXmb constructor
PFontXmb::PFontXmb(void)
	: PFont(),
	  _fontset(0), _gc_fg(None), _gc_bg(None)
{
}

//! @brief PFontXmb destructor
PFontXmb::~PFontXmb(void)
{
	PFontXmb::unload();
}

//! @brief Loads Xmb font font_name
bool
PFontXmb::load(const std::string &font_name)
{
	unload();

	std::string basename(font_name);
	size_t pos = font_name.rfind(",*");
	if (pos == std::string::npos || pos != (font_name.size() - 2)) {
		basename.append(",*");
	}

	// Create GC
	XGCValues gv;

	uint mask = GCFunction;
	gv.function = GXcopy;
	_gc_fg = XCreateGC(X11::getDpy(), X11::getRoot(), mask, &gv);
	_gc_bg = XCreateGC(X11::getDpy(), X11::getRoot(), mask, &gv);

	// Try to load font
	char *def_string;
	char **missing_list, **font_names;
	int missing_count;

	_fontset = XCreateFontSet(X11::getDpy(), basename.c_str(),
				  &missing_list, &missing_count, &def_string);
	if (! _fontset) {
		USER_WARN("failed to create fontset for " << font_name);
		_fontset = XCreateFontSet(X11::getDpy(), DEFAULT_FONTSET,
					  &missing_list, &missing_count,
					  &def_string);
	}

	if (_fontset) {
		for (int i = 0; i < missing_count; ++i) {
			USER_WARN(font_name << " missing charset "
				  <<  missing_list[i]);
		}
		XFreeStringList(missing_list);

		_ascent = _descent = 0;

		XFontStruct **fonts;
		int fnum = XFontsOfFontSet (_fontset, &fonts, &font_names);
		for (int i = 0; i < fnum; ++i) {
			if (signed(_ascent) < fonts[i]->ascent) {
				_ascent = fonts[i]->ascent;
			}
			if (signed(_descent) < fonts[i]->descent) {
				_descent = fonts[i]->descent;
			}
		}

		_height = _ascent + _descent;

	} else {
		return false;
	}

	return true;
}

//! @brief Unloads font resources
void
PFontXmb::unload(void)
{
	if (_fontset) {
		XFreeFontSet(X11::getDpy(), _fontset);
		_fontset = 0;
	}

	if (_gc_fg != None) {
		XFreeGC(X11::getDpy(), _gc_fg);
		_gc_fg = None;
	}
	if (_gc_bg != None) {
		XFreeGC(X11::getDpy(), _gc_bg);
		_gc_bg = None;
	}
}

//! @brief Gets the width the text would take using this font
uint
PFontXmb::getWidth(const std::string &text, uint max_chars)
{
	if (! text.size()) {
		return 0;
	}

	// Make sure the max_chars has a decent value, if it's less than 1 wich
	// it defaults to set it to the numbers of chars in the string text
	if (! max_chars || (max_chars > text.size())) {
		max_chars = text.size();
	}

	uint width = 0;
	if (_fontset) {
		XRectangle ink, logical;
		XmbTextExtents(_fontset, text.c_str(), max_chars,
			       &ink, &logical);
		width = logical.width;
	}

	return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontXmb::drawText(Drawable dest, int x, int y,
		   const std::string &text, uint chars, bool fg)
{
	GC gc = fg ? _gc_fg : _gc_bg;

	if (_fontset && (gc != None)) {
		XmbDrawString(X11::getDpy(), dest, _fontset, gc,
			      x, y, text.c_str(), chars ? chars : text.size());
	}
}

//! @brief Sets the color that should be used when drawing
void
PFontXmb::setColor(PFont::Color *color)
{
	if (color->hasFg()) {
		XSetForeground(X11::getDpy(), _gc_fg, color->getFg()->pixel);
	}

	if (color->hasBg()) {
		XSetForeground(X11::getDpy(), _gc_bg, color->getBg()->pixel);
	}
}

// PFontXft

#ifdef PEKWM_HAVE_XFT

//! @brief PFontXft constructor
PFontXft::PFontXft(void)
	: PFont(),
	  _draw(0), _font(0), _cl_fg(0), _cl_bg(0)
{
	_draw = XftDrawCreate(X11::getDpy(), X11::getRoot(),
			      X11::getVisual(), X11::getColormap());
}

//! @brief PFontXft destructor
PFontXft::~PFontXft(void)
{
	PFontXft::unload();

	XftDrawDestroy(_draw);

	if (_cl_fg) {
		XftColorFree(X11::getDpy(), X11::getVisual(),
			     X11::getColormap(), _cl_fg);
		delete _cl_fg;
	}

	if (_cl_bg) {
		XftColorFree(X11::getDpy(), X11::getVisual(),
			     X11::getColormap(), _cl_bg);
		delete _cl_bg;
	}
}

//! @brief Loads the Xft font font_name
bool
PFontXft::load(const std::string &font_name)
{
	unload();

	_font = XftFontOpenName(X11::getDpy(), X11::getScreenNum(),
				font_name.c_str());
	if (! _font) {
		_font = XftFontOpenXlfd(X11::getDpy(), X11::getScreenNum(),
					font_name.c_str());
	}

	if (_font) {
		_ascent = _font->ascent;
		_descent = _font->descent;
		_height = _font->height;

		return true;
	}
	return false;
}

//! @brief Unloads font resources
void
PFontXft::unload(void)
{
	if (_font) {
		XftFontClose(X11::getDpy(), _font);
		_font = 0;
	}
}

//! @brief Gets the width the text would take using this font
uint
PFontXft::getWidth(const std::string &text, uint max_chars)
{
	if (! text.size()) {
		return 0;
	}

	// Make sure the max_chars has a decent value, if it's less than 1 wich
	// it defaults to set it to the numbers of chars in the string text
	if (! max_chars || (max_chars > text.size())) {
		max_chars = text.size();
	}

	uint width = 0;
	if (_font) {
		std::string sub_text = text.substr(0, max_chars);

		XGlyphInfo extents;
		XftTextExtentsUtf8(X11::getDpy(), _font,
				   reinterpret_cast<const XftChar8*>(
					   sub_text.c_str()),
				   sub_text.size(), &extents);

		width = extents.xOff;
	}

	return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontXft::drawText(Drawable dest, int x, int y,
		   const std::string &text, uint chars, bool fg)
{
	XftColor *cl = fg ? _cl_fg : _cl_bg;

	if (_font && cl) {
		std::string sub_text;
		if (chars != 0) {
			sub_text = text.substr(0, chars);
		} else {
			sub_text = text;
		}

		XftDrawChange(_draw, dest);
		XftDrawStringUtf8(_draw, cl, _font, x, y,
				  reinterpret_cast<const XftChar8*>(
					  sub_text.c_str()),
				  sub_text.size());
	}
}

//! @brief Sets the color that should be used when drawing
void
PFontXft::setColor(PFont::Color *color)
{
	if (_cl_fg) {
		XftColorFree(X11::getDpy(), X11::getVisual(),
			     X11::getColormap(), _cl_fg);
		delete _cl_fg;
		_cl_fg = 0;
	}
	if (_cl_bg) {
		XftColorFree(X11::getDpy(), X11::getVisual(),
			     X11::getColormap(), _cl_bg);
		delete _cl_bg;
		_cl_bg = 0;
	}

	if (color->hasFg()) {
		_xrender_color.red = color->getFg()->red;
		_xrender_color.green = color->getFg()->green;
		_xrender_color.blue = color->getFg()->blue;
		_xrender_color.alpha = color->getFgAlpha();

		_cl_fg = new XftColor;

		XftColorAllocValue(X11::getDpy(), X11::getVisual(),
				   X11::getColormap(), &_xrender_color, _cl_fg);
	}

	if (color->hasBg()) {
		_xrender_color.red = color->getBg()->red;
		_xrender_color.green = color->getBg()->green;
		_xrender_color.blue = color->getBg()->blue;
		_xrender_color.alpha = color->getBgAlpha();

		_cl_bg = new XftColor;

		XftColorAllocValue(X11::getDpy(), X11::getVisual(),
				   X11::getColormap(), &_xrender_color, _cl_bg);
	}
}

#endif // PEKWM_HAVE_XFT
