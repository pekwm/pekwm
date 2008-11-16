//
// TextureHandler.cc for pekwm
// Copyright © 2004-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PTexture.hh"
#include "TextureHandler.hh"
#include "PScreen.hh"
#include "PTexturePlain.hh"
#include "Util.hh"

#include <vector>
#include <iostream>

using std::list;
using std::vector;
using std::map;
using std::string;
using std::cerr;
using std::endl;

TextureHandler* TextureHandler::_instance = 0;
map<ParseUtil::Entry, PTexture::Type> TextureHandler::_parse_map = map<ParseUtil::Entry, PTexture::Type>();

const int TextureHandler::LENGTH_MIN = 5;

//! @brief TextureHandler constructor
TextureHandler::TextureHandler(void)
{
#ifdef DEBUG
    if (_instance) {
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "TextureHandler(" << this << ")::TextureHandler()"
             << " *** _instance allready set: " << _instance << endl;
    }
#endif // DEBUG
    _instance = this;

    // fill parse map with values
    _parse_map[""] = PTexture::TYPE_NO;
    _parse_map["SOLID"] = PTexture::TYPE_SOLID;
    _parse_map["SOLIDRAISED"] = PTexture::TYPE_SOLID_RAISED;
    _parse_map["IMAGE"] = PTexture::TYPE_IMAGE;
    _parse_map["EMPTY"] = PTexture::TYPE_EMPTY;
}

//! @brief TextureHandler destructor
TextureHandler::~TextureHandler(void)
{
    _instance = 0;
}

//! @brief Gets or creates a PTexture
PTexture*
TextureHandler::getTexture(const std::string &texture)
{
    // check for allready existing entry
    list<TextureHandler::Entry*>::iterator it(_texture_list.begin());

    for (; it != _texture_list.end(); ++it) {
        if (*(*it) == texture) {
            (*it)->incRef();
            return (*it)->getTexture();
        }
    }

    // parse texture
    PTexture *ptexture = parse(texture);

    if (ptexture) {
        // create new entry
        TextureHandler::Entry *entry = new TextureHandler::Entry(texture, ptexture);
        entry->incRef();

        _texture_list.push_back(entry);
    }

    return ptexture;
}

//! @brief Add/Increment reference cont for texture.
//! @return Pointer to texture referenced.
PTexture*
TextureHandler::referenceTexture(PTexture *texture)
{
    // Check for allready existing entry
    list<TextureHandler::Entry*>::iterator it(_texture_list.begin());

    for (; it != _texture_list.end(); ++it) {
        if ((*it)->getTexture() == texture) {
            (*it)->incRef();
            return texture;
        }
    }

    // Create new entry
    TextureHandler::Entry *entry = new TextureHandler::Entry("", texture);
    entry->incRef();

    _texture_list.push_back(entry);

    return texture;
}

//! @brief Returns a texture
void
TextureHandler::returnTexture(PTexture *texture)
{
    bool found = false;

    list<TextureHandler::Entry*>::iterator it(_texture_list.begin());
    for (; it != _texture_list.end(); ++it) {
        if ((*it)->getTexture() == texture) {
            found = true;

            (*it)->decRef();
            if ((*it)->getRef() == 0) {
                delete *it;
                _texture_list.erase(it);
            }
            break;
        }
    }

    if (! found) {
        delete texture;
    }
}

//! @brief Parses the string, and creates a texture
PTexture*
TextureHandler::parse(const std::string &texture)
{
    PTexture *ptexture = 0;
    vector<string> tok;

    PTexture::Type type;
    if (Util::splitString(texture, tok, " \t")) {
        type = ParseUtil::getValue<PTexture::Type>(tok[0], _parse_map);
    } else {
        type = ParseUtil::getValue<PTexture::Type>(texture, _parse_map);
    }

    // need atleast type and parameter, except for EMPTY type
    if (tok.size() > 1) {
        tok.erase(tok.begin()); // remove type
        switch (type) {
        case PTexture::TYPE_SOLID:
            ptexture = parseSolid(tok);
            break;
        case PTexture::TYPE_SOLID_RAISED:
            ptexture = parseSolidRaised(tok);
            break;
        case PTexture::TYPE_IMAGE:
            ptexture = new PTextureImage(PScreen::instance()->getDpy(), tok[1]);
            break;
        case PTexture::TYPE_NO:
        default:
            cerr << "*** WARNING: invalid texture type: " << tok[0] << endl;
            break;
        }

        // If it fails to load, set clean resources and set it to 0.
        if (ptexture && ! ptexture->isOk()) {
            delete ptexture;
            ptexture = 0;
        }

    } else if (type == PTexture::TYPE_EMPTY) {
        ptexture = new PTexture(PScreen::instance()->getDpy());
    }

    return ptexture;
}

//! @brief Parse and create PTextureSolid
PTexture*
TextureHandler::parseSolid(std::vector<std::string> &tok)
{
    if (tok.size() < 1) {
        cerr << "*** WARNING: not enough parameters to texture Solid" << endl;
        return 0;
    }

    PTextureSolid *tex = new PTextureSolid(PScreen::instance()->getDpy(), tok[0]);	tok.erase(tok.begin());

    // check if we have size
    if (tok.size() == 1) {
        parseSize(tex, tok[0]);
    }

    return tex;
}

//! @brief Parse and create PTextureSolidRaised
PTexture*
TextureHandler::parseSolidRaised(std::vector<std::string> &tok)
{
    if (tok.size() < 3) {
        cerr << "*** WARNING: not enough parameters to texture SolidRaised" << endl;
        return 0;
    }

    PTextureSolidRaised *tex = new PTextureSolidRaised(PScreen::instance()->getDpy(),
                                                       tok[0], tok[1], tok[2]);
    tok.erase(tok.begin(), tok.begin() + 3);

    // Check if we have line width and offset.
    if (tok.size() > 2) {
        tex->setLineWidth(strtol(tok[0].c_str(), 0, 10));
        tex->setLineOff(strtol(tok[1].c_str(), 0, 10));
        tok.erase(tok.begin(), tok.begin() + 2);
    }
    // Check if have side draw specificed.
    if (tok.size() > 4) {
        tex->setDraw(Util::isTrue(tok[0]), Util::isTrue(tok[1]),
                     Util::isTrue(tok[2]), Util::isTrue(tok[3]));
        tok.erase(tok.begin(), tok.begin() + 4);
    }

    // Check if we have size
    if (tok.size() == 1) {
        parseSize(tex, tok[0]);
    }

    return tex;
}

//! @brief Parses size paremeter ie 10x20
void
TextureHandler::parseSize(PTexture *tex, const std::string &size)
{
    vector<string> tok;
    if ((Util::splitString(size, tok, "x", 2)) == 2) {
        tex->setWidth(strtol(tok[0].c_str(), 0, 10));
        tex->setHeight(strtol(tok[1].c_str(), 0, 10));
    }
}
