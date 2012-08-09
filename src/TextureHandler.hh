//
// TextureHandler.hh for pekwm
// Copyright © 2005 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _TEXTURE_HANDLER_HH_
#define _TEXTURE_HANDLER_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

    static TextureHandler *instance(void) { return _instance; }
    static int getLengthMin(void) { return LENGTH_MIN; }

    PTexture *getTexture(const std::string &texture);
    PTexture *referenceTexture(PTexture *texture);
    void returnTexture(PTexture *texture);

private:
    PTexture *parse(const std::string &texture);
    PTexture *parseSolid(std::vector<std::string> &tok);
    PTexture *parseSolidRaised(std::vector<std::string> &tok);

    void parseSize(PTexture *tex, const std::string &size);

private:
    static TextureHandler *_instance;
    static std::map<ParseUtil::Entry, PTexture::Type> _parse_map;
    static const int LENGTH_MIN; //!< Minimum texture name length.

    std::vector<TextureHandler::Entry*> _textures;
};

#endif // _TEXTURE_HANDLER_HH_
