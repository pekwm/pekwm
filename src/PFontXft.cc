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
	  _font(0)
{
}

PFontXft::~PFontXft(void)
{
	PFontXft::unload();
	XftDrawDestroy(_draw);
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
PFontXft::drawText(PSurface *dest, int x, int y,
		   const std::string &text, uint chars, bool fg)
{
	PXftColor& color = fg ? _color_fg : _color_bg;
	if (_font && color.isSet()) {
		std::string sub_text;
		if (chars != 0) {
			sub_text = text.substr(0, chars);
		} else {
			sub_text = text;
		}

		XftDrawChange(_draw, dest->getDrawable());
		XftDrawStringUtf8(_draw, *color, _font, x, y,
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
	_color_fg.unset();
	_color_bg.unset();

	if (color->hasFg()) {
		_color_fg.set(color->getFg()->red, color->getFg()->green,
			      color->getFg()->blue, color->getFgAlpha());
	}

	if (color->hasBg()) {
		_color_bg.set(color->getBg()->red, color->getBg()->green,
			      color->getBg()->blue, color->getBgAlpha());
	}
}
