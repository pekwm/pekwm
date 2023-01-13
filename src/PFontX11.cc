//
// PFontX11.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "PFontX11.hh"
#include "X11.hh"

const char *FALLBACK_FONT = "fixed";

PFontX11::PFontX11(void)
	: PFontX(),
	  _font(0),
	  _gc_fg(None),
	  _gc_bg(None)
{
}

PFontX11::~PFontX11(void)
{
	PFontX11::unload();
}

/**
 * @brief Loads the X11 font font_name
 */
bool
PFontX11::load(const PFont::Descr& descr)
{
	unload();

	std::string spec = descr.useStr() ? descr.str() : toNativeDescr(descr);
	_font = XLoadQueryFont(X11::getDpy(), spec.c_str());
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

/**
 * Unloads font resources
 */
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

/**
 * Gets the width the text would take using this font
 */
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

/**
 * Draws the text on dest
 * @param dest Drawable to draw on
 * @param chars Max numbers of characthers to print
 * @param fg Bool, to use foreground or background colors
 */
void
PFontX11::drawText(PSurface *dest, int x, int y,
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

		XDrawString(X11::getDpy(), dest->getDrawable(), gc, x, y,
			    mb_text.c_str(), mb_text.size());
	}
}

/**
 * Sets the color that should be used when drawing
 */
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


