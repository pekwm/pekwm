//
// PFontPangoXft.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "PFontPangoXft.hh"
#include "X11.hh"

#ifdef PEKWM_HAVE_PANGO_XFT

// PFontPangoXftLayout

/**
 * Wrapper for PangoLayout with resource management.
 */
class PFontPangoXftLayout {
public:
	PFontPangoXftLayout(PangoContext* context,
			    PangoFontDescription *font_description,
			    const std::string& text, int len)
		: _layout(pango_layout_new(context))
	{
		pango_layout_set_font_description(_layout,
						  font_description);
		pango_layout_set_text(_layout, text.c_str(), len);
	}
	~PFontPangoXftLayout()
	{
		g_object_unref(_layout);
	}

	PangoLayout* operator*() { return _layout; }

private:
	PFontPangoXftLayout(const PFontPangoXftLayout&);
	PFontPangoXftLayout& operator=(const PFontPangoXftLayout&);

private:
	PangoLayout* _layout;
};

// PFontPangoXft

PFontPangoXft::PFontPangoXft(float scale)
	: PFontPango(scale),
	  _draw(XftDrawCreate(X11::getDpy(), X11::getRoot(),
			      X11::getVisual(), X11::getColormap()))
{
	  _font_map = pango_xft_get_font_map(X11::getDpy(),
					     X11::getScreenNum());
	  _context = pango_font_map_create_context(_font_map);
}

PFontPangoXft::~PFontPangoXft()
{
	PFontPango::unload();
	XftDrawDestroy(_draw);
}

uint
PFontPangoXft::getWidth(const StringView &text)
{
	PFontPangoXftLayout layout(_context, _font_description, *text,
				   static_cast<int>(text.size()));

	PangoRectangle rect;
	pango_layout_get_pixel_extents(*layout, NULL, &rect);

	return rect.width;
}

void
PFontPangoXft::setColor(PFont::Color* color)
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

void
PFontPangoXft::drawText(PSurface* dest, int x, int y, const StringView& text,
			bool fg)
{
	XftColor* color;
	if (fg && _color_fg.isSet()) {
		color = *_color_fg;
	} else if (! fg && _color_bg.isSet()) {
		color = *_color_bg;
	} else {
		P_TRACE_IF(fg, "Pango foreground not set");
		return;
	}

	XftDrawChange(_draw, dest->getDrawable());
	PFontPangoXftLayout layout(_context, _font_description, *text,
				   static_cast<int>(text.size()));
	PangoLayoutLine* line = pango_layout_get_line_readonly(*layout, 0);
	drawPangoLine(x, y + _ascent, line, color);
}

void
PFontPangoXft::drawPangoLine(int x, int y,
			     PangoLayoutLine* line, XftColor* color)
{
	for (GSList* p = line->runs; p != NULL; p = p->next) {
		PangoLayoutRun* run =
			reinterpret_cast<PangoLayoutRun*>(p->data);
		pango_xft_render(_draw, color,
				 run->item->analysis.font, run->glyphs,
				 x, y);

		int width = pango_glyph_string_get_width(run->glyphs);
		x += width / PANGO_SCALE;
	}
}

#endif // PEKWM_HAVE_PANGO_XFT
