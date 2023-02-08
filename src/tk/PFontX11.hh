//
// PFontX11.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_X11_HH_
#define _PEKWM_PFONT_X11_HH_

#include "PFontX.hh"

class PFontX11 : public PFontX {
public:
	PFontX11(void);
	virtual ~PFontX11(void);

	// virtual interface
	virtual bool load(const PFont::Descr& descr);
	virtual void unload(void);

	virtual uint getWidth(const std::string &text, uint max_chars = 0);
	virtual bool useAscentDescent(void) const;

	virtual void setColor(PFont::Color *color);

private:
	virtual void drawText(PSurface *dest, int x, int y,
			      const std::string &text, uint chars, bool fg);

private:
	XFontStruct *_font;
	GC _gc_fg, _gc_bg;
};

#endif // _PEKWM_PFONT_X11_HH_
