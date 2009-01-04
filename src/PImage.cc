//
// PImage.cc for pekwm
// Copyright © 2005-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PImage.hh"
#include "PScreen.hh"
#include "ScreenResources.hh"
#include "PixmapHandler.hh"
#include "Util.hh"

#include <iostream>

extern "C" {
#include <X11/Xutil.h>
}

using std::cerr;
using std::endl;
using std::list;
using std::string;

list<PImageLoader*> PImage::_loader_list = list<PImageLoader*>();

/**
 * PImage constructor, loads image if one is specified.
 *
 * @param dpy Display image is valid on.
 * @param path Path to image file, if specified this is loaded.
 */
PImage::PImage(Display *dpy, const std::string &path) throw(LoadException&)
    : _dpy(dpy), _type(IMAGE_TYPE_NO), _pixmap(None), _mask(None), _width(0), _height(0),
      _data(0), _has_alpha(false), _use_alpha(false)
{
    if (path.size()) {
        if (! load(path)) {
            throw LoadException(path.c_str());
        }
    }
}

//! @brief PImage destructor.
PImage::~PImage(void)
{
    unload();
}

//! @brief Loads image from file.
//! @param file File to load.
//! @return Returns true on success, else false.
bool
PImage::load(const std::string &file)
{
    string ext(Util::getFileExt(file));
    if (! ext.size()) {
        cerr << " *** WARNING: no file extension on " << file << "!" << endl;
        return false;
    }

    list<PImageLoader*>::iterator it(_loader_list.begin());
    for (; it != _loader_list.end(); ++it) {
        if (! strcasecmp((*it)->getExt(), ext.c_str())) {
            _data = (*it)->load(file, _width, _height, _has_alpha, _use_alpha);
            if (_data) {
                _pixmap = createPixmap(_data, _width, _height);
                _mask = createMask(_data, _width, _height);
                break;
            }
        }
    }

    return (_data);
}

//! @brief Frees resources used by image.
void
PImage::unload(void)
{
    if (_data) {
        delete [] _data;
        _data = 0;
    }
    if (_pixmap) {
        ScreenResources::instance()->getPixmapHandler()->returnPixmap(_pixmap);
    }
    if (_mask) {
        ScreenResources::instance()->getPixmapHandler()->returnPixmap(_mask);
    }

    _pixmap = None;
    _mask = None;
    _width = 0;
    _height = 0;
}

//! @brief Draws image on drawable.
//! @param draw Drawable to draw on.
//! @param x Destination x coordinate.
//! @param y Destination y coordinate.
//! @param width Destination width, defaults to 0 which expands to image size.
//! @param height Destination height, defaults to 0 which expands to image size.
void
PImage::draw(Drawable draw, int x, int y, uint width, uint height)
{
    if (! _data) {
        return;
    }

    // Expand variables.
    if (! width) {
        width = _width;
    } if (! height) {
        height = _height;
    }

    // Draw image, select correct drawing method depending on image type,
    // size and if alpha exists.
    if ((_type == IMAGE_TYPE_FIXED)
        || ((_type == IMAGE_TYPE_SCALED)
            && (_width == width) && (_height == height))) {
        if (_use_alpha) {
            drawAlphaFixed(draw, x, y, width, height);
        } else {
            drawFixed(draw, x, y, width, height);
        }
    } else if (_type == IMAGE_TYPE_SCALED) {
        if (_use_alpha) {
            drawAlphaScaled(draw, x, y, width, height);
        } else {
            drawScaled(draw, x, y, width, height);
        }
    } else if (_type == IMAGE_TYPE_TILED) {
        if (_use_alpha) {
            drawAlphaTiled(draw, x, y, width, height);
        } else {
            drawTiled(draw, x, y, width, height);
        }
    }
}

//! @brief Returns pixmap at sizen.
//! @param need_free Is set to wheter shape mask returned needs to be freed.
//! @param width Pixmap width, defaults to 0 which expands to image size.
//! @param height Pixmap height, defaults to 0 which expands to image size.
//! @return Returns pixmap at size or None on error.
Pixmap
PImage::getPixmap(bool &need_free, uint width, uint height)
{
    // Default
    Pixmap pix = None;
    need_free = false;

    // Expand parameters.
    if (! width) {
        width = _width;
    }
    
    if (! height) {
        height = _height;
    }
    
    // Same size, return _pixmap.
    if ((width == _width) && (height == _height)) {
        pix = _pixmap;
    } else {
        uchar *scaled_data;

        scaled_data = getScaledData(width, height);
        if (scaled_data) {
            need_free = true;
            pix = createPixmap(scaled_data, width, height);
            delete [] scaled_data;
        }
    }

    return pix;
}

//! @brief Returns shape mask at size, if any.
//! @param need_free Is set to wheter shape mask returned needs to be freed.
//! @param width Shape mask width, defaults to 0 which expands to image size.
//! @param height Shape mask height, defaults to 0 which expands to image size.
//! @return Returns shape mask at size or None if there is no mask information.
Pixmap
PImage::getMask(bool &need_free, uint width, uint height)
{
    // Default
    Pixmap pix = None;
    need_free = false;

    // Expand parameters.
    if (! width) {
        width = _width;
    }
    
    if (! height) {
        height = _height;
    }
    
    // Same size, return _mask.
    if ((width == _width) && (height == _height)) {
        pix = _mask;
    } else {
        uchar *scaled_data;

        scaled_data = getScaledData(width, height);
        if (scaled_data) {
            need_free = true;
            pix = createMask(scaled_data, width, height);
            delete [] scaled_data;
        }
    }

    return pix;
}

//! @brief Scales image to size.
//! @param width Width to scale image to.
//! @param height Height to scale image to.
void
PImage::scale(uint width, uint height)
{
    // Invalid width or height or no need to scale.
    if (! width || ! height || ((width == _width) && (height == height))) {
        return;
    }
    uchar *scaled_data;

    scaled_data = getScaledData(width, height);
    if (scaled_data) {
        // Free old resources.
        unload();

        // Set data pointer and create pixmap and mask at new size.
        _data = scaled_data;
        _pixmap = createPixmap(_data, width, height);
        _mask = createMask(_data, width, height);
        _width = width;
        _height = height;
    }
}

//! @brief Draw image at position, not scaling.
void
PImage::drawFixed(Drawable dest, int x, int y, uint width, uint height)
{
    // Plain copy of the pixmap onto Drawable.
    XCopyArea(_dpy, _pixmap, dest, PScreen::instance()->getGC(),
              0, 0, width, height, x, y);

}

//! @brief Draw image scaled to fit width and height.
void
PImage::drawScaled(Drawable dest, int x, int y, uint width, uint height)
{
    uchar *scaled_data;
    // Create scaled representation of image.
    scaled_data = getScaledData(width, height);
    if (scaled_data) {
        Pixmap pix;
        // Create pixmap.
        pix = createPixmap(scaled_data, width, height);
        if (pix) {
            XCopyArea(_dpy, pix, dest, PScreen::instance()->getGC(),
                      0, 0, width, height, x, y);
            ScreenResources::instance()->getPixmapHandler()->returnPixmap(pix);
        }

        delete [] scaled_data;
    }
}

//! @brief Draw image tiled to fit width and height.
void
PImage::drawTiled(Drawable dest, int x, int y, uint width, uint height)
{
    // Create a GC with _pixmap as tile and tiled fill style.
    GC gc;
    XGCValues gv;

    gv.fill_style = FillTiled;
    gv.tile = _pixmap;
    gv.ts_x_origin = x;
    gv.ts_y_origin = y;

    gc = XCreateGC(_dpy , dest,
                   GCFillStyle|GCTile|GCTileStipXOrigin|GCTileStipYOrigin, &gv);

    // Tile the image onto drawable.
    XFillRectangle(_dpy, dest, gc, x, y, width, height);

    XFreeGC(_dpy, gc);
}

//! @brief Draw image at position, not scaling.
void
PImage::drawAlphaFixed(Drawable dest, int x, int y, uint width, uint height, uchar *data)
{
    XImage *dest_image = XGetImage(_dpy, dest, x, y, width, height, AllPlanes, ZPixmap);
    if (! dest_image) {
        cerr << " *** ERROR: failed to get image for destination." << endl;
        return;
    }

    // Get mask from visual
    Visual *visual = PScreen::instance()->getVisual()->getXVisual();
    dest_image->red_mask = visual->red_mask;
    dest_image->green_mask = visual->green_mask;
    dest_image->blue_mask = visual->blue_mask;

    uchar *src;
    uchar r, g, b, a;
    uchar d_r = 0, d_g = 0, d_b = 0;
    float a_percent, a_percent_inv;

    if (data) {
        src = data;
    } else {
        src = _data;
        width = std::min(width, _width);
        height = std::min(height, _height);
    }

    for (uint i_y = 0; i_y < height; ++i_y) {
        for (uint i_x = 0; i_x < width; ++i_x) {
            // Get pixel value, copy them directly if alpha is set to 255.
            r = *src++;
            g = *src++;
            b = *src++;
            a = *src++;

            // Alpha not 100% solid, blend
            if (a != 255) {
                // Get RGB values from pixel.
                getRgbFromPixel(dest_image, XGetPixel(dest_image, i_x, i_y),
                                d_r, d_g, d_b);

                a_percent = static_cast<float>(a) / 255;
                a_percent_inv = 1 - a_percent;

                r = (a_percent_inv * d_r) + (a_percent * r);
                g = (a_percent_inv * d_g) + (a_percent * g);
                b = (a_percent_inv * d_b) + (a_percent * b);
            }

            XPutPixel(dest_image, i_x, i_y, getPixelFromRgb(dest_image, r, g, b));
        }
    }

    XPutImage(_dpy, dest, PScreen::instance()->getGC(), dest_image,
              0, 0, x, y, width, height);
    XDestroyImage(dest_image);
}

//! @brief Draw image scaled to fit width and height.
void
PImage::drawAlphaScaled(Drawable dest, int x, int y, uint width, uint height)
{
    uchar *scaled_data = getScaledData(width, height);
    if (scaled_data) {
        drawAlphaFixed(dest, x, y, width, height, scaled_data);
        delete [] scaled_data;
    }
}

//! @brief Draw image tiled to fit width and height.
void
PImage::drawAlphaTiled(Drawable dest, int x, int y, uint width, uint height)
{
    // FIXME: Implement tiled rendering with alpha support
    drawTiled(dest, x, y, width, height);
}

//! @brief Creates Pixmap from data.
//! @param data Pointer to data to create pixmap from.
//! @param width Width of image data is representing.
//! @param height Height of image data is representing.
//! @return Returns Pixmap on success, else None.
Pixmap
PImage::createPixmap(uchar *data, uint width, uint height)
{
    XImage *ximage;
    Pixmap pix = None;

    ximage = createXImage(data, width, height);
    if (ximage) {
        pix = ScreenResources::instance()->getPixmapHandler()->getPixmap(width,
                height, PScreen::instance()->getDepth());

        XPutImage(_dpy, pix, PScreen::instance()->getGC(), ximage,
                  0, 0, 0, 0, width, height);

        delete [] ximage->data;
        ximage->data = 0;
        XDestroyImage(ximage);
    }

    return pix;
}

//! @brief Creates shape mask Pixmap from data.
//! @param data Pointer to data to create mask from.
//! @param width Width of image data is representing.
//! @param height Height of image data is representing.
//! @return Returns Pixmap mask on success, else None.
Pixmap
PImage::createMask(uchar *data, uint width, uint height)
{
    if (! _has_alpha) {
        return None;
    }

    // Create XImage
    XImage *ximage;
    ximage = XCreateImage(_dpy, PScreen::instance()->getVisual()->getXVisual(),
                          1, ZPixmap, 0, 0, width, height, 32, 0);
    if (! ximage) {
        cerr << " *** WARNING: unable to create XImage!" << endl;
        return None;
    }

    // Alocate ximage data storage.
    ximage->data = new char[ximage->bytes_per_line * height / sizeof(char)];

    uchar *src = data + 3; // Skip R, G and B.
    ulong pixel_trans, pixel_solid;

    pixel_trans = PScreen::instance()->getBlackPixel();
    pixel_solid = PScreen::instance()->getWhitePixel();

    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            XPutPixel(ximage, x, y, (*src > 127) ? pixel_solid : pixel_trans);
            src += 4; // Skip R, G, B and A
        }
    }

    // Create Pixmap
    Pixmap pix;
    pix = ScreenResources::instance()->getPixmapHandler()->getPixmap(width, height, 1);

    GC gc = XCreateGC(_dpy, pix, 0, 0);
    XPutImage(_dpy, pix, gc, ximage, 0, 0, 0, 0, width, height);
    XFreeGC(_dpy, gc);

    delete [] ximage->data;
    ximage->data = 0;
    XDestroyImage(ximage);

    return pix;
}

//! @brief Createx XImage from data.
//! @param data Pointer to data to create XImage from.
//! @param width Width of image data is representing.
//! @param height Height of image data is representing.
XImage*
PImage::createXImage(uchar *data, uint width, uint height)
{
    // Create XImage
    XImage *ximage;
    ximage = XCreateImage(_dpy, PScreen::instance()->getVisual()->getXVisual(),
                          PScreen::instance()->getDepth(), ZPixmap, 0, 0,
                          width, height, 32, 0);
    if (! ximage) {
        cerr << " *** WARNING: unable to create XImage!" << endl;
        return 0;
    }

    // Allocate ximage data storage.
    ximage->data = new char[ximage->bytes_per_line * height / sizeof(char)];

    uchar *src = data;
    uchar r, g, b;

    // Put data into XImage.
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            r = *src++;
            g = *src++;
            b = *src++;
            if (_has_alpha) {
                *src++;
            }
            
            XPutPixel(ximage, x, y, getPixelFromRgb(ximage, r, g, b));
        }
    }

    return ximage;
}

//! @brief Scales image data and returns pointer to new data.
//! @param width Width of image data to return.
//! @param height Height of image data to return.
//! @return Pointer to image data on success, else 0.
//! @todo Implement decent scaling routine with data [] cache?
uchar*
PImage::getScaledData(uint width, uint height)
{
    if (! width || ! height) {
        return 0;
    }
    
    // Calculate aspect ratio.
    float ratio_x, ratio_y;
    ratio_x = static_cast<float>(_width) / static_cast<float>(width);
    ratio_y = static_cast<float>(_height) / static_cast<float>(height);

    // Allocate memory.
    uchar *scaled_data, *dest;
    scaled_data = new uchar[width * height * (_has_alpha ? 4 : 3)];
    dest = scaled_data;

    // Scale image.
    int i_src;
    float f_src;
    for (uint y = 0; y < height; ++y) {
        f_src = static_cast<int>(ratio_y * y) * _width;
        for (uint x = 0; x < width; ++x) {
            i_src = static_cast<int>(f_src);
            i_src *= (_has_alpha ? 4 : 3);

            *dest++ = _data[i_src++];
            *dest++ = _data[i_src++];
            *dest++ = _data[i_src++];
            if (_has_alpha) {
                *dest++ = _data[i_src++];
            }

            f_src += ratio_x;
        }
    }

    return scaled_data;
}
