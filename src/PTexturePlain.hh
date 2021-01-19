//
// PTexturePlain.hh for pekwm
// Copyright (C) 2004-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PTEXTURE_PLAIN_HH_
#define _PTEXTURE_PLAIN_HH_

#include "config.h"

#include <string>

#include "pekwm.hh"
#include "PTexture.hh"

class PImage;

// PTextureEmpty

class PTextureEmpty : public PTexture {
public:
    PTextureEmpty() { }
    virtual ~PTextureEmpty() { }

    // START - PTexture interface.
    virtual void render(Drawable draw,
                        int x, int y, uint width, uint height) override {
    }
    // END - PTexture interface.
};

// PTextureSolid

class PTextureSolid : public PTexture {
public:
    PTextureSolid(const std::string &color);
    virtual ~PTextureSolid();

    // START - PTexture interface.
    virtual void render(Drawable draw,
                        int x, int y, uint width, uint height) override;
    // END - PTexture interface.

    inline XColor *getColor() { return _xc; }
    bool setColor(const std::string &color);
    void unsetColor();

private:
    GC _gc;
    XColor *_xc;
};

// PTextureSolidRaised

class PTextureSolidRaised : public PTexture {
public:
    PTextureSolidRaised(const std::string &base,
                        const std::string &hi, const std::string &lo);
    virtual ~PTextureSolidRaised();

    // START - PTexture interface.
    virtual void render(Drawable draw,
                        int x, int y, uint width, uint height) override;
    // END - PTexture interface.

    inline void setLineOff(uint loff) { _loff = loff; _loff2 = loff * 2; }
    inline void setDraw(bool top, bool bottom, bool left, bool right) {
        _draw_top = top;
        _draw_bottom = bottom;
        _draw_left = left;
        _draw_right = right;
    }

    void setLineWidth(uint lw);
    bool setColor(const std::string &base,
                  const std::string &hi, const std::string &lo);
    void unsetColor();

private:
    GC _gc;

    XColor *_xc_base;
    XColor *_xc_hi;
    XColor *_xc_lo;

    uint _lw, _loff, _loff2;
    bool _draw_top;
    bool _draw_bottom;
    bool _draw_left;
    bool _draw_right;
};

// PTxtureLines

class PTextureLines : public PTexture {
public:
    PTextureLines(float line_size, bool size_percent, bool horz,
                  const std::vector<std::string> &colors);
    virtual ~PTextureLines();

    // START - PTexture interface.
    virtual void render(Drawable draw,
                        int x, int y, uint width, uint height) override;
    // END - PTexture interface.

private:
    void renderHorz(Drawable draw, int x, int y, uint width, uint height);
    void renderVert(Drawable draw, int x, int y, uint width, uint height);

    void setColors(const std::vector<std::string> &colors);
    void unsetColors();

private:
    GC _gc;
    /** Line width/height, given in percent (0-100) if _size_percent is true */
    float _line_size;
    /** If true, size given in percent instead of pixels. */
    bool _size_percent;
    /** If false, vertical mode. */
    bool _horz;
    /** Line colours. */
    std::vector<XColor*> _colors;
};

// PTextureImage

class PTextureImage : public PTexture {
public:
    PTextureImage();
    PTextureImage(const std::string &image);
    virtual ~PTextureImage();

    // START - PTexture interface.
    virtual void render(Drawable draw,
                        int x, int y, uint width, uint height) override;
    virtual Pixmap getMask(uint width, uint height, bool &do_free) override;
    // END - PTexture interface.

    bool setImage(const std::string &image);
    void setImage(PImage *image);
    void unsetImage();

private:
    PImage *_image;
};

#endif // _PTEXTURE_PLAIN_HH_
