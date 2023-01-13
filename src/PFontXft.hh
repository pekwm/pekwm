//
// PFontXft.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_XFT_HH_
#define _PEKWM_PFONT_XFT_HH_

#include <string>

extern "C" {
#include <X11/Xft/Xft.h>
}

#include "PFont.hh"
#include "X11Util.hh"

class PFontXft : public PFont {
public:
	PFontXft(void);
	virtual ~PFontXft(void);

	// virtual interface
	virtual bool load(const PFont::Descr& descr);
	virtual void unload(void);

	virtual uint getWidth(const std::string &text, uint max_chars = 0);

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

#endif // _PEKWM_PFONT_XFT_HH_
