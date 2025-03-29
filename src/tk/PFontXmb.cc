//
// PFontXmb.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "Debug.hh"
#include "PFontXmb.hh"
#include "ThemeUtil.hh"
#include "X11.hh"

const char* PFontXmb::DEFAULT_FONTSET = "fixed";

PFontXmb::PFontXmb(float scale, PPixmapSurface &surface)
	: PFontX(scale, surface),
	  _fontset(nullptr),
	  _oascent(0),
	  _odescent(0)
{
}

PFontXmb::~PFontXmb()
{
	PFontXmb::unload();
}

bool
PFontXmb::doLoadFont(const std::string &spec)
{
	std::string basename(spec);
	size_t pos = basename.rfind(",*");
	if (pos == std::string::npos || pos != (basename.size() - 2)) {
		
		basename.append(",*");
	}
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

	if (! _fontset) {
		return false;
	}


	for (int i = 0; i < missing_count; ++i) {
		USER_WARN(basename << " missing charset " <<  missing_list[i]);
	}
	XFreeStringList(missing_list);

	_oascent = 0;
	_odescent = 0;

	XFontStruct **fonts;
	int fnum = XFontsOfFontSet (_fontset, &fonts, &font_names);
	for (int i = 0; i < fnum; ++i) {
		if (fonts[i]->ascent > static_cast<int>(_oascent)) {
			_oascent = fonts[i]->ascent;
		}
		if (fonts[i]->descent > static_cast<int>(_odescent)) {
			_odescent = fonts[i]->descent;
		}
	}
	_ascent = ThemeUtil::scaledPixelValue(_scale, _oascent);
	_descent = ThemeUtil::scaledPixelValue(_scale, _odescent);
	_height = _ascent + _descent;
	return true;
}

void
PFontXmb::doUnloadFont()
{
	if (_fontset) {
		XFreeFontSet(X11::getDpy(), _fontset);
		_fontset = 0;
	}
}

/**
 * Gets the width the text would take using this font
 */
uint
PFontXmb::getWidth(const StringView &text)
{
	if (! text.size()) {
		return 0;
	}
	uint width = doGetWidth(text);
	return ThemeUtil::scaledPixelValue(_scale, width + _offset_x);
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
PFontXmb::drawText(PSurface *dest, int x, int y, const StringView &text,
		   bool fg)
{
	GC gc = fg ? _gc_fg : _gc_bg;
	if (! _fontset || gc == None) {
		return;
	}

	if (isScaled()) {
		drawScaled(dest, x, y, text, gc, fg ? _pixel_fg : _pixel_bg);
	} else {
		doDrawText(dest->getDrawable(), x, y, text, gc);
	}
}

void
PFontXmb::doDrawText(Drawable draw, int x, int y, const StringView &text,
		     GC gc)
{
#ifdef X_HAVE_UTF8_STRING
	Xutf8DrawString(X11::getDpy(), draw, _fontset, gc, x, y + _oascent,
			*text, text.size());
#else // ! X_HAVE_UTF8_STRING
	XmbDrawString(X11::getDpy(), draw, _fontset, gc, x, y + _oascent,
		      *text, text.size());
#endif // X_HAVE_UTF8_STRING
}

int
PFontXmb::doGetWidth(const StringView &text) const
{
	uint width = 0;
	if (_fontset) {
		XRectangle ink, logical;
		XmbTextExtents(_fontset, *text, text.size(), &ink, &logical);
		width = logical.width;
	}
	return width;
}

int
PFontXmb::doGetHeight() const
{
	return _oascent + _odescent;
}
