//
// FontHandler.hh for pekwm
// Copyright (C) 2004-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_FONTHANDLER_HH_
#define _PEKWM_FONTHANDLER_HH_

#include "config.h"

#include "Handler.hh"
#include "PFont.hh"
#include "RegexString.hh"
#include "PPixmapSurface.hh"

#include <map>
#include <string>

/**
 * FontHandler, a caching and font type transparent font handler.
 */
class FontHandler {
public:
	FontHandler(float scale, bool default_font_x11,
		    const std::string &charset_override);
	~FontHandler();

	void setScale(float scale) { _scale = scale; }

	void setDefaultFontX11(bool default_font_x11) {
		_default_font_x11 = default_font_x11;
	}
	void setCharsetOverride(const std::string &charset_override) {
		_charset_override = charset_override;
	}

	PFont *getFont(const std::string &font);
	void returnFont(PFont **font);

	PFont::Color *getColor(const std::string &color);
	void returnColor(PFont::Color **color);

protected:
	bool parseFontOffset(PFont *pfont, const std::string &str);
	bool parseFontJustify(PFont *pfont, const std::string &str);
	bool replaceFontCharset(std::string &font);

	PFont *newFont(const std::string &font,
		       std::vector<std::string> &tok, PFont::Type &type);

private:
	PFont *newFontX11(PFont::Type &type);
	PFont *newFontAuto() const;
	void parseFontOptions(PFont *pfont,
			      std::vector<std::string> &tok);
	void loadColor(const std::string &color, PFont::Color *font_color,
		       bool fg);

	/** Pattern matching X11/Xmb style font specifications. */
	RegexString _x11_font_name;
	/** If != 1.0, font size gets adjusted by factor. */
	float _scale;
	/** If true, unspecified font type is X11 and not XMB. */
	bool _default_font_x11;
	/** If non empty, override charset in X11/XMB font strings. */
	std::string _charset_override;
	std::vector<HandlerEntry<PFont*> > _fonts;
	std::vector<HandlerEntry<PFont::Color*> > _colors;
	/** Surface used as temporary destination when scaling output. */
	PPixmapSurface _surface;
};

namespace pekwm
{
	FontHandler* fontHandler();
}

#endif // _PEKWM_FONTHANDLER_HH_
