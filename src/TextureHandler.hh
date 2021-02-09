//
// TextureHandler.hh for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "PTexture.hh"

#include <map>
#include <string>
#include <vector>

extern "C" {
#include <string.h>
}

class PTexture;

class TextureHandler {
public:
    class Entry {
    public:
        Entry(const std::string &name, PTexture *texture)
            : _name(name),
              _texture(texture),
              _ref(0)
        {
        }
        ~Entry(void)
        {
            delete _texture;
        }

        PTexture *getTexture(void) { return _texture; }

        inline uint getRef(void) const { return _ref; }
        inline void incRef(void) { ++_ref; }
        inline void decRef(void) { if (_ref > 0) { --_ref; } }

        inline bool operator==(const std::string &name) {
            return (::strcasecmp(_name.c_str(), name.c_str()) == 0);
        }

    private:
        std::string _name;
        PTexture *_texture;

        uint _ref;
    };

    TextureHandler(void);
    ~TextureHandler(void);

    int getLengthMin(void) { return _length_min; }
    PTexture *getTexture(const std::string &texture);
    PTexture *referenceTexture(PTexture *texture);
    void returnTexture(PTexture *texture);

private:
    PTexture *parse(const std::string &texture);
    PTexture *parseSolid(std::vector<std::string> &tok);
    PTexture *parseSolidRaised(std::vector<std::string> &tok);
    PTexture *parseLines(bool horz, std::vector<std::string> &tok);
    PTexture *parseImage(const std::string& texture);
    PTexture *parseImageMapped(const std::string& texture);

    void parseSize(PTexture *tex, const std::string &size);

private:
    /** Minimum texture name length. */
    const int _length_min;

    std::vector<TextureHandler::Entry*> _textures;
    std::map<std::string, std::map<int,int>*> _color_maps;
};

namespace pekwm
{
    TextureHandler* textureHandler();
}
