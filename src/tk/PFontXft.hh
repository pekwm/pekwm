//
// PFontXft.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_XFT_HH_
#define _PEKWM_PFONT_XFT_HH_

#include "config.h"

#ifdef PEKWM_HAVE_XFT

#include <string>

#include "PFont.hh"
#include "PXftColor.hh"

class PFontXft : public PFont {
public:
	PFontXft(float scale);
	virtual ~PFontXft();

	// virtual interface
	virtual bool load(const PFont::Descr& descr);
	virtual void unload(void);

	virtual uint getWidth(const std::string &text, uint max_chars = 0);
	virtual bool useAscentDescent(void) const;

	virtual void setColor(PFont::Color *color);

protected:
	virtual std::string toNativeDescr(const PFont::Descr &descr) const;

private:
	virtual void drawText(PSurface *dest, int x, int y,
			      const std::string &text, uint chars, bool fg);

	XftDraw *_draw;
	XftFont *_font;
	PXftColor _color_fg;
	PXftColor _color_bg;
};

#endif // PEKWM_HAVE_XFT

#endif // _PEKWM_PFONT_XFT_HH_
