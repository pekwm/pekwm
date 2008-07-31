//
// PTexturePlain.cc for pekwm
// Copyright © 2004-2007 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PTexture.hh"
#include "PTexturePlain.hh"
#include "ColorHandler.hh"
#include "PImage.hh"
#include "ImageHandler.hh"

using std::string;

// PTextureSolid

//! @brief PTextureSolid constructor
PTextureSolid::PTextureSolid(Display *dpy, const std::string &color) : PTexture(dpy),
        _xc(NULL)
{
    // PTexture attributes
    _type = PTexture::TYPE_SOLID;

    XGCValues gv;
    gv.function = GXcopy;
    _gc = XCreateGC(_dpy, RootWindow(_dpy, DefaultScreen(_dpy)), GCFunction, &gv);

    setColor(color);
}

//! @brief PTextureSolid destructor
PTextureSolid::~PTextureSolid(void)
{
    XFreeGC(_dpy, _gc);

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

    XFillRectangle(_dpy, draw, _gc, x, y, width, height);
}

// END - PTexture interface.

//! @brief Loads color resources
bool
PTextureSolid::setColor(const std::string &color)
{
    unsetColor(); // unload resources if any usedA

    _xc = ColorHandler::instance()->getColor(color);
    XSetForeground(_dpy, _gc, _xc->pixel);

    _ok = true;

    return _ok;
}

//! @brief Frees color resources used by texture
void
PTextureSolid::unsetColor(void)
{
    if (_xc) {
        ColorHandler::instance()->returnColor(_xc);

        _xc = NULL;
        _ok = false;
    }
}

// PTextureSolidRaised

//! @brief PTextureSolidRaised constructor
PTextureSolidRaised::PTextureSolidRaised(Display *dpy, const std::string &base, const std::string &hi, const std::string &lo) : PTexture(dpy),
        _xc_base(NULL), _xc_hi(NULL), _xc_lo(NULL),
        _lw(1), _loff(0), _loff2(0),
        _draw_top(true), _draw_bottom(true),
        _draw_left(true), _draw_right(true)
{
    // PTexture attributes
    _type = PTexture::TYPE_SOLID_RAISED;

    XGCValues gv;
    gv.function = GXcopy;
    gv.line_width = _lw;
    _gc = XCreateGC(_dpy, RootWindow(_dpy, DefaultScreen(_dpy)),
                    GCFunction|GCLineWidth, &gv);

    setColor(base, hi, lo);
}

//! @brief PTextureSolidRasied destructor
PTextureSolidRaised::~PTextureSolidRaised(void)
{
    XFreeGC(_dpy, _gc);

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
    XSetForeground(_dpy, _gc, _xc_base->pixel);
    XFillRectangle(_dpy, draw, _gc, x, y, width, height);

    // hi line ( consisting of two lines )
    XSetForeground(_dpy, _gc, _xc_hi->pixel);
    if (_draw_top) {
        XDrawLine(_dpy, draw, _gc,
                  x + _loff, y + _loff, x + width - _loff - _lw, y + _loff);
    }
    if (_draw_left) {
        XDrawLine(_dpy, draw, _gc,
                  x + _loff, y + _loff, x + _loff, y + height - _loff - _lw);
    }

    // lo line ( consisting of two lines )
    XSetForeground(_dpy, _gc, _xc_lo->pixel);
    if (_draw_bottom) {
        XDrawLine(_dpy, draw, _gc,
                  x + _loff + _lw,
                  y + height - _loff - (_lw ? _lw : 1),
                  x + width - _loff - _lw,
                  y + height - _loff - (_lw ? _lw : 1));
    }
    if (_draw_right) {
        XDrawLine(_dpy, draw, _gc,
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
    XChangeGC(_dpy, _gc, GCLineWidth, &gv);
}

//! @brief Loads color resources
bool
PTextureSolidRaised::setColor(const std::string &base, const std::string &hi, const std::string &lo)
{
    unsetColor(); // unload resources if any usedA

    _xc_base = ColorHandler::instance()->getColor(base);
    _xc_hi = ColorHandler::instance()->getColor(hi);
    _xc_lo = ColorHandler::instance()->getColor(lo);

    _ok = true;

    return _ok;
}

//! @brief Unloads color resources
void
PTextureSolidRaised::unsetColor(void)
{
    _ok = false;

    ColorHandler::instance()->returnColor(_xc_base);
    ColorHandler::instance()->returnColor(_xc_hi);
    ColorHandler::instance()->returnColor(_xc_lo);

    _xc_base = _xc_hi = _xc_lo = NULL;
}

// PTextureImage

//! @brief PTextureImage constructor
PTextureImage::PTextureImage(Display *dpy)
  : PTexture(dpy),
    _image(NULL)
{
  // PTexture attributes
  _type = PTexture::TYPE_IMAGE;
}
                                             

//! @brief PTextureImage constructor
PTextureImage::PTextureImage(Display *dpy, const std::string &image) : PTexture(dpy),
        _image(NULL)
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

//! @brief
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
    _width = _image->getWidth();
    _height = _image->getHeight();
    _ok = true;
    return _ok;
}

//! @brief Unloads image resources
void
PTextureImage::unsetImage(void)
{
    ImageHandler::instance()->returnImage(_image);
    _image = NULL;
    _width = 1;
    _height = 1;
    _ok = false;
}

