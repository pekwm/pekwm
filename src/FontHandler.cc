//
// FontHandler.cc for pekwm
// Copyright (C) 2004-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <cctype>

#include "Color.hh"
#include "Debug.hh"
#include "FontHandler.hh"
#include "Util.hh"
#include "X11.hh"

#include "PFontX11.hh"
#include "PFontXmb.hh"
#ifdef PEKWM_HAVE_XFT
#include "PFontXft.hh"
#endif // PEKWM_HAVE_XFT
#ifdef PEKWM_HAVE_PANGO_CAIRO
#include "PFontPangoCairo.hh"
#endif // PEKWM_HAVE_PANGO_CAIRO
#ifdef PEKWM_HAVE_PANGO_XFT
#include "PFontPangoXft.hh"
#endif // PEKWM_HAVE_PANGO_XFT

/**
 * Font pattern used to detect X11 font names when no font type is
 * specified.
 */
#define X11_FONT_NAME_PATTERN  \
	"-[^-]+-[^-]+-[^-]+-[^-]+-[^-]+-[^-]+" \
	"-[*0-9]+-[*0-9]+-[*0-9]+-[*0-9]+" \
	"-[^-]+-[*0-9]+-[^-]+-[^-]+"

static Util::StringTo<PFont::Type> map_type[] =
	{{"", PFont::FONT_TYPE_AUTO},
	 {"X11", PFont::FONT_TYPE_X11},
	 {"XMB", PFont::FONT_TYPE_XMB},
	 {"XFT", PFont::FONT_TYPE_XFT},
	 {"PANGO", PFont::FONT_TYPE_PANGO},
	 {"PANGOCAIRO", PFont::FONT_TYPE_PANGO_CAIRO},
	 {"PANGOXFT", PFont::FONT_TYPE_PANGO_XFT},
	 {"EMPTY", PFont::FONT_TYPE_EMPTY},
	 {nullptr, PFont::FONT_TYPE_NO}};

static Util::StringTo<FontJustify> map_justify[] =
	{{"LEFT", FONT_JUSTIFY_LEFT},
	 {"CENTER", FONT_JUSTIFY_CENTER},
	 {"RIGHT", FONT_JUSTIFY_RIGHT},
	 {nullptr, FONT_JUSTIFY_NO}};

FontHandler::FontHandler(bool default_font_x11,
			 const std::string &charset_override)
	: _x11_font_name(X11_FONT_NAME_PATTERN),
	  _default_font_x11(default_font_x11),
	  _charset_override(charset_override)
{
}

FontHandler::~FontHandler(void)
{
	std::vector<HandlerEntry<PFont*> >::iterator fit = _fonts.begin();
	for (; fit != _fonts.end(); ++fit) {
		delete fit->getData();
	}

	std::vector<HandlerEntry<PFont::Color*> >::iterator cit =
		_colors.begin();
	for (; cit != _colors.end(); ++cit) {
		delete cit->getData();
	}
}

/**
 * @brief Gets or allocs a font
 *
 * Syntax of font specification goes as follows:
 *   "XFT#Font Name#Justify#Offset#Type" ex "XFT#Vera#Center#1 1"
 * where only the first field is obligatory and type needs to be the last
 */
PFont*
FontHandler::getFont(const std::string &font)
{
	// Check cache
	std::vector<HandlerEntry<PFont*> >::iterator it = _fonts.begin();
	for (; it != _fonts.end(); ++it) {
		if (*it == font) {
			it->incRef();
			return it->getData();
		}
	}

	// create new
	PFont::Type type;
	std::vector<std::string> tok;
	PFont *pfont = newFont(font, tok, type);

	std::string font_str(tok.front());
	tok.erase(tok.begin());
	if (type == PFont::FONT_TYPE_X11 || type == PFont::FONT_TYPE_XMB) {
		replaceFontCharset(font_str);
	}
	PFont::Descr descr(font_str, type != PFont::FONT_TYPE_AUTO);
	pfont->load(descr);

	// fields left for justify and offset
	parseFontOptions(pfont, tok);

	// create new entry
	HandlerEntry<PFont*> entry(font);
	entry.incRef();
	entry.setData(pfont);

	_fonts.push_back(entry);

	return pfont;
}

PFont*
FontHandler::newFont(const std::string &font,
		     std::vector<std::string> &tok, PFont::Type &type)
{
	if (_x11_font_name == font) {
		tok.push_back(font);
		return newFontX11(type);
	} else if (pekwm::ascii_ncase_equal("EMPTY", font)) {
		tok.push_back(font);
		return new PFontEmpty;
	}

	std::vector<std::string>::iterator tok_it;
	if (Util::splitString(font, tok, "#", 0, true) > 1) {
		tok_it = tok.begin();
		type = Util::StringToGet(map_type, *tok_it);
		if (type == PFont::FONT_TYPE_NO) {
			type = PFont::FONT_TYPE_AUTO;
		} else {
			tok.erase(tok_it);
		}
	} else {
		type = PFont::FONT_TYPE_AUTO;
	}

	switch (type) {
	case PFont::FONT_TYPE_AUTO:
		return newFontAuto();
#ifdef PEKWM_HAVE_XFT
	case PFont::FONT_TYPE_XFT:
		return new PFontXft;
#endif // PEKWM_HAVE_XFT
	case PFont::FONT_TYPE_X11:
		return new PFontX11;
	case PFont::FONT_TYPE_XMB:
		return new PFontXmb;
#ifdef PEKWM_HAVE_PANGO_CAIRO
	case PFont::FONT_TYPE_PANGO:
		return new PFontPangoCairo;
	case PFont::FONT_TYPE_PANGO_CAIRO:
		return new PFontPangoCairo;
#endif // PEKWM_HAVE_PANGO_CAIRO
#ifdef PEKWM_HAVE_PANGO_XFT
#ifndef PEKWM_HAVE_PANGO_CAIRO
	case PFont::FONT_TYPE_PANGO:
		return new PFontPangoCairo;
#endif // ! PEKWM_HAVE_PANGO_CAIRO
	case PFont::FONT_TYPE_PANGO_XFT:
		return new PFontPangoXft;
#endif // PEKWM_HAVE_PANGO_XFT
	case PFont::FONT_TYPE_EMPTY:
		return new PFontEmpty;
	default:
		return newFontX11(type);
	};
}

PFont*
FontHandler::newFontX11(PFont::Type &type) const
{
	if (_default_font_x11) {
		type = PFont::FONT_TYPE_X11;
		return new PFontX11;
	}
	type = PFont::FONT_TYPE_XMB;
	return new PFontXmb;
}

/**
 * Create font type, based on compiled in features. Priority list goes as
 * follows:
 *
 * # PangoCairo
 * # PangoXft
 * # Xft
 * # Xmb/X11 depending on _default_font_x11
 *
 */
PFont*
FontHandler::newFontAuto() const
{
#ifdef PEKWM_HAVE_PANGO_CAIRO
	return new PFontPangoCairo;
#else // ! PEKWM_HAVE_PANGO_CAIRO
#ifdef PEKWM_HAVE_PANGO_XFT
	return new PFontPangoXft;
#else // ! PEKWM_HAVE_PANGO_XFT
#ifdef PEKWM_HAVE_XFT
	return new PFontXft;
#else // ! PEKWM_HAVE_XFT
	PFont::Type type;
	return newFontX11(type);
#endif // PEKWM_HAVE_XFT
#endif // PEKWM_HAVE_PANGO_XFT
#endif // PEKWM_HAVE_PANGO_CAIRO
}

void
FontHandler::parseFontOptions(PFont *pfont,
			      std::vector<std::string> &tok)
{
	std::vector<std::string>::iterator s_it(tok.begin());
	for (; s_it != tok.end(); ++s_it) {
		if (! parseFontOffset(pfont, *s_it)) {
			parseFontJustify(pfont, *s_it);
		}
	}
}

bool
FontHandler::parseFontOffset(PFont *pfont, const std::string &str)
{
	if (str.size() == 0 || ! isdigit(str[0])) {
		return false;
	}

	std::vector<std::string> tok;
	if (Util::splitString(str, tok, " \t", 2) == 2) {
		try {
			pfont->setOffset(std::stoi(tok[0]),
					 std::stoi(tok[1]));
		} catch (std::invalid_argument&) { }
	}
	return true;
}

bool
FontHandler::parseFontJustify(PFont *pfont, const std::string &str)
{
	uint justify = Util::StringToGet(map_justify, str);
	if (justify == FONT_JUSTIFY_NO) {
		justify = FONT_JUSTIFY_LEFT;
	}
	pfont->setJustify(justify);
	return true;
}

/**
 * Replace last two -*-* parts of a font string and replace with
 * _charset_override if _charset_override is non empty.
 */
bool
FontHandler::replaceFontCharset(std::string &font)
{
	size_t begin;
	if (_charset_override.size() == 0
	    || (begin = font.rfind('-')) == std::string::npos || (begin == 0)
	    || (begin = font.rfind('-', begin - 1)) == std::string::npos) {
		return false;
	}

	// keep the -, default charset should be specified without leading -
	begin += 1;
	font.replace(begin, font.size() - begin, _charset_override);
	return true;
}

//! @brief Returns a font
void
FontHandler::returnFont(const PFont *font)
{
	std::vector<HandlerEntry<PFont*> >::iterator it = _fonts.begin();
	for (; it != _fonts.end(); ++it) {
		if (it->getData() == font) {
			it->decRef();
			if (! it->getRef()) {
				delete it->getData();
				_fonts.erase(it);
			}
			break;
		}
	}
}

//! @brief Gets or allocs a color
PFont::Color*
FontHandler::getColor(const std::string &color)
{
	// check cache
	std::vector<HandlerEntry<PFont::Color*> >::iterator it =
		_colors.begin();
	for (; it != _colors.end(); ++it) {
		if (*it == color) {
			it->incRef();
			return it->getData();
		}
	}

	// create new
	PFont::Color *font_color = new PFont::Color();

	std::vector<std::string> tok;
	if (Util::splitString(color, tok, " \t", 2) == 2) {
		loadColor(tok[0], font_color, true);
		loadColor(tok[1], font_color, false);
	} else {
		loadColor(color, font_color, true);
	}

	// create new entry
	HandlerEntry<PFont::Color*> entry(color);
	entry.incRef();
	entry.setData(font_color);

	_colors.push_back(entry);

	return font_color;
}

//! @brief Returns a color
void
FontHandler::returnColor(const PFont::Color *color)
{
	std::vector<HandlerEntry<PFont::Color*> >::iterator it =
		_colors.begin();
	for (; it != _colors.end(); ++it) {
		if (it->getData() == color) {
			it->decRef();
			if (! it->getRef()) {
				delete it->getData();
				_colors.erase(it);
			}
			break;
		}
	}
}

//! @brief Helper loader of font colors ( main and offset color )
void
FontHandler::loadColor(const std::string &color, PFont::Color *font_color,
		       bool fg)
{
	XColor *xc;

	std::vector<std::string> tok;
	if (Util::splitString(color, tok, ",", 2, true) == 2) {
		uint alpha = static_cast<uint>(strtol(tok[1].c_str(), 0, 10));
		if (alpha > 100) {
			USER_WARN("alpha for font color greater than 100%");
			alpha = 100;
		}

		alpha = static_cast<uint>(65535
				* (static_cast<float>(alpha) / 100));

		if (fg) {
			font_color->setFgAlpha(alpha);
		} else {
			font_color->setBgAlpha(alpha);
		}
		xc = pekwm::getColor(tok[0]);
	} else {
		xc = pekwm::getColor(color);
	}

	if (fg) {
		font_color->setFg(xc);
	} else {
		font_color->setBg(xc);
	}
}
