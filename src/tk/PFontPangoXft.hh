//
// PFontPangoXft.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_PANGO_XFT_HH_
#define _PEKWM_PFONT_PANGO_XFT_HH_

#include "config.h"

#ifdef PEKWM_HAVE_PANGO_XFT

#include "PFontPango.hh"
#include "PXftColor.hh"

extern "C" {
#include <pango/pangoxft.h>
}

class PFontPangoXft : public PFontPango {
public:
	PFontPangoXft(float scale);
	virtual ~PFontPangoXft();

	// virtual interface
	virtual uint getWidth(const StringView& view);
	virtual void setColor(PFont::Color* color);

private:
	virtual void drawText(PSurface* dest, int x, int y,
			      const StringView& text, bool fg);

	void drawPangoLine(int x, int y, PangoLayoutLine* line,
			   XftColor* color);

private:
	XftDraw* _draw;
	PXftColor _color_fg;
	PXftColor _color_bg;
};

#endif // PEKWM_HAVE_PANGO_XFT

#endif // _PEKWM_PFONT_PANGO_XFT_HH_
