//
// PFontX.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
#ifndef _PEKWM_PFONT_X_HH_
#define _PEKWM_PFONT_X_HH_

#include "PFont.hh"
#include "PPixmapSurface.hh"

class PFontX : public PFont {
public:
	PFontX(float scale, PPixmapSurface &surface);
	virtual ~PFontX();

	virtual bool load(const PFont::Descr &descr);
	virtual void unload();

	virtual void setColor(PFont::Color *color);

protected:
	std::string toNativeDescr(const PFont::Descr &descr) const;

	void drawScaled(PSurface *dest, int x, int y,
			const StringView &text, GC gc, ulong color);

	Drawable getShadowSurface(uint width, uint height);

	GC _gc_fg;
	GC _gc_bg;
	ulong _pixel_fg;
	ulong _pixel_bg;

private:
	virtual bool doLoadFont(const std::string &spec) = 0;
	virtual void doUnloadFont() = 0;
	virtual void doDrawText(Drawable draw, int x, int y,
				const StringView &text, GC gc) = 0;
	virtual int doGetWidth(const StringView &text) const = 0;
	virtual int doGetHeight() const = 0;

	/** Surface used as temporary destination when scaling output, shared
	 * between all X fonts. */
	PPixmapSurface &_surface;
};


#endif // _PEKWM_PFONT_X_HH_
