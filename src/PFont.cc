//
// PFont.cc for pekwm
// Copyright (C) 2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "PFont.hh"

#include "PScreen.hh"
#include "Util.hh"

#include <iostream>
#include <cstring>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

string PFont::_trim_string = string();
const char *FALLBACK_FONT = "fixed";

// PFont

//! @brief PFont constructor
PFont::PFont(PScreen *scr) :
_scr(scr),
_height(0), _ascent(0), _descent(0),
_offset_x(0), _offset_y(0), _justify(FONT_JUSTIFY_LEFT)
{
	_type = FONT_TYPE_NO;
}

//! @brief PFont destructor
PFont::~PFont(void)
{
}

//! @brief Draws the text on the drawable.
//! @param dest destination Drawable
//! @param x x start position
//! @param y y start position
//! @param text text to draw
//! @param max_chars max nr of chars, defaults to 0 == strlen(text)
//! @param max_width max nr of pixels, defaults to 0 == infinity
//! @param trim_type how to trim title if not enough space, defaults to FONT_TRIM_END
void
PFont::draw(Drawable dest, int x, int y, const char *text,
						uint max_chars, uint max_width, PFont::TrimType trim_type)
{
	if (text == NULL) {
		return;
	}

	uint offset = x, chars = max_chars;
	bool free_text = false;
	char *real_text = (char*) text;

	// if we have max width set, make sure our text isn't longer
	if (max_width > 0) {
		free_text = trim(&real_text, trim_type, max_width, chars);
		offset += justify(real_text, max_width, 0,
										  (chars == 0) ? strlen(real_text) : chars);
	// chars will be set in trim if calling that
	} else if (chars == 0) {
		chars = strlen(real_text);
	}

	// shadowed font
	if ((_offset_x != 0) || (_offset_y != 0)) {
		drawText(dest, offset + _offset_x, y + _ascent + _offset_y,
						 real_text, chars, false); // false as in bg
	}

	// fg text
	drawText(dest, offset, y + _ascent, real_text, chars, true); // true as in fg

	if (free_text) {
		free(real_text);
	}
}

//! @brief Trims the text making it max max_width wide
//! @return true if *text needs to be free, else false
bool
PFont::trim(char **text, PFont::TrimType trim_type, uint max_width, uint &chars)
{
	bool free_text = false;

	if (getWidth(*text) > max_width) {
		if ((_trim_string.size() > 0) && (trim_type == FONT_TRIM_MIDDLE)) {
			*text = trimMiddle(*text, max_width);
			chars = trimEnd(*text, max_width);

			free_text = true;
		} else {
			chars = trimEnd(*text, max_width);
		}
	} else {
		chars = strlen(*text);
	}

	return free_text;
}

//! @brief Figures how many charachters to have before exceding max_width
uint
PFont::trimEnd(const char *text, uint max_width)
{
	for (uint i = strlen(text); i > 0; --i) {
		if (getWidth(text, i) <= max_width) {
			return i;
		}
	}

	return 0;
}

//! @brief Replace middle of string with _trim_string making it max_width wide
char*
PFont::trimMiddle(const char *text, uint max_width)
{
	if (_trim_string.size() == 0) {
		return strdup(text);
	}

	string dest;
	uint chars = strlen(text), pos = 0;

	// max width
	uint max_side = (max_width / 2);
	uint sep_width = getWidth(_trim_string.c_str(),
														_trim_string.size() / 2 + _trim_string.size() % 2);

	// if we can't hold the trim string, just do an trim end
	if (max_side <= sep_width) {
		return strdup(text);
	}
	// add space for the trim string
	max_side -= sep_width;

	// get numbers of chars before ...
	for (uint i = chars; i > 0; --i) {
		if (getWidth(text, i) < max_side) {
			pos = i;
			dest.insert(0, text, i);
			break;
		}
	}
	// get numbers of chars after ...
	for (uint i = pos; i < chars; ++i) {
		if (getWidth(text + i, chars - i) < max_side) {
			dest.insert(dest.size(), text + i, chars - i);
			break;
		}
	}

	// got a char after and before
	if (dest.size() > 1) {
		if ((getWidth(dest.c_str()) + getWidth(_trim_string.c_str())) < max_width) {
			dest.insert(pos, _trim_string);
		}
	// not enough room to place a characther in max_side, do trim end instead
	} else {
		return strdup(text);
	}

#ifdef DEBUG
	if (getWidth(dest.c_str()) > max_width) {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "width(" << getWidth(dest.c_str()) << ") is greater than max_width("
				 << max_width << ")" << endl;
	}
#endif // DEBUG

	return strdup(dest.c_str());
}

//! @brief Justifies the string based on _justify property of the Font
uint
PFont::justify(const char *text, uint max_width, uint padding, uint chars)
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
PFontX11::PFontX11(PScreen *scr) : PFont(scr),
_font(NULL),
_gc_fg(None), _gc_bg(None)
{
	_type = FONT_TYPE_X11;
}

//! @brief PFontX11 destructor
PFontX11::~PFontX11(void)
{
	unload();
}

//! @brief Loads the X11 font font_name
bool
PFontX11::load(const std::string &font_name)
{
	unload();

	_font = XLoadQueryFont(_scr->getDpy(), font_name.c_str());
	if (_font == NULL) {
		_font = XLoadQueryFont(_scr->getDpy(), FALLBACK_FONT);
		if (_font == NULL) {
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
	_gc_fg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);
	_gc_bg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);

	return true;
}

//! @brief Unloads font resources
void
PFontX11::unload(void)
{

	if (_font) {
		XFreeFont(_scr->getDpy(), _font);
		_font = NULL;
	}
	if (_gc_fg != None) {
		XFreeGC(_scr->getDpy(), _gc_fg);
		_gc_fg = None;
	}
	if (_gc_bg != None) {
		XFreeGC(_scr->getDpy(), _gc_bg);
		_gc_bg = None;
	}
}

//! @brief Gets the width the text would take using this font
uint
PFontX11::getWidth(const char *text, uint max_chars)
{
	if (text == NULL)
		return 0;

	// make sure the max_chars has a decent value, if it's less than 1 wich it
	// defaults to set it to the numbers of chars in the string text
	if ((max_chars == 0) || (max_chars > strlen(text)))
		max_chars = strlen(text);

	uint width = 0;
	if (_font != NULL) {
		width = XTextWidth(_font, text, max_chars);
	}

	return (width + _offset_x);
}

//! @brief Sets the color that should be used when drawing
void
PFontX11::setColor(PFont::Color *color)
{
	if (color->hasFg()) {
		XSetForeground(_scr->getDpy(), _gc_fg, color->getFg()->pixel);
	}

	if (color->hasBg()) {
		XSetForeground(_scr->getDpy(), _gc_bg, color->getBg()->pixel);
	}
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontX11::drawText(Drawable dest, int x, int y, const char *text, uint chars, bool fg)
{
	GC gc = fg ? _gc_fg : _gc_bg;

	if (_font && (gc != None)) {
		XDrawString(_scr->getDpy(), dest, gc, x, y, text, chars);
	}
}

// PFontXmb

//! @brief PFontXmb constructor
PFontXmb::PFontXmb(PScreen *scr) : PFont(scr),
_fontset(NULL),
_gc_fg(None), _gc_bg(None)
{
	_type = FONT_TYPE_XMB;
}

//! @brief PFontXmb destructor
PFontXmb::~PFontXmb(void)
{
	unload();
}

//! @brief Loads Xmb font font_name
bool
PFontXmb::load(const std::string &font_name)
{
	unload();

	char *def_string;
	char **missing_list, **font_names;
	int missing_count, fnum;
	XFontStruct **fonts;

	string basename(font_name);
	basename.append(",*");

	_fontset =
		XCreateFontSet(_scr->getDpy(), basename.c_str(),
									 &missing_list, &missing_count, &def_string);

	if (_fontset == NULL) {
		return false;
	}

	for (int i = 0; i < missing_count; ++i)
		cerr <<  "PFontXmb::load(): Missing charset" <<  missing_list[i] << endl;

	_ascent = _descent = 0;

	fnum = XFontsOfFontSet (_fontset, &fonts, &font_names);
	for (int i = 0; i < fnum; ++i) {
		if (signed(_ascent) < fonts[i]->ascent)
			_ascent = fonts[i]->ascent;
		if (signed(_descent) < fonts[i]->descent)
			_descent = fonts[i]->descent;
	}

	_height = _ascent + _descent;

	// create GC's
	XGCValues gv;

	uint mask = GCFunction;
	gv.function = GXcopy;
	_gc_fg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);
	_gc_bg = XCreateGC(_scr->getDpy(), _scr->getRoot(), mask, &gv);

	return true;
}

//! @brief Unloads font resources
void
PFontXmb::unload(void)
{
	if (_fontset != NULL) {
		XFreeFontSet(_scr->getDpy(), _fontset);
		_fontset = NULL;
	}
}

//! @brief Gets the width the text would take using this font
uint
PFontXmb::getWidth(const char *text, uint max_chars)
{
	if (text == NULL)
		return 0;

	// make sure the max_chars has a decent value, if it's less than 1 wich it
	// defaults to set it to the numbers of chars in the string text
	if ((max_chars == 0) || (max_chars > strlen(text)))
		max_chars = strlen(text);

	uint width = 0;
	if (_fontset != NULL) {
		XRectangle ink, logical;
		XmbTextExtents(_fontset, text, max_chars, &ink, &logical);
		width = logical.width;
	}

	return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontXmb::drawText(Drawable dest, int x, int y, const char *text, uint chars, bool fg)
{
	GC gc = fg ? _gc_fg : _gc_bg;

	if ((_fontset != NULL) && (gc != None)) {
		XmbDrawString(_scr->getDpy(), dest, _fontset, gc, x, y, text, chars);
	}
}

//! @brief Sets the color that should be used when drawing
void
PFontXmb::setColor(PFont::Color *color)
{
	if (color->hasFg()) {
		XSetForeground(_scr->getDpy(), _gc_fg, color->getFg()->pixel);
	}

	if (color->hasBg()) {
		XSetForeground(_scr->getDpy(), _gc_bg, color->getBg()->pixel);
	}
}

// PFontXft

#ifdef HAVE_XFT

//! @brief PFontXft constructor
PFontXft::PFontXft(PScreen *scr) : PFont(scr),
_draw(NULL), _font(NULL),
_cl_fg(NULL), _cl_bg(NULL)
{
	_type = FONT_TYPE_XFT;
	_draw = XftDrawCreate(_scr->getDpy(), _scr->getRoot(),
												_scr->getVisual()->getXVisual(), _scr->getColormap());
}

//! @brief PFontXft destructor
PFontXft::~PFontXft(void)
{
	XftDrawDestroy(_draw);

	if (_cl_fg != NULL) {
		XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
								 _scr->getColormap(), _cl_fg);
	}
	if (_cl_bg != NULL) {
		XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
								 _scr->getColormap(), _cl_bg);
	}
}

//! @brief Loads the Xft font font_name
bool
PFontXft::load(const std::string &font_name)
{
	unload();

	_font = XftFontOpenName(_scr->getDpy(), _scr->getScreenNum(),
													font_name.c_str());
	if (_font == NULL) {
		_font = XftFontOpenXlfd(_scr->getDpy(), _scr->getScreenNum(),
														font_name.c_str());
	}

	if (_font != NULL) {
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
	if (_font != NULL) {
		XftFontClose(_scr->getDpy(), _font);
		_font = NULL;
	}
}

//! @brief Gets the width the text would take using this font
uint
PFontXft::getWidth(const char *text, uint max_chars)
{
	uint width = 0;

	// make sure the max_chars has a decent value, if it's less than 1 wich it
	// defaults to set it to the numbers of chars in the string text
	if ((max_chars > strlen(text)) || (max_chars < 1))
		max_chars = strlen(text);

	if (_font != NULL) {
		XGlyphInfo extents;
		XftTextExtents8(_scr->getDpy(), _font,
										(XftChar8 *) text, max_chars, &extents);

		width = extents.width;
	}

	return (width + _offset_x);
}

//! @brief Draws the text on dest
//! @param dest Drawable to draw on
//! @param chars Max numbers of characthers to print
//! @param fg Bool, to use foreground or background colors
void
PFontXft::drawText(Drawable dest, int x, int y, const char *text, uint chars, bool fg)
{
	XftColor *cl = fg ? _cl_fg : _cl_bg;

	if ((_font != NULL) && (cl != NULL)) {
		XftDrawChange(_draw, dest);
		XftDrawString8(_draw, cl, _font, x, y, (XftChar8 *) text, chars);
	}
}

//! @brief Sets the color that should be used when drawing
void
PFontXft::setColor(PFont::Color *color)
{
	if (_cl_fg != NULL) {
		XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
								 _scr->getColormap(), _cl_fg);
		_cl_fg = NULL;
	}
	if (_cl_bg != NULL) {
		XftColorFree(_scr->getDpy(), _scr->getVisual()->getXVisual(),
								 _scr->getColormap(), _cl_bg);
		_cl_bg = NULL;
	}

	if (color->hasFg()) {
		_xrender_color.red = color->getFg()->red;
		_xrender_color.green = color->getFg()->green;
		_xrender_color.blue = color->getFg()->blue;
		_xrender_color.alpha = color->getFgAlpha();

		_cl_fg = new XftColor;

		XftColorAllocValue(_scr->getDpy(), _scr->getVisual()->getXVisual(),
											 _scr->getColormap(), &_xrender_color, _cl_fg);
	}

	if (color->hasBg()) {
		_xrender_color.red = color->getBg()->red;
		_xrender_color.green = color->getBg()->green;
		_xrender_color.blue = color->getBg()->blue;
		_xrender_color.alpha = color->getBgAlpha();

		_cl_bg = new XftColor;

		XftColorAllocValue(_scr->getDpy(), _scr->getVisual()->getXVisual(),
											 _scr->getColormap(), &_xrender_color, _cl_bg);
	}
}

#endif // HAVE_XFT
