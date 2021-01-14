//
// FontHandler.cc for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <cctype>
#include <iostream>

#include "FontHandler.hh"
#include "x11.hh"
#include "Util.hh"

FontHandler font_handler_instance;
FontHandler* FontHandler::_instance = &font_handler_instance;

//! @brief FontHandler constructor
FontHandler::FontHandler(void)
{
    if (_map_justify.size() == 0) {
        _map_justify[""] = FONT_JUSTIFY_NO;
        _map_justify["LEFT"] = FONT_JUSTIFY_LEFT;
        _map_justify["CENTER"] = FONT_JUSTIFY_CENTER;
        _map_justify["RIGHT"] = FONT_JUSTIFY_RIGHT;
    }
    if (_map_type.size() == 0) {
        _map_type[""] = PFont::FONT_TYPE_NO;
        _map_type["X11"] = PFont::FONT_TYPE_X11;
        _map_type["XFT"] = PFont::FONT_TYPE_XFT;
        _map_type["XMB"] = PFont::FONT_TYPE_XMB;
    }
}

//! @brief FontHandler destructor
FontHandler::~FontHandler(void)
{
    for (auto it : _fonts) {
        delete it.getData();
    }

    for (auto it : _colours) {
        delete it.getData();
    }
}

//! @brief Gets or allocs a font
//!
//! Syntax of font specification goes as follows:
//!   "Font Name#Justify#Offset#Type" ex "Vera#Center#1 1#XFT"
//! where only the first field is obligatory and type needs to be the last
//!
PFont*
FontHandler::getFont(const std::string &font)
{
    // Check cache
    for (auto it : _fonts) {
        if (it == font) {
            it.incRef();
            return it.getData();
        }
    }

    // create new
    PFont *pfont = 0;

    std::vector<std::string> tok;
    std::vector<std::string>::iterator tok_it;
    if ((Util::splitString(font, tok, "#", 0, true)) > 1) {
        // Try getting the font type from the first paramter, if that
        // doesn't work fall back to the last. This is to backwards
        // compatible.
        tok_it = tok.begin();
        uint type = ParseUtil::getValue<PFont::Type>(*tok_it, _map_type);
        if (type == PFont::FONT_TYPE_NO) {
            tok_it = tok.end() - 1;
            type = ParseUtil::getValue<PFont::Type>(*tok_it, _map_type);
        }

        switch (type) {
        case PFont::FONT_TYPE_XMB:
            pfont = new PFontXmb;
            tok.erase(tok_it);
            break;
#ifdef HAVE_XFT
        case PFont::FONT_TYPE_XFT:
            pfont = new PFontXft;
            tok.erase(tok_it);
            break;
#endif // HAVE_XFT
        case PFont::FONT_TYPE_X11:
            pfont = new PFontX11;
            tok.erase(tok_it);
            break;
        default:
            pfont = new PFontXmb;
            break;
        };
        pfont->load(tok.front());

        // Remove used fields, type and
        tok.erase(tok.begin());

        // fields left for justify and offset
        auto s_it(tok.begin());
        for (; s_it != tok.end(); ++s_it) {
            if (isdigit((*s_it)[0])) { // number
                std::vector<std::string> tok_2;
                if (Util::splitString(*s_it, tok_2, " \t", 2) == 2) {
                    pfont->setOffset(strtol(tok_2[0].c_str(), 0, 10),
                           strtol(tok_2[1].c_str(), 0, 10));
                }
            } else { // justify
                uint justify = ParseUtil::getValue<FontJustify>(*s_it, _map_justify);
                if (justify == FONT_JUSTIFY_NO) {
                    justify = FONT_JUSTIFY_LEFT;
                }
                pfont->setJustify(justify);
            }
        }
    } else {
        pfont = new PFontXmb;
        pfont->load(font);
    }

    // create new entry
    HandlerEntry<PFont*> entry(font);
    entry.incRef();
    entry.setData(pfont);

    _fonts.push_back(entry);

    return pfont;
}

//! @brief Returns a font
void
FontHandler::returnFont(PFont *font)
{
    auto it = _fonts.begin();
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
    for (auto it : _colours) {
        if (it == color) {
            it.incRef();
            return it.getData();
        }
    }

    // create new
    PFont::Color *font_color = new PFont::Color();
    font_color->setHasFg(true);

    std::vector<std::string> tok;
    if (Util::splitString(color, tok, " \t", 2) == 2) {
        loadColor(tok[0], font_color, true);
        loadColor(tok[1], font_color, false);
        font_color->setHasBg(true);
    } else {
        loadColor(color, font_color, true);
    }

    // create new entry
    HandlerEntry<PFont::Color*> entry(color);
    entry.incRef();
    entry.setData(font_color);

    _colours.push_back(entry);

    return font_color;
}

//! @brief Returns a color
void
FontHandler::returnColor(PFont::Color *color)
{
    auto it(_colours.begin());
    for (; it != _colours.end(); ++it) {
        if (it->getData() == color) {
            it->decRef();
            if (! it->getRef()) {
                delete it->getData();
                _colours.erase(it);
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
            std::cerr << " *** WARNING: Alpha for font color greater than 100%"
                      << std::endl;
            alpha = 100;
        }

        alpha = static_cast<uint>(65535 * (static_cast<float>(alpha) / 100));

        if (fg) {
            font_color->setFgAlpha(alpha);
        } else {
            font_color->setBgAlpha(alpha);
        }
        xc = X11::getColor(tok[0]);
    } else {
        xc = X11::getColor(color);
    }

    if (fg) {
        font_color->setFg(xc);
    } else {
        font_color->setBg(xc);
    }
}

//! @brief Helper unloader of font colors
void
FontHandler::freeColor(PFont::Color *font_color)
{
    if (font_color->hasFg()) {
        X11::returnColor(font_color->getFg());
    }
    
    if (font_color->hasBg()) {
        X11::returnColor(font_color->getBg());
    }
    
    delete font_color;
}
