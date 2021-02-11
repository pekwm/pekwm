//
// PTexturePlain.cc for pekwm
// Copyright (C) 2004-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "PTexture.hh"
#include "PTexturePlain.hh"
#include "PImage.hh"
#include "ImageHandler.hh"
#include "x11.hh"

#include <iostream>

extern "C" {
#include <assert.h>
}

// PTextureEmpty

void
PTextureEmpty::doRender(Drawable draw,
                      int x, int y, uint width, uint height)
{
}

bool
PTextureEmpty::getPixel(ulong &pixel) const
{
    pixel = X11::getWhitePixel();
    return true;
}

// PTextureSolid

PTextureSolid::PTextureSolid(const std::string &color)
    : PTexture(),
      _xc(0)
{
    // PTexture attributes
    _type = PTexture::TYPE_SOLID;

    XGCValues gv;
    gv.function = GXcopy;
    _gc = XCreateGC(X11::getDpy(), X11::getRoot(), GCFunction, &gv);

    setColor(color);
}

PTextureSolid::~PTextureSolid(void)
{
    XFreeGC(X11::getDpy(), _gc);

    unsetColor();
}

// BEGIN - PTexture interface.

/**
 * Render single color on draw.
 */
void
PTextureSolid::doRender(Drawable draw, int x, int y, uint width, uint height)
{
    XFillRectangle(X11::getDpy(), draw, _gc, x, y, width, height);
}

// END - PTexture interface.

/**
 * Load color resources
 */
bool
PTextureSolid::setColor(const std::string &color)
{
    unsetColor(); // unload used resources

    _xc = X11::getColor(color);
    XSetForeground(X11::getDpy(), _gc, _xc->pixel);

    _ok = true;

    return _ok;
}

/**
 * Frees color resources used by texture
 */
void
PTextureSolid::unsetColor(void)
{
    if (_xc) {
        X11::returnColor(_xc);

        _xc = 0;
        _ok = false;
    }
}

// PTextureSolidRaised

PTextureSolidRaised::PTextureSolidRaised(const std::string &base,
                                         const std::string &hi,
                                         const std::string &lo)
    : PTexture(),
      _xc_base(0),
      _xc_hi(0),
      _xc_lo(0),
      _lw(1),
      _loff(0),
      _loff2(0),
      _draw_top(true),
      _draw_bottom(true),
      _draw_left(true),
      _draw_right(true)
{
    // PTexture attributes
    _type = PTexture::TYPE_SOLID_RAISED;

    XGCValues gv;
    gv.function = GXcopy;
    gv.line_width = _lw;
    _gc = XCreateGC(X11::getDpy(), X11::getRoot(), GCFunction|GCLineWidth, &gv);

    setColor(base, hi, lo);
}

PTextureSolidRaised::~PTextureSolidRaised(void)
{
    XFreeGC(X11::getDpy(), _gc);

    unsetColor();
}

// START - PTexture interface.

/**
 * Renders a "raised" rectangle onto draw.
 */
void
PTextureSolidRaised::doRender(Drawable draw,
                              int x, int y, uint width, uint height)
{
    // base rectangle
    XSetForeground(X11::getDpy(), _gc, _xc_base->pixel);
    XFillRectangle(X11::getDpy(), draw, _gc, x, y, width, height);

    // hi line ( consisting of two lines )
    XSetForeground(X11::getDpy(), _gc, _xc_hi->pixel);
    if (_draw_top) {
        XDrawLine(X11::getDpy(), draw, _gc,
                  x + _loff, y + _loff, x + width - _loff - _lw, y + _loff);
    }
    if (_draw_left) {
        XDrawLine(X11::getDpy(), draw, _gc,
                  x + _loff, y + _loff, x + _loff, y + height - _loff - _lw);
    }

    // lo line ( consisting of two lines )
    XSetForeground(X11::getDpy(), _gc, _xc_lo->pixel);
    if (_draw_bottom) {
        XDrawLine(X11::getDpy(), draw, _gc,
                  x + _loff + _lw,
                  y + height - _loff - (_lw ? _lw : 1),
                  x + width - _loff - _lw,
                  y + height - _loff - (_lw ? _lw : 1));
    }
    if (_draw_right) {
        XDrawLine(X11::getDpy(), draw, _gc,
                  x + width - _loff - (_lw ? _lw : 1),
                  y + _loff + _lw,
                  x + width - _loff - (_lw ? _lw : 1),
                  y + height - _loff - _lw);
    }
}

// END - PTexture interface.

/**
 * Sets line width
 */
void
PTextureSolidRaised::setLineWidth(uint lw)
{
    _lw = lw;
    // This is a hack to be able to rid the spacing lw == 1 does.
    if (! lw) {
        lw = 1;
    }

    XGCValues gv;
    gv.line_width = lw;
    XChangeGC(X11::getDpy(), _gc, GCLineWidth, &gv);
}

/**
 * Loads color resources
 */
bool
PTextureSolidRaised::setColor(const std::string &base,
                              const std::string &hi, const std::string &lo)
{
    unsetColor(); // unload used resources

    _xc_base = X11::getColor(base);
    _xc_hi = X11::getColor(hi);
    _xc_lo = X11::getColor(lo);

    _ok = true;

    return _ok;
}

/**
 * Free color resources
 */
void
PTextureSolidRaised::unsetColor(void)
{
    _ok = false;

    X11::returnColor(_xc_base);
    X11::returnColor(_xc_hi);
    X11::returnColor(_xc_lo);

    _xc_base = _xc_hi = _xc_lo = 0;
}

// PTextureLines

PTextureLines::PTextureLines(float line_size, bool size_percent, bool horz,
                             const std::vector<std::string> &colors)
    : _line_size(line_size),
      _size_percent(size_percent),
      _horz(horz)
{
    _type = horz ? PTexture::TYPE_LINES_HORZ : PTexture::TYPE_LINES_VERT;
    XGCValues gv;
    gv.function = GXcopy;
    _gc = XCreateGC(X11::getDpy(), X11::getRoot(), GCFunction, &gv);
    setColors(colors);
}

PTextureLines::~PTextureLines()
{
    XFreeGC(X11::getDpy(), _gc);
}

void
PTextureLines::doRender(Drawable draw, int x, int y, uint width, uint height)
{
    if (_horz) {
        renderHorz(draw, x, y, width, height);
    } else {
        renderVert(draw, x, y, width, height);
    }
}

void
PTextureLines::renderHorz(Drawable draw, int x, int y, uint width, uint height)
{
    uint line_height;
    if (_size_percent) {
        line_height = static_cast<float>(height) * _line_size;
    } else {
        line_height = _line_size;
    }

    // ensure code does not get stuck never increasing pos
    if (line_height < 1) {
        line_height = 1;
    }

    uint pos = 0;
    while (pos < height) {
        for (auto it : _colors) {
            XSetForeground(X11::getDpy(), _gc, it->pixel);
            XFillRectangle(X11::getDpy(), draw, _gc,
                           x, y + pos,
                           width,
                           std::min(line_height, height - pos));
            pos += line_height;
        }
    }
}

void
PTextureLines::renderVert(Drawable draw, int x, int y, uint width, uint height)
{
    uint line_width;
    if (_size_percent) {
        line_width = static_cast<float>(width) * _line_size;
    } else {
        line_width = _line_size;
    }

    uint pos = 0;
    while (pos < width) {
        for (auto it : _colors) {
            XSetForeground(X11::getDpy(), _gc, it->pixel);
            XFillRectangle(X11::getDpy(), draw, _gc,
                           x + pos, y,
                           std::min(line_width, width - pos),
                           height);
            pos += line_width;
        }
    }
}

void
PTextureLines::setColors(const std::vector<std::string> &colors)
{
    unsetColors();

    for (auto it : colors) {
        _colors.push_back(X11::getColor(it));
    }
    _ok = ! _colors.empty();
}

void
PTextureLines::unsetColors()
{
    for (auto it : _colors) {
        X11::returnColor(it);
    }
    _colors.clear();
}

// PTextureImage

PTextureImage::PTextureImage(PImage *image)
    : _image(nullptr)
{
    // PTexture attributes
    _type = PTexture::TYPE_IMAGE;
    setImage(image);
}

PTextureImage::PTextureImage(const std::string &image,
                             const std::string &colormap)
    : _image(nullptr),
      _colormap(colormap)
{
    // PTexture attributes
    _type = colormap.empty()
        ? PTexture::TYPE_IMAGE : PTexture::TYPE_IMAGE_MAPPED;
    setImage(image, colormap);
}

PTextureImage::~PTextureImage(void)
{
    unsetImage();
}

/**
 * Renders image onto draw
 */
void
PTextureImage::doRender(Drawable draw, int x, int y, uint width, uint height)
{
    _image->draw(draw, x, y, width, height);
}

Pixmap
PTextureImage::getMask(uint width, uint height, bool &do_free)
{
    return _image->getMask(do_free, width, height);
}

/**
 * Set image resource
 */
void
PTextureImage::setImage(PImage *image)
{
    unsetImage();
    _image = image;
    _colormap.clear();
    _width = _image->getWidth();
    _height = _image->getHeight();
    pekwm::imageHandler()->takeOwnership(image);
    _ok = true;
}

/**
 * Load image resource
 */
bool
PTextureImage::setImage(const std::string &image, const std::string &colormap)
{
    unsetImage();

    if (colormap.empty()) {
        _image = pekwm::imageHandler()->getImage(image);
    } else {
        _image = pekwm::imageHandler()->getMappedImage(image, colormap);
    }

    assert(_image->getData());

    if (_image) {
        _colormap = colormap;
        _width = _image->getWidth();
        _height = _image->getHeight();
        _ok = true;
    } else {
        _ok = false;
    }
    return _ok;
}

/**
 * Free image resource
 */
void
PTextureImage::unsetImage(void)
{
    if (_ok) {
        if (_colormap.empty()) {
            pekwm::imageHandler()->returnImage(_image);
        } else {
            pekwm::imageHandler()->returnMappedImage(_image, _colormap);
        }
    }
    _image = nullptr;
    _colormap.clear();
    _width = 0;
    _height = 0;
    _ok = false;
}
