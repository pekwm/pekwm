//
// PFontXmb.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "Debug.hh"
#include "PFontXmb.hh"
#include "X11.hh"

const char* PFontXmb::DEFAULT_FONTSET = "fixed";

PFontXmb::PFontXmb(void)
	: PFontX(),
	  _fontset(nullptr),
	  _gc_fg(None),
	  _gc_bg(None)
{
}

PFontXmb::~PFontXmb(void)
{
	PFontXmb::unload();
}

/**
 * Loads Xmb font font_name
 */
bool
PFontXmb::load(const PFont::Descr& descr)
{
	unload();

	std::string basename =
		descr.useStr() ? descr.str() : toNativeDescr(descr);
	size_t pos = basename.rfind(",*");
	if (pos == std::string::npos || pos != (basename.size() - 2)) {
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
		USER_WARN("failed to create fontset for " << basename);
		_fontset = XCreateFontSet(X11::getDpy(), DEFAULT_FONTSET,
					  &missing_list, &missing_count,
					  &def_string);
	}

	if (_fontset) {
		for (int i = 0; i < missing_count; ++i) {
			USER_WARN(basename << " missing charset "
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

/**
 * Unloads font resources
 */
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

/**
 * Gets the width the text would take using this font
 */
uint
PFontXmb::getWidth(const std::string &text, uint max_chars)
{
	if (! text.size()) {
		return 0;
	}

	// Make sure the max_chars has a decent value, if it's less than
	// 1 which it defaults to set it to the numbers of chars in the
	// string text
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

bool
PFontXmb::useAscentDescent(void) const
{
	return PFont::useAscentDescent();
}

/**
 * Draws the text on dest
 * @param dest Drawable to draw on
 * @param chars Max numbers of characthers to print
 * @param fg Bool, to use foreground or background colors
 */
void
PFontXmb::drawText(PSurface *dest, int x, int y,
		   const std::string &text, uint chars, bool fg)
{
	GC gc = fg ? _gc_fg : _gc_bg;

	if (_fontset && (gc != None)) {
		XmbDrawString(X11::getDpy(), dest->getDrawable(), _fontset, gc,
			      x, y, text.c_str(), chars ? chars : text.size());
	}
}

/**
 * Sets the color that should be used when drawing
 */
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
