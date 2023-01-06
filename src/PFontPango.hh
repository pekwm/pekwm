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

class PFontPango : public PFont {
public:
	PFontPango(void);
	virtual ~PFontPango(void);

	// virtual interface
	virtual bool load(const std::string& font_name);
	virtual void unload(void);

	virtual uint getWidth(const std::string& text, uint max_chars = 0);

	virtual void setColor(PFont::Color* color);

private:
	virtual void drawText(Drawable dest, int x, int y,
			      const std::string& text, uint chars,
			      bool fg);

};

#endif // _PEKWM_PFONT_PANGO_HH_
