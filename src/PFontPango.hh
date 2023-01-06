//
// PFontPango.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PFONT_PANGO_HH_
#define _PEKWM_PFONT_PANGO_HH_

#include "PFont.hh"

extern "C" {
#include <pango/pango.h>
};

class PFontPango : public PFont {
public:
	PFontPango();
	virtual ~PFontPango();

	// virtual interface
	virtual bool load(const std::string& font_name);
	virtual void unload(void);

	virtual uint getWidth(const std::string& text, uint chars = 0) = 0;
	virtual void setColor(PFont::Color* color) = 0;

protected:
	int charsToLen(uint chars);

private:
	virtual void drawText(PSurface* dest, int x, int y,
			      const std::string& text, uint chars,
			      bool fg) = 0;

protected:
	PangoContext* _context;
	PangoFontMap* _font_map;
	PangoFont* _font;
	PangoFontDescription* _font_description;
};

#endif // _PEKWM_PFONT_PANGO_HH_
