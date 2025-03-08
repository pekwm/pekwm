//
// PFontPangoCario.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_PANGO_CAIRO_HH_
#define _PEKWM_PFONT_PANGO_CAIRO_HH_

#include "config.h"

#ifdef PEKWM_HAVE_PANGO_CAIRO

#include "PFontPango.hh"

extern "C" {
#include <pango/pangocairo.h>
}

class PFontPangoCairo : public PFontPango {
public:
	class Color {
	public:
		Color();
		~Color();

		void set(XColor* xcolor, uint alpha);

		bool is_set;
		double r;
		double g;
		double b;
		double a;
	};

	PFontPangoCairo(float scale);
	virtual ~PFontPangoCairo();

	virtual uint getWidth(const std::string& text, uint chars = 0);
	virtual void setColor(PFont::Color* color);

private:
	virtual void drawText(PSurface* dest, int x, int y,
			      const std::string& text, uint chars,
			      bool fg);

	void ensureCairoSurface(PSurface* dest);

private:
	// cache information for _cairo_surface
	Drawable _surface_drawable;
	uint _surface_width;
	uint _surface_height;

	// cairo surface used for drawing
	cairo_surface_t* _cairo_surface;
	// cairo_t used to create context, used while loading font.
	cairo_t* _cairo;

	Color _fg;
	Color _bg;
};

#endif // PEKWM_HAVE_PANGO_CAIRO

#endif // _PEKWM_PFONT_PANGO_CAIRO_HH_
