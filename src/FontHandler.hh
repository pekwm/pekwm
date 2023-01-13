//
// FontHandler.hh for pekwm
// Copyright (C) 2004-2023 Claes Nästén <pekdon@gmail.com>
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

#include <map>
#include <string>

//! @brief FontHandler, a caching and font type transparent font handler.
class FontHandler {
public:
	FontHandler(bool default_font_x11,
		    const std::string &charset_override);
	~FontHandler(void);

	void setDefaultFontX11(bool default_font_x11) {
		_default_font_x11 = default_font_x11;
	}
	void setCharsetOverride(const std::string &charset_override) {
		_charset_override = charset_override;
	}

	PFont *getFont(const std::string &font);
	void returnFont(const PFont *font);

	PFont::Color *getColor(const std::string &color);
	void returnColor(const PFont::Color *color);

protected:
	bool parseFontOffset(PFont *pfont, const std::string &str);
	bool parseFontJustify(PFont *pfont, const std::string &str);
	bool replaceFontCharset(std::string &font);

	PFont *newFont(const std::string &font,
		       std::vector<std::string> &tok, PFont::Type &type);

private:
	PFont *newFontX11(PFont::Type &type) const;
	PFont *newFontAuto() const;
	void parseFontOptions(PFont *pfont,
			      std::vector<std::string> &tok);
	void loadColor(const std::string &color, PFont::Color *font_color,
		       bool fg);

private:
	/** Pattern matching X11/Xmb style font specifications. */
	RegexString _x11_font_name;
	/** If true, unspecified font type is X11 and not XMB. */
	bool _default_font_x11;
	/** If non empty, override charset in X11/XMB font strings. */
	std::string _charset_override;
	std::vector<HandlerEntry<PFont*> > _fonts;
	std::vector<HandlerEntry<PFont::Color*> > _colors;
};

namespace pekwm
{
	FontHandler* fontHandler();
}

#endif // _PEKWM_FONTHANDLER_HH_
