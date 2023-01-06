//
// PFontXft.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PFontXft.hh"
#include "X11.hh"

PFontXft::PFontXft(void)
	: PFont(),
	  _draw(XftDrawCreate(X11::getDpy(), X11::getRoot(),
			      X11::getVisual(), X11::getColormap())),
	  _font(0),
	  _cl_fg(0),
	  _cl_bg(0)
{
}

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

/**
 * Loads the Xft font font_name
 */
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

/**
 * Unloads font resources
 */
void
PFontXft::unload(void)
{
	if (_font) {
		XftFontClose(X11::getDpy(), _font);
		_font = 0;
	}
}

/**
 * Gets the width the text would take using this font.
 */
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

/**
 * Draws the text on dest
 * @param dest Drawable to draw on
 * @param chars Max numbers of characthers to print
 * @param fg Bool, to use foreground or background colors
 */
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

/**
 * Sets the color that should be used when drawing
 */
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
				   X11::getColormap(), &_xrender_color,
				   _cl_fg);
	}

	if (color->hasBg()) {
		_xrender_color.red = color->getBg()->red;
		_xrender_color.green = color->getBg()->green;
		_xrender_color.blue = color->getBg()->blue;
		_xrender_color.alpha = color->getBgAlpha();

		_cl_bg = new XftColor;

		XftColorAllocValue(X11::getDpy(), X11::getVisual(),
				   X11::getColormap(), &_xrender_color,
				   _cl_bg);
	}
}


