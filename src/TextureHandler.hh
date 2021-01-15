//
// TextureHandler.hh for pekwm
// Copyright (C) 2005-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _TEXTURE_HANDLER_HH_
#define _TEXTURE_HANDLER_HH_

#include "config.h"

#include "ParseUtil.hh"

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
        Entry(const std::string &name, PTexture *texture) : _name(name), _texture(texture), _ref(0) { }
        ~Entry(void) { delete _texture; }

        PTexture *getTexture(void) { return _texture; }

        inline uint getRef(void) const { return _ref; }
        inline void incRef(void) { ++_ref; }
        inline void decRef(void) { if (_ref > 0) { --_ref; }
        }

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

    void parseSize(PTexture *tex, const std::string &size);

private:
    std::map<ParseUtil::Entry, PTexture::Type> _parse_map;
    /** Minimum texture name length. */
    const int _length_min;

    std::vector<TextureHandler::Entry*> _textures;
};

namespace pekwm
{
    TextureHandler* textureHandler();
};

#endif // _TEXTURE_HANDLER_HH_
