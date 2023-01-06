#ifndef _PEKWM_PFONT_XMB_HH_
#define _PEKWM_PFONT_XMB_HH_

#include "PFont.hh"

class PFontXmb : public PFont {
public:
	PFontXmb(void);
	virtual ~PFontXmb(void);

	// virtual interface
	virtual bool load(const std::string &name);
	virtual void unload(void);

	virtual uint getWidth(const std::string &text, uint max_chars = 0);

	virtual void setColor(PFont::Color *color);

private:
	virtual void drawText(Drawable dest, int x, int y,
			      const std::string &text, uint chars, bool fg);

	XFontSet _fontset;
	GC _gc_fg, _gc_bg;
	static const char *DEFAULT_FONTSET; /**< Default fallback fontset. */
};


#endif // _PEKWM_PFONT_XMB_HH_
