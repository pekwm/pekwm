//
// PFontX11.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_X11_HH_
#define _PEKWM_PFONT_X11_HH_

#include "PFontX.hh"

class PFontX11 : public PFontX {
public:
	PFontX11(float scale, PPixmapSurface &surface);
	virtual ~PFontX11();

	virtual uint getWidth(const StringView &text);
	virtual bool useAscentDescent() const;

private:
	virtual bool doLoadFont(const std::string &spec);
	virtual void doUnloadFont();

	virtual void drawText(PSurface *dest, int x, int y,
			      const StringView &text, bool fg);
	virtual void doDrawText(Drawable draw, int x, int y,
				const StringView &text, GC gc);
	virtual int doGetWidth(const StringView &text) const;
	virtual int doGetHeight() const;

private:
	XFontStruct *_font;
};

#endif // _PEKWM_PFONT_X11_HH_
