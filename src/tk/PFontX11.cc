//
// PFontX11.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "PFontX11.hh"
#include "ThemeUtil.hh"
#include "X11.hh"

const char *FALLBACK_FONT = "fixed";

PFontX11::PFontX11(float scale, PPixmapSurface &surface)
	: PFontX(scale, surface),
	  _font(nullptr)
{
}

PFontX11::~PFontX11()
{
	PFontX11::unload();
}

bool
PFontX11::doLoadFont(const std::string &spec)
{
	_font = XLoadQueryFont(X11::getDpy(), spec.c_str());
	if (! _font) {
		_font = XLoadQueryFont(X11::getDpy(), FALLBACK_FONT);
		if (! _font) {
			return false;
		}
	}

	XSetFont(X11::getDpy(), _gc_fg, _font->fid);
	XSetFont(X11::getDpy(), _gc_bg, _font->fid);

	_ascent = ThemeUtil::scaledPixelValue(_scale, _font->ascent);
	_descent = ThemeUtil::scaledPixelValue(_scale, _font->descent);
	_height = _ascent + _descent;
	return true;
}

void
PFontX11::doUnloadFont()
{
	if (_font) {
		XFreeFont(X11::getDpy(), _font);
		_font = nullptr;
	}
}

/**
 * Gets the width the text would take using this font
 */
uint
PFontX11::getWidth(const StringView &text)
{
	if (! text.size()) {
		return 0;
	}

	std::string mb_text = Charset::toSystem(text);
	uint width = doGetWidth(mb_text);
	return ThemeUtil::scaledPixelValue(_scale, width) + _offset_x;
}

bool
PFontX11::useAscentDescent() const
{
	return false;
}

/**
 * Draws the text on dest
 * @param dest Drawable to draw on
 * @param chars Max numbers of characthers to print
 * @param fg Bool, to use foreground or background colors
 */
void
PFontX11::drawText(PSurface *dest, int x, int y, const StringView &text,
		   bool fg)
{
	GC gc = fg ? _gc_fg : _gc_bg;
	if (! _font || gc == None) {
		return;
	}

	std::string mb_text = Charset::toSystem(text);
	if (isScaled()) {
		drawScaled(dest, x, y, mb_text, gc,
			   fg ? _pixel_fg : _pixel_bg);
	} else {
		doDrawText(dest->getDrawable(), x, y, mb_text, gc);
	}
}

void
PFontX11::doDrawText(Drawable draw, int x, int y, const StringView &text,
		     GC gc)
{
	XDrawString(X11::getDpy(), draw, gc, x, y + _font->ascent,
		    *text, text.size());
}

int
PFontX11::doGetWidth(const StringView &text) const
{
	return _font ? XTextWidth(_font, *text, text.size()) : 0;
}

int
PFontX11::doGetHeight() const
{
	return _font ? _font->ascent + _font->descent : 0;
}
