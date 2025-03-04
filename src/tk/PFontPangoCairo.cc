//
// PFontPangoCairo.cc for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "PFontPangoCairo.hh"
#include "X11.hh"

#ifdef PEKWM_HAVE_PANGO_CAIRO

extern "C" {
#include <cairo/cairo-xlib.h>
}

// PFontPangoCairoLayout

/**
 * Wrapper for PangoLayout with resource management.
 */
class PFontPangoCairoLayout {
public:
	PFontPangoCairoLayout(cairo_surface_t* cairo_surface,
			      PangoFontDescription* font_description,
			      const std::string& text, int len)
		: _cairo(cairo_create(cairo_surface)),
		  _layout(pango_cairo_create_layout(_cairo))
	{
		pango_layout_set_font_description(_layout,
						  font_description);
		pango_layout_set_text(_layout, text.c_str(), len);
	}
	~PFontPangoCairoLayout()
	{
		g_object_unref(_layout);
		cairo_destroy(_cairo);
	}

	cairo_t* getCairo() { return _cairo; }
	PangoLayout* operator*() { return _layout; }

private:
	PFontPangoCairoLayout(const PFontPangoCairoLayout&);
	PFontPangoCairoLayout& operator=(const PFontPangoCairoLayout&);

private:
	cairo_t* _cairo;
	PangoLayout* _layout;
};

// PFontPangoCairo::Color

PFontPangoCairo::Color::Color()
	: is_set(false),
	  r(0.0),
	  g(0.0),
	  b(0.0),
	  a(1.0)
{
}

PFontPangoCairo::Color::~Color()
{
}

void
PFontPangoCairo::Color::set(XColor* xcolor, uint alpha)
{
	if (xcolor == nullptr) {
		is_set = false;
	} else {
		is_set = true;
		r = static_cast<double>(xcolor->red) / 65535.0;
		g = static_cast<double>(xcolor->green) / 65535.0;;
		b = static_cast<double>(xcolor->blue) / 65535.0;;
		a = static_cast<double>(alpha) / 65535.0;;
	}
}

// PFontPangoCairo

PFontPangoCairo::PFontPangoCairo(float scale)
	: PFontPango(scale),
	  _surface_drawable(X11::getRoot()),
	  _surface_width(X11::getWidth()),
	  _surface_height(X11::getHeight()),
	  _cairo_surface(cairo_xlib_surface_create(X11::getDpy(),
						   _surface_drawable,
						   X11::getVisual(),
						   _surface_width,
						   _surface_height)),
	  _cairo(cairo_create(_cairo_surface))
{
	  _context = pango_cairo_create_context(_cairo);
	  _font_map = pango_context_get_font_map(_context);
}

PFontPangoCairo::~PFontPangoCairo()
{
	PFontPango::unload();
	cairo_surface_destroy(_cairo_surface);
	cairo_destroy(_cairo);
}

uint
PFontPangoCairo::getWidth(const std::string& text, uint chars)
{
	PFontPangoCairoLayout layout(_cairo_surface, _font_description,
				     text, charsToLen(chars));

	PangoRectangle rect;
	pango_layout_get_pixel_extents(*layout, NULL, &rect);

	return rect.width;
}

void
PFontPangoCairo::setColor(PFont::Color* color)
{
	_fg.set(color->getFg(), color->getFgAlpha());
	_bg.set(color->getBg(), color->getBgAlpha());
}

void
PFontPangoCairo::drawText(PSurface* dest, int x, int y,
			  const std::string& text, uint chars,
			  bool fg)
{
	PFontPangoCairo::Color& c = fg ? _fg : _bg;
	if (! c.is_set) {
		return;
	}

	cairo_surface_t *cairo_surface =
		cairo_xlib_surface_create(X11::getDpy(), dest->getDrawable(),
					  X11::getVisual(),
					  dest->getWidth(), dest->getHeight());

	PFontPangoCairoLayout layout(cairo_surface, _font_description,
				     text, charsToLen(chars));
	cairo_set_source_rgba(layout.getCairo(), c.r, c.g, c.b, c.a);
	double dx = x;
	double dy = y + _ascent;
	cairo_device_to_user(layout.getCairo(), &dx, &dy);
	cairo_move_to(layout.getCairo(), dx, dy);

	PangoLayoutLine* line = pango_layout_get_line_readonly(*layout, 0);
	pango_cairo_show_layout_line(layout.getCairo(), line);

	cairo_surface_flush(cairo_surface);
	cairo_surface_destroy(cairo_surface);
}

#endif // PEKWM_HAVE_PANGO_CAIRO
