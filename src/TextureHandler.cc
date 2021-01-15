//
// TextureHandler.cc for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "PTexture.hh"
#include "TextureHandler.hh"
#include "x11.hh"
#include "PTexturePlain.hh"
#include "Util.hh"

#include <iostream>

//! @brief TextureHandler constructor
TextureHandler::TextureHandler(void)
    : _length_min(5)
{
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
}

//! @brief Gets or creates a PTexture
PTexture*
TextureHandler::getTexture(const std::string &texture)
{
    // check for already existing entry
    auto it(_textures.begin());
    for (; it != _textures.end(); ++it) {
        if (*(*it) == texture) {
            (*it)->incRef();
            return (*it)->getTexture();
        }
    }

    // parse texture
    auto ptexture = parse(texture);
    if (ptexture) {
        // create new entry
        auto entry = new TextureHandler::Entry(texture, ptexture);
        entry->incRef();
        _textures.push_back(entry);
    }

    return ptexture;
}

//! @brief Add/Increment reference cont for texture.
//! @return Pointer to texture referenced.
PTexture*
TextureHandler::referenceTexture(PTexture *texture)
{
    // Check for already existing entry
    auto it(_textures.begin());
    for (; it != _textures.end(); ++it) {
        if ((*it)->getTexture() == texture) {
            (*it)->incRef();
            return texture;
        }
    }

    // Create new entry
    auto entry = new TextureHandler::Entry("", texture);
    entry->incRef();
    _textures.push_back(entry);

    return texture;
}

//! @brief Returns a texture
void
TextureHandler::returnTexture(PTexture *texture)
{
    bool found = false;

    auto it(_textures.begin());
    for (; it != _textures.end(); ++it) {
        if ((*it)->getTexture() == texture) {
            found = true;

            (*it)->decRef();
            if ((*it)->getRef() == 0) {
                delete *it;
                _textures.erase(it);
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
    std::vector<std::string> tok;

    PTexture::Type type;
    if (Util::splitString(texture, tok, " \t")) {
        type = ParseUtil::getValue<PTexture::Type>(tok[0], _parse_map);
    } else {
        type = ParseUtil::getValue<PTexture::Type>(texture, _parse_map);
    }

    // need at least type and parameter, except for EMPTY type
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
            // 6==strlen("IMAGE ")
            ptexture = new PTextureImage(texture.substr(6));
            if (! ptexture->isOk()) {
                auto image = static_cast<PTextureImage*>(ptexture);
                auto pos = texture.find_first_not_of(" \t", 6);
                image->setImage(texture.substr(pos));
            }
            break;
        case PTexture::TYPE_NO:
        default:
            break;
        }

        // If it fails to load, set clean resources and set it to 0.
        if (ptexture && ! ptexture->isOk()) {
            delete ptexture;
            ptexture = 0;
        }

    } else if (type == PTexture::TYPE_EMPTY) {
        ptexture = new PTexture;
    }

    return ptexture;
}

//! @brief Parse and create PTextureSolid
PTexture*
TextureHandler::parseSolid(std::vector<std::string> &tok)
{
    if (tok.size() < 1) {
        std::cerr << "*** WARNING: not enough parameters to texture Solid"
                  << std::endl;
        return 0;
    }

    PTextureSolid *tex = new PTextureSolid(tok[0]);
    tok.erase(tok.begin());

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
        std::cerr << "*** WARNING: not enough parameters to texture SolidRaised"
                  << std::endl;
        return 0;
    }

    PTextureSolidRaised *tex = new PTextureSolidRaised(tok[0], tok[1], tok[2]);
    tok.erase(tok.begin(), tok.begin() + 3);

    // Check if we have line width and offset.
    if (tok.size() > 2) {
        tex->setLineWidth(strtol(tok[0].c_str(), 0, 10));
        tex->setLineOff(strtol(tok[1].c_str(), 0, 10));
        tok.erase(tok.begin(), tok.begin() + 2);
    }
    // Check if have side draw specified.
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

//! @brief Parses size parameter, i.e. 10x20
void
TextureHandler::parseSize(PTexture *tex, const std::string &size)
{
    std::vector<std::string> tok;
    if ((Util::splitString(size, tok, "x", 2, true)) == 2) {
        tex->setWidth(strtol(tok[0].c_str(), 0, 10));
        tex->setHeight(strtol(tok[1].c_str(), 0, 10));
    }
}
