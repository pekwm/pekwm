//
// PImage.cc for pekwm
// Copyright (C) 2005-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "PImage.hh"
#include "x11.hh"
#include "Util.hh"

#include <iostream>

extern "C" {
#include <X11/Xutil.h>
}

/**
 * Create pixel value suitable for ximage from R, G and B.
 */
static inline ulong
getPixelFromRgb(XImage* ximage, uchar r, uchar g, uchar b)
{
    // 5 R, 5 G, 5 B (15 bit display)
    if ((ximage->red_mask == 0x7c00)
        && (ximage->green_mask == 0x3e0) && (ximage->blue_mask == 0x1f)) {
        return ((r << 7) & 0x7c00)
            | ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f);

        // 5 R, 6 G, 5 B (16 bit display)
    } else  if ((ximage->red_mask == 0xf800)
                && (ximage->green_mask == 0x07e0)
                && (ximage->blue_mask == 0x001f)) {
        return ((r << 8) &  0xf800)
            | ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f);

        // 8 R, 8 G, 8 B (24/32 bit display)
    } else if  ((ximage->red_mask == 0xff0000)
                && (ximage->green_mask == 0xff00)
                && (ximage->blue_mask == 0xff)) {
        return ((r << 16) & 0xff0000)
            | ((g << 8) & 0x00ff00) | (b & 0x0000ff);
    } else {
        return 0;
    }
}

/**
 * Fill in RGB values from pixel value from ximage.
 */
static inline void
getRgbFromPixel(XImage *ximage, ulong pixel, uchar &r, uchar &g, uchar &b)
{
    // 5 R, 5 G, 5 B (15 bit display)
    if ((ximage->red_mask == 0x7c00)
        && (ximage->green_mask == 0x3e0) && (ximage->blue_mask == 0x1f)) {
        r = (pixel >> 7) & 0x7c;
        g = (pixel >> 2) & 0x3e;
        b = (pixel << 3) & 0x1f;
        // 5 R, 6 G, 5 B (16 bit display)
    } else  if ((ximage->red_mask == 0xf800)
                && (ximage->green_mask == 0x07e0)
                && (ximage->blue_mask == 0x001f)) {
        r = (pixel >> 8) & 0xf8;
        g = (pixel >> 3) & 0x7e;
        b = (pixel << 3) & 0x1f;
        // 8 R, 8 G, 8 B (24/32 bit display)
    } else if  ((ximage->red_mask == 0xff0000)
                && (ximage->green_mask == 0xff00)
                && (ximage->blue_mask == 0xff)) {
        r = (pixel >> 16) & 0xff;
        g = (pixel >> 8) & 0xff;
        b = pixel & 0xff;
    } else {
        r = 0;
        g = 0;
        b = 0;
    }
}

std::vector<PImageLoader*> PImage::_loaders;

/**
 * PImage constructor, loads image if one is specified.
 *
 * @param dpy Display image is valid on.
 * @param path Path to image file, if specified this is loaded.
 */
PImage::PImage(const std::string &path)
    : _type(IMAGE_TYPE_NO),
      _pixmap(None),
      _mask(None),
      _width(0),
      _height(0),
      _data(nullptr),
      _use_alpha(false)
{
    if (path.size()) {
        if (! load(path)) {
            throw LoadException(path.c_str());
        }
    }
}

PImage::~PImage(void)
{
    unload();
}

/**
 * Loads image from file.
 *
 * @param file File to load.
 * @return Returns true on success, else false.
 */
bool
PImage::load(const std::string &file)
{
    std::string ext(Util::getFileExt(file));
    if (! ext.size()) {
        std::cerr << " *** WARNING: no file extension on " << file << "!"
                  << std::endl;
        return false;
    }

    for (auto it : _loaders) {
        if (! strcasecmp(it->getExt(), ext.c_str())) {
            _data = it->load(file, _width, _height, _use_alpha);
            if (_data) {
                _pixmap = createPixmap(_data, _width, _height);
                _mask = createMask(_data, _width, _height);
                break;
            }
        }
    }

    return _data != nullptr;
}

/**
 * Frees resources used by image.
 */
void
PImage::unload(void)
{
    if (_data) {
        delete [] _data;
        _data = nullptr;
    }
    if (_pixmap) {
        X11::freePixmap(_pixmap);
    }
    if (_mask) {
        X11::freePixmap(_mask);
    }

    _pixmap = None;
    _mask = None;
    _width = 0;
    _height = 0;
}

/**
 * Draws image on drawable.
 *
 * @param draw Drawable to draw on.
 * @param x Destination x coordinate.
 * @param y Destination y coordinate.
 * @param width Destination width, defaults to 0 which expands to image size.
 * @param height Destination height, defaults to 0 which expands to image size.
 */
void
PImage::draw(Drawable draw, int x, int y, uint width, uint height)
{
    if (_data == nullptr) {
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

/**
 * Returns pixmap at sizen.
 * @param need_free Is set to wheter shape mask returned needs to be freed.
 * @param width Pixmap width, defaults to 0 which expands to image size.
 * @param height Pixmap height, defaults to 0 which expands to image size.
 * @return Returns pixmap at size or None on error.
 */
Pixmap
PImage::getPixmap(bool &need_free, uint width, uint height)
{
    need_free = false;

    if (! width) {
        width = _width;
    }
    if (! height) {
        height = _height;
    }

    Pixmap pix;
    if ((width == _width) && (height == _height)) {
        // Same size, return _pixmap.
        pix = _pixmap;
    } else {
        auto scaled_data = getScaledData(width, height);
        if (scaled_data) {
            need_free = true;
            pix = createPixmap(scaled_data, width, height);
            delete [] scaled_data;
        } else {
            pix = None;
        }
    }

    return pix;
}

/**
 * Returns shape mask at size, if any.
 *
 * @param need_free Is set to wheter shape mask returned needs to be freed.
 * @param width Shape mask width, defaults to 0 which expands to image size.
 * @param height Shape mask height, defaults to 0 which expands to image size.
 * @return Returns shape mask at size or None if there is no mask information.
 */
Pixmap
PImage::getMask(bool &need_free, uint width, uint height)
{
    need_free = false;

    if (! _use_alpha) {
        return None;
    }

    // Expand parameters.
    if (! width) {
        width = _width;
    }
    if (! height) {
        height = _height;
    }

    Pixmap pix;
    if ((width == _width) && (height == _height)) {
        // Same size, return _mask.
        pix = _mask;
    } else {
        auto scaled_data = getScaledData(width, height);
        if (scaled_data) {
            need_free = true;
            pix = createMask(scaled_data, width, height);
            delete [] scaled_data;
        } else {
            pix = None;
        }
    }

    return pix;
}

/**
 * Scales image to size.
 *
 * @param width Width to scale image to.
 * @param height Height to scale image to.
 */
void
PImage::scale(uint width, uint height)
{
    // Invalid width or height or no need to scale.
    if (! width || ! height || ((width == _width) && (height == _height))) {
        return;
    }

    auto scaled_data = getScaledData(width, height);
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

/**
 * Draw image at position, not scaling.
 */
void
PImage::drawFixed(Drawable dest, int x, int y, uint width, uint height)
{
    // Plain copy of the pixmap onto Drawable.
    XCopyArea(X11::getDpy(), _pixmap, dest, X11::getGC(),
              0, 0, width, height, x, y);

}

/**
 * Draw image scaled to fit width and height.
 */
void
PImage::drawScaled(Drawable dest, int x, int y, uint width, uint height)
{
    // Create scaled representation of image.
    auto scaled_data = getScaledData(width, height);
    if (scaled_data) {
        // Create pixmap.
        auto pix = createPixmap(scaled_data, width, height);
        if (pix) {
            XCopyArea(X11::getDpy(), pix, dest, X11::getGC(),
                      0, 0, width, height, x, y);
            X11::freePixmap(pix);
        }

        delete [] scaled_data;
    }
}

/**
 * Draw image tiled to fit width and height.
 */
void
PImage::drawTiled(Drawable dest, int x, int y, uint width, uint height)
{
    // Create a GC with _pixmap as tile and tiled fill style.
    XGCValues gv;
    gv.fill_style = FillTiled;
    gv.tile = _pixmap;
    gv.ts_x_origin = x;
    gv.ts_y_origin = y;

    GC gc = XCreateGC(X11::getDpy(), dest,
                      GCFillStyle|GCTile|GCTileStipXOrigin|GCTileStipYOrigin,
                      &gv);

    // Tile the image onto drawable.
    XFillRectangle(X11::getDpy(), dest, gc, x, y, width, height);

    XFreeGC(X11::getDpy(), gc);
}

/**
 * Draw image at position, not scaling.
 */
void
PImage::drawAlphaFixed(Drawable dest, int x, int y, uint width, uint height,
                       uchar* data)
{
    auto dest_image = XGetImage(X11::getDpy(), dest,
                                x, y, width, height, AllPlanes, ZPixmap);
    if (! dest_image) {
        std::cerr << " *** ERROR: failed to get image for destination."
                  << std::endl;
        return;
    }

    // Get mask from visual
    auto visual = X11::getVisual();
    dest_image->red_mask = visual->red_mask;
    dest_image->green_mask = visual->green_mask;
    dest_image->blue_mask = visual->blue_mask;

    uchar *src;
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
            uchar a = *src++;
            uchar r = *src++;
            uchar g = *src++;
            uchar b = *src++;

            // Alpha not 100% solid, blend
            if (a != 255) {
                // Get RGB values from pixel.
                uchar d_r = 0, d_g = 0, d_b = 0;
                getRgbFromPixel(dest_image, XGetPixel(dest_image, i_x, i_y),
                                d_r, d_g, d_b);

                float a_percent = static_cast<float>(a) / 255;
                float a_percent_inv = 1 - a_percent;
                r = static_cast<uchar>((a_percent_inv * d_r) + (a_percent * r));
                g = static_cast<uchar>((a_percent_inv * d_g) + (a_percent * g));
                b = static_cast<uchar>((a_percent_inv * d_b) + (a_percent * b));
            }

            XPutPixel(dest_image, i_x, i_y,
                      getPixelFromRgb(dest_image, r, g, b));
        }
    }

    XPutImage(X11::getDpy(), dest, X11::getGC(), dest_image,
              0, 0, x, y, width, height);
    XDestroyImage(dest_image);
}

/**
 * Draw image scaled to fit width and height.
 */
void
PImage::drawAlphaScaled(Drawable dest, int x, int y, uint width, uint height)
{
    auto scaled_data = getScaledData(width, height);
    if (scaled_data) {
        drawAlphaFixed(dest, x, y, width, height, scaled_data);
        delete [] scaled_data;
    }
}

/**
 * Draw image tiled to fit width and height.
 */
void
PImage::drawAlphaTiled(Drawable dest, int x, int y, uint width, uint height)
{
    // FIXME: Implement tiled rendering with alpha support
    drawTiled(dest, x, y, width, height);
}

/**
 * Creates Pixmap from data.
 *
 * @param data Pointer to data to create pixmap from.
 * @param width Width of image data is representing.
 * @param height Height of image data is representing.
 * @return Returns Pixmap on success, else None.
 */
Pixmap
PImage::createPixmap(uchar* data, uint width, uint height)
{
    Pixmap pix = None;

    auto ximage = createXImage(data, width, height);
    if (ximage) {
        pix = X11::createPixmap(width, height);
        XPutImage(X11::getDpy(), pix, X11::getGC(), ximage,
                  0, 0, 0, 0, width, height);

        delete [] ximage->data;
        ximage->data = 0;
        XDestroyImage(ximage);
    }

    return pix;
}

/**
 * Creates shape mask Pixmap from data.
 *
 * @param data Pointer to data to create mask from.
 * @param width Width of image data is representing.
 * @param height Height of image data is representing.
 * @return Returns Pixmap mask on success, else None.
 */
Pixmap
PImage::createMask(uchar* data, uint width, uint height)
{
    if (! _use_alpha) {
        return None;
    }

    // Create XImage
    auto ximage = XCreateImage(X11::getDpy(), X11::getVisual(),
                               1, ZPixmap, 0, 0, width, height, 32, 0);
    if (! ximage) {
        std::cerr << " *** WARNING: unable to create XImage!" << std::endl;
        return None;
    }

    // Alocate ximage data storage.
    ximage->data = new char[ximage->bytes_per_line * height];

    uchar *src = data;

    auto pixel_trans = X11::getBlackPixel();
    auto pixel_solid = X11::getWhitePixel();
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            XPutPixel(ximage, x, y, (*src > 127) ? pixel_solid : pixel_trans);
            src += 4; // Skip A, R, G, and B
        }
    }

    Pixmap pix{X11::createPixmapMask(width, height)};
    GC gc = XCreateGC(X11::getDpy(), pix, 0, 0);
    XPutImage(X11::getDpy(), pix, gc, ximage, 0, 0, 0, 0, width, height);
    XFreeGC(X11::getDpy(), gc);

    delete [] ximage->data;
    ximage->data = 0;
    XDestroyImage(ximage);

    return pix;
}

/**
 * Createx XImage from data.
 *
 * @param data Pointer to data to create XImage from.
 * @param width Width of image data is representing.
 * @param height Height of image data is representing.
 */
XImage*
PImage::createXImage(uchar* data, uint width, uint height)
{
    // Create XImage
    auto ximage = XCreateImage(X11::getDpy(), X11::getVisual(),
                               X11::getDepth(), ZPixmap, 0, 0,
                               width, height, 32, 0);
    if (! ximage) {
        std::cerr << " *** WARNING: unable to create XImage!" << std::endl;
        return nullptr;
    }

    // Allocate ximage data storage.
    ximage->data = new char[ximage->bytes_per_line * height];

    uchar *src = data;

    // Put data into XImage.
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            // skip alpha
            src++;
            uchar r = *src++;
            uchar g = *src++;
            uchar b = *src++;
            XPutPixel(ximage, x, y, getPixelFromRgb(ximage, r, g, b));
        }
    }

    return ximage;
}

static inline uchar
scalePixel(uchar* data, int pos, int width, float x_diff, float y_diff)
{
    float p0 = data[pos];
    float p1 = data[pos + 4];
    float p2 = data[pos + width * 4];
    float p3 = data[pos + 4 + width * 4];
    float res = p0 * (1 - x_diff) * (1 - y_diff)
        + p1 * (x_diff) * (1 - y_diff)
        + p2 * (y_diff) * (1 - x_diff)
        + p3 * (x_diff * y_diff);
    return static_cast<uchar>(res);
}


/**
 * Scales image data and returns pointer to new data.
 *
 * @param width Width of image data to return.
 * @param height Height of image data to return.
 * @return Pointer to image data on success, else nullptr.
 */
uchar*
PImage::getScaledData(uint dwidth, uint dheight)
{
    if (dwidth < 1 || dheight < 1) {
        return nullptr;
    }

    auto scaled_data = new uchar[dwidth * dheight * 4] ;
    float x_ratio = static_cast<float>(_width - 1) / dwidth ;
    float y_ratio = static_cast<float>(_height - 1) / dheight;

    uint dst = 0 ;
    for (uint dy = 0; dy < dheight; dy++) {
        for (uint dx = 0; dx < dwidth; dx++) {
            uint sx = static_cast<uint>(x_ratio * dx);
            uint sy = static_cast<uint>(y_ratio * dy);

            int spos = (sy * _width + sx) * 4;
            float x_diff = (x_ratio * dx) - sx;
            float y_diff = (y_ratio * dy) - sy;

            scaled_data[dst++] =
                scalePixel(_data, spos, _width, x_diff, y_diff); // A
            scaled_data[dst++] =
                scalePixel(_data + 1, spos, _width, x_diff, y_diff); // R
            scaled_data[dst++] =
                scalePixel(_data + 2, spos, _width, x_diff, y_diff); // G
            scaled_data[dst++] =
                scalePixel(_data + 3, spos, _width, x_diff, y_diff); // B
        }
    }

    return scaled_data;
}
