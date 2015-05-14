//
// PTexture.cc for pekwm
// Copyright (C) 2004-2009 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifndef _PTEXTURE_HH_
#define _PTEXTURE_HH_

#include "pekwm.hh"

class PTexture {
public:
    enum Type {
        TYPE_SOLID, TYPE_SOLID_RAISED,
        TYPE_IMAGE, TYPE_EMPTY, TYPE_NO
    };

    PTexture() : _ok(false), _width(0), _height(0), _type(PTexture::TYPE_NO) { }
    virtual ~PTexture(void) { }

    virtual void render(Drawable draw, int x, int y, uint width, uint height) { }
    virtual Pixmap getMask(uint width, uint height, bool &do_free) { return None; }

    inline bool isOk(void) const { return _ok; }
    inline uint getWidth(void) const { return _width; }
    inline uint getHeight(void) const { return _height; }
    inline PTexture::Type getType(void) const { return _type; }

    inline void setWidth(uint width) { _width = width; }
    inline void setHeight(uint height) { _height = height; }

protected:
    bool _ok; // Texture successfully loaded
    uint _width, _height; // for images etc, 0 for infinite like in stretch
    PTexture::Type _type; // Type of texture
};

#endif // _PTEXTURE_HH_
