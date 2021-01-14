//
// PTexturePlain.cc for pekwm
// Copyright (C) 2004-2020 Claes Nästén <pekdon@gmail.com>
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

// PTextureSolid

//! @brief PTextureSolid constructor
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

//! @brief PTextureSolid destructor
PTextureSolid::~PTextureSolid(void)
{
    XFreeGC(X11::getDpy(), _gc);

    unsetColor();
}

// BEGIN - PTexture interface.

//! @brief Renders texture on drawable draw
void
PTextureSolid::render(Drawable draw, int x, int y, uint width, uint height)
{
    if (width == 0) {
        width = _width;
    }
    if (height == 0) {
        height = _height;
    }

    XFillRectangle(X11::getDpy(), draw, _gc, x, y, width, height);
}

// END - PTexture interface.

//! @brief Loads color resources
bool
PTextureSolid::setColor(const std::string &color)
{
    unsetColor(); // unload used resources

    _xc = X11::getColor(color);
    XSetForeground(X11::getDpy(), _gc, _xc->pixel);

    _ok = true;

    return _ok;
}

//! @brief Frees color resources used by texture
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

//! @brief PTextureSolidRaised constructor
PTextureSolidRaised::PTextureSolidRaised(const std::string &base, const std::string &hi, const std::string &lo)
    : PTexture(),
      _xc_base(0), _xc_hi(0), _xc_lo(0),
      _lw(1), _loff(0), _loff2(0),
      _draw_top(true), _draw_bottom(true),
      _draw_left(true), _draw_right(true)
{
    // PTexture attributes
    _type = PTexture::TYPE_SOLID_RAISED;

    XGCValues gv;
    gv.function = GXcopy;
    gv.line_width = _lw;
    _gc = XCreateGC(X11::getDpy(), X11::getRoot(), GCFunction|GCLineWidth, &gv);

    setColor(base, hi, lo);
}

//! @brief PTextureSolidRasied destructor
PTextureSolidRaised::~PTextureSolidRaised(void)
{
    XFreeGC(X11::getDpy(), _gc);

    unsetColor();
}

// START - PTexture interface.

//! @brief Renders texture on drawable draw
void
PTextureSolidRaised::render(Drawable draw, int x, int y, uint width, uint height)
{
    if (width == 0) {
        width = _width;
    }
    if (height == 0) {
        height = _height;
    }

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

//! @brief Sets line width
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

//! @brief Loads color resources
bool
PTextureSolidRaised::setColor(const std::string &base, const std::string &hi, const std::string &lo)
{
    unsetColor(); // unload used resources

    _xc_base = X11::getColor(base);
    _xc_hi = X11::getColor(hi);
    _xc_lo = X11::getColor(lo);

    _ok = true;

    return _ok;
}

//! @brief Unloads color resources
void
PTextureSolidRaised::unsetColor(void)
{
    _ok = false;

    X11::returnColor(_xc_base);
    X11::returnColor(_xc_hi);
    X11::returnColor(_xc_lo);

    _xc_base = _xc_hi = _xc_lo = 0;
}

// PTextureImage

//! @brief PTextureImage constructor
PTextureImage::PTextureImage(void)
  : PTexture(),
    _image(0)
{
  // PTexture attributes
  _type = PTexture::TYPE_IMAGE;
}
                                             

//! @brief PTextureImage constructor
PTextureImage::PTextureImage(const std::string &image)
    : PTexture(),
      _image(0)
{
    // PTexture attributes
    _type = PTexture::TYPE_IMAGE;

    setImage(image);
}

//! @brief PTextureImage destructor
PTextureImage::~PTextureImage(void)
{
    unsetImage();
}


//! @brief Renders texture on drawable draw
void
PTextureImage::render(Drawable draw, int x, int y, uint width, uint height)
{
    _image->draw(draw, x, y, width, height);
}

Pixmap
PTextureImage::getMask(uint width, uint height, bool &do_free)
{
    return _image->getMask(do_free, width, height);
}

//! @brief Loads image resources
void
PTextureImage::setImage(PImage *image)
{
    unsetImage();
    _image = image;
    _width = _image->getWidth();
    _height = _image->getHeight();
    _ok = true;
}

//! @brief Loads image resources
bool
PTextureImage::setImage(const std::string &image)
{
    unsetImage();
    _image = ImageHandler::instance()->getImage(image);
    if (_image) {
        _width = _image->getWidth();
        _height = _image->getHeight();
        _ok = true;
    } else {
        _ok = false;
    }
    return _ok;
}

//! @brief Unloads image resources
void
PTextureImage::unsetImage(void)
{
    ImageHandler::instance()->returnImage(_image);
    _image = 0;
    _width = 1;
    _height = 1;
    _ok = false;
}

