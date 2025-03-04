//
// PFontXmb.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_XMB_HH_
#define _PEKWM_PFONT_XMB_HH_

#include "PFontX.hh"

class PFontXmb : public PFontX {
public:
	PFontXmb(float scale, PPixmapSurface &surface);
	virtual ~PFontXmb();

	virtual uint getWidth(const std::string &text, uint max_chars = 0);
	virtual bool useAscentDescent() const;

private:
	virtual bool doLoadFont(const std::string &basename);
	virtual void doUnloadFont();

	virtual void drawText(PSurface *dest, int x, int y,
			      const std::string &text, uint chars, bool fg);
	virtual void doDrawText(Drawable draw, int x, int y,
				const std::string &text, int size, GC gc);
	virtual int doGetWidth(const std::string &text, int size) const;
	virtual int doGetHeight() const;

	XFontSet _fontset;
	uint _oascent;
	uint _odescent;
	static const char *DEFAULT_FONTSET; /**< Default fallback fontset. */
};


#endif // _PEKWM_PFONT_XMB_HH_
