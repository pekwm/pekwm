//
// PImageNative.cc for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "PImageNative.hh"
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

list<PImageNativeLoader*> PImageNative::_loader_list = list<PImageNativeLoader*>();

//! @brief PImageNative constructor.
PImageNative::PImageNative(Display *dpy) : PImage(dpy),
																					 _data(NULL), _has_alpha(false)
{
}

//! @brief PImageNative destructor.
PImageNative::~PImageNative(void)
{
}

//! @brief Loads image from file.
//! @param file File to load.
//! @return Returns true on success, else false.
bool
PImageNative::load(const std::string &file)
{
	string ext(Util::getFileExt(file));
	if (!ext.size()) {
		cerr << " *** WARNING: no file extension on " << file << "!" << endl;
		return false;
	}

	list<PImageNativeLoader*>::iterator it(_loader_list.begin());
	for (; it != _loader_list.end(); ++it) {
		if (!strcasecmp((*it)->getExt(), ext.c_str())) {
			_data = (*it)->load(file, _width, _height, _has_alpha);
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
PImageNative::unload(void)
{
	if (_data) {
		delete [] _data;
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
PImageNative::draw(Drawable draw, int x, int y, uint width, uint height)
{
	if (!_data)
		return;

	// Expand variables.
	if (!width)
		width = _width;
	if (!height)
		height = _height;

	// Draw image.

	// Fixed and Scaled to original size is the same thing.
	if ((_type == IMAGE_TYPE_FIXED)
			|| ((_type == IMAGE_TYPE_SCALED)
					&& (_width == width) && (_height == height))) {
		// Plain copy of the pixmap onto Drawable.
		XCopyArea(_dpy, _pixmap, draw, PScreen::instance()->getGC(),
							0, 0, width, height, x, y);

	} else if (_type == IMAGE_TYPE_SCALED) {
		uchar *scaled_data;
		// Create scaled representation of image.		
		scaled_data = getScaledData(width, height);
		if (scaled_data) {
			Pixmap pix;
			// Create pixmap.
			pix = createPixmap(scaled_data, width, height);
			if (pix) {
				XCopyArea(_dpy, pix, draw, PScreen::instance()->getGC(),
									0, 0, width, height, x, y);
				ScreenResources::instance()->getPixmapHandler()->returnPixmap(pix);
			}

			delete [] scaled_data;
		}
		
	} else if (_type == IMAGE_TYPE_TILED) {
		// Create a GC with _pixmap as tile and tiled fill style.
		GC gc;
		XGCValues gv;

		gv.fill_style = FillTiled;
		gv.tile = _pixmap;
		gv.ts_x_origin = x;
		gv.ts_y_origin = y;

		gc = XCreateGC(_dpy , draw,
									 GCFillStyle|GCTile|GCTileStipXOrigin|GCTileStipYOrigin,
									 &gv);

		// Tile the image onto drawable.
		XFillRectangle(_dpy, draw, gc, x, y, width, height);

		XFreeGC(_dpy, gc);
	}
#ifdef DEBUG
	else {
		cerr << __FILE__ << "@" << __LINE__ << ": "
				 << "PImageNative(" << this << ")::draw(" << draw << ", "
				 << x << " ," << y << ", " << width << ", " << height << ")" << endl
				 << " *** no image type set, not drawing!" << endl;
	}
#endif // DEBUG
}

//! @brief Returns pixmap at sizen.
//! @param need_free Is set to wheter shape mask returned needs to be freed.
//! @param width Pixmap width, defaults to 0 which expands to image size.
//! @param height Pixmap height, defaults to 0 which expands to image size.
//! @return Returns pixmap at size or None on error.
Pixmap
PImageNative::getPixmap(bool &need_free, uint width, uint height)
{
	// Default
	Pixmap pix = None;
	need_free = false;

	// Expand parameters.
	if (!width)
		width = _width;
	if (!height)
		height = _height;

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
PImageNative::getMask(bool &need_free, uint width, uint height)
{
	// Default
	Pixmap pix = None;
	need_free = false;

	// Expand parameters.
	if (!width)
		width = _width;
	if (!height)
		height = _height;

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
PImageNative::scale(uint width, uint height)
{
	// Invalid width or height or no need to scale.
	if (!width || !height || ((width == _width) && (height == height)))
		return;
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

//! @brief Creates Pixmap from data.
//! @param data Pointer to data to create pixmap from.
//! @param width Width of image data is representing.
//! @param height Height of image data is representing.
//! @return Returns Pixmap on success, else None.
Pixmap
PImageNative::createPixmap(uchar *data, uint width, uint height)
{
	XImage *ximage;
	Pixmap pix = None;

	ximage = createXImage(data, width, height);
	if (ximage) {
		pix = ScreenResources::instance()->getPixmapHandler()->getPixmap(width,
																																		 height,
																																		 PScreen::instance()->getDepth(),
																																		 false);

		XPutImage(_dpy, pix, PScreen::instance()->getGC(), ximage,
							0, 0, 0, 0, width, height);

		delete [] ximage->data;
		ximage->data = NULL;
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
PImageNative::createMask(uchar *data, uint width, uint height)
{
  if (!_has_alpha) {
    return None;
  }

  // Create XImage
  XImage *ximage;
  ximage = XCreateImage(_dpy, PScreen::instance()->getVisual()->getXVisual(),
                        1, ZPixmap, 0, NULL, width, height, 32, 0);
  if (!ximage) {
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
  pix = ScreenResources::instance()->getPixmapHandler()->getPixmap(width,
                                                                   height,
                                                                   1,
                                                                   true);
  GC gc = XCreateGC(_dpy, pix, 0, NULL);
  XPutImage(_dpy, pix, gc, ximage,
            0, 0, 0, 0, width, height);
  XFreeGC(_dpy, gc);

  delete [] ximage->data;
  ximage->data = NULL;
  XDestroyImage(ximage);

  return pix;
}

//! @brief Createx XImage from data.
//! @param data Pointer to data to create XImage from.
//! @param width Width of image data is representing.
//! @param height Height of image data is representing.
XImage*
PImageNative::createXImage(uchar *data, uint width, uint height)
{
	// Create XImage
	XImage *ximage;
	ximage = XCreateImage(_dpy, PScreen::instance()->getVisual()->getXVisual(),
                          PScreen::instance()->getDepth(), ZPixmap, 0, NULL,
                          width, height, 32, 0);
	if (!ximage) {
		cerr << " *** WARNING: unable to create XImage!" << endl;
		return NULL;
	}

	// Allocate ximage data storage.
	ximage->data = new char[ximage->bytes_per_line * height / sizeof(char)];

	uchar *src = data;
	uchar r, g, b;
	ulong pixel;

	// Put data into XImage.
	for (uint y = 0; y < height; ++y) {
		for (uint x = 0; x < width; ++x) {
			r = *src++;
			g = *src++;
			b = *src++;
			if (_has_alpha)
				*src++;

			// 5 R, 5 G, 5 B (15 bit display)
			if ((ximage->red_mask == 0x7c00)
					&& (ximage->green_mask == 0x3e0) && (ximage->blue_mask == 0x1f)) {
				pixel = ((r << 7) & 0x7c00)
					| ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f);

				// 5 R, 6 G, 5 B (16 bit display)
			} else  if ((ximage->red_mask == 0xf800)
									&& (ximage->green_mask == 0x07e0)
									&& (ximage->blue_mask == 0x001f)) {
				pixel = ((r << 8) &  0xf800)
					| ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f);

			// 8 R, 8 G, 8 B (24/32 bit display)
			} else if  ((ximage->red_mask == 0xff0000)
									&& (ximage->green_mask == 0xff00)
									&& (ximage->blue_mask == 0xff)) {
				pixel = ((r << 16) & 0xff0000)
					| ((g << 8) & 0x00ff00) | (b & 0x0000ff);
			} else {
				pixel = 0;
			}

			XPutPixel(ximage, x, y, pixel);
		}
	}

	return ximage;
}

//! @brief Scales image data and returns pointer to new data.
//! @param width Width of image data to return.
//! @param height Height of image data to return.
//! @return Pointer to image data on success, else NULL.
//! @todo Implement decent scaling routine with data [] cache?
uchar*
PImageNative::getScaledData(uint width, uint height)
{
	if (!width || !height)
		return NULL;

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
			if (_has_alpha)
				*dest++ = _data[i_src++];

			f_src += ratio_x;
		}
	}

	return scaled_data;
}
