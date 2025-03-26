//
// PImage.cc for pekwm
//
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2005-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "Exception.hh"
#include "PImage.hh"
#include "PImageLoaderJpeg.hh"
#include "PImageLoaderPng.hh"
#include "PImageLoaderXpm.hh"
#include "String.hh"
#include "Util.hh"

#include <memory>

extern "C" {
#include <math.h>
#include <string.h>
#include <X11/Xutil.h>
}

static void
destroyXImage(XImage *ximage)
{
	delete [] ximage->data;
	ximage->data = nullptr;
	X11::destroyImage(ximage);
}

typedef ulong (*rgbToPixel)(uchar, uchar, uchar);

static ulong rgbToPixel15bitLSB(uchar r, uchar g, uchar b)
{
	return ((r << 7) & 0x7c00)
		| ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f);
}

static ulong rgbToPixel16bitLSB(uchar r, uchar g, uchar b)
{
	return ((r << 8) &  0xf800)
		| ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f);
}

static ulong rgbToPixel24bitLSB(uchar r, uchar g, uchar b)
{
	return ((r << 16) & 0xff0000)
		| ((g << 8) & 0x00ff00) | (b & 0x0000ff);
}

static ulong rgbToPixel15bitMSB(uchar r, uchar g, uchar b)
{
	return ((b << 7) & 0x7c00)
		| ((g << 2) & 0x03e0) | ((r >> 3) & 0x001f);
}

static ulong rgbToPixel16bitMSB(uchar r, uchar g, uchar b)
{
	return ((b << 8) &  0xf800)
		| ((g << 3) & 0x07e0) | ((r >> 3) & 0x001f);
}

static ulong rgbToPixel24bitMSB(uchar r, uchar g, uchar b)
{
	return ((b << 16) & 0xff0000)
		| ((g << 8) & 0x00ff00) | (r & 0x0000ff);
}

static ulong rgbToPixelUnknown(uchar r, uchar g, uchar b)
{
	return 0;
}

/**
 * Get RGB to pixel conversion appropriate for this XImage.
 */
static rgbToPixel
getRgbToPixelFun(XImage* ximage)
{
	if ((ximage->red_mask == 0xff0000)
	    && (ximage->green_mask == 0xff00)
	    && (ximage->blue_mask == 0xff)) {
		return rgbToPixel24bitLSB;
	} else if ((ximage->red_mask == 0xf800)
		   && (ximage->green_mask == 0x07e0)
		   && (ximage->blue_mask == 0x001f)) {
		return rgbToPixel16bitLSB;
	} else if ((ximage->red_mask == 0x7c00)
		   && (ximage->green_mask == 0x3e0)
		   && (ximage->blue_mask == 0x1f)) {
		return rgbToPixel15bitLSB;
	} else if  ((ximage->red_mask == 0xff)
		    && (ximage->green_mask == 0xff00)
		    && (ximage->blue_mask == 0xff0000)) {
		return rgbToPixel24bitMSB;
	} else if ((ximage->red_mask == 0x001f)
		   && (ximage->green_mask == 0x07e0)
		   && (ximage->blue_mask == 0xf800)) {
		return rgbToPixel16bitMSB;
	} else if ((ximage->red_mask == 0x1f)
		   && (ximage->green_mask == 0x3e0)
		   && (ximage->blue_mask == 0x7c00)) {
		return rgbToPixel15bitMSB;
	} else {
		return rgbToPixelUnknown;
	}
}

typedef void (*pixelToRgb)(ulong, uchar&, uchar&, uchar&);

static void pixelToRgb15bitLSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel >> 7) & 0x7c;
	g = (pixel >> 2) & 0x3e;
	b = (pixel << 3) & 0x1f;
}

static void pixelToRgb16bitLSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel >> 8) & 0xf8;
	g = (pixel >> 3) & 0x7e;
	b = (pixel << 3) & 0x1f;
}

static void pixelToRgb24bitLSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel >> 16) & 0xff;
	g = (pixel >> 8) & 0xff;
	b = pixel & 0xff;
}

static void pixelToRgb15bitMSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel << 3) & 0x1f;
	g = (pixel >> 2) & 0x3e;
	b = (pixel >> 7) & 0x7c;
}

static void pixelToRgb16bitMSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel << 3) & 0x1f;
	g = (pixel >> 3) & 0x7e;
	b = (pixel >> 8) & 0xf8;
}

static void pixelToRgb24bitMSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = pixel & 0xff;
	g = (pixel >> 8) & 0xff;
	b = (pixel >> 16) & 0xff;
}

static void pixelToRgbUnknown(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = 0;
	g = 0;
	b = 0;
}

/**
 * Get pixel to RGB conversion appropriate for this XImage.
 */
static pixelToRgb
getPixelToRgbFun(ulong red_mask, ulong green_mask, ulong blue_mask,
		 bool is_retry)
{
	if  ((red_mask == 0xff0000)
	     && (green_mask == 0xff00)
	     && (blue_mask == 0xff)) {
		return pixelToRgb24bitLSB;
	} else if ((red_mask == 0xf800)
		   && (green_mask == 0x07e0)
		   && (blue_mask == 0x001f)) {
		return pixelToRgb16bitLSB;
	} else if ((red_mask == 0x7c00)
		   && (green_mask == 0x3e0)
		   && (blue_mask == 0x1f)) {
		return pixelToRgb15bitLSB;
	} else if  ((red_mask == 0xff)
		    && (green_mask == 0xff00)
		    && (blue_mask == 0xff0000)) {
		return pixelToRgb24bitMSB;
	} else if ((red_mask == 0x001f)
		   && (green_mask == 0x07e0)
		   && (blue_mask == 0xf800)) {
		return pixelToRgb16bitMSB;
	} else if ((red_mask == 0x1f)
		   && (green_mask == 0x3e0)
		   && (blue_mask == 0x7c00)) {
		return pixelToRgb15bitMSB;
	} else if (is_retry) {
		P_DBG("failed to get visual for r: " << std::hex << red_mask
		      << " g: " << std::hex << green_mask
		      << " b: " << std::hex << blue_mask);
		return pixelToRgbUnknown;
	} else {
		XVisualInfo info;
		if (X11::getVisualInfo(info)) {
			return getPixelToRgbFun(info.red_mask, info.green_mask,
						info.blue_mask, true);
		}
		return pixelToRgbUnknown;
	}
}

static pixelToRgb
getPixelToRgbFun(XImage *ximage)
{
	return getPixelToRgbFun(ximage->red_mask, ximage->green_mask,
				ximage->blue_mask, false);
}

PImage::PImage()
	: _type(IMAGE_TYPE_NO),
	  _pixmap(None),
	  _mask(None),
	  _width(0),
	  _height(0),
	  _data(nullptr),
	  _use_alpha(false)
{
}

/**
 * PImage constructor, loads image if one is specified.
 *
 * @param path Path to image file, if specified this is loaded.
 */
PImage::PImage(const std::string &path)
	: _type(IMAGE_TYPE_NO),
	  _pixmap(None),
	  _mask(None),
	  _width(0),
	  _height(0),
	  _data(nullptr),
	  _use_alpha(false),
	  _trans_pixel(0)
{
	if (! path.size() || ! load(path)) {
		throw LoadException(path);
	}
}

PImage::PImage(PImage *image)
	: _type(image->getType()),
	  _pixmap(None),
	  _mask(None),
	  _width(image->getWidth()),
	  _height(image->getHeight()),
	  _use_alpha(image->_use_alpha),
	  _trans_pixel(image->_trans_pixel)
{
	_data = new uchar[_width * _height * 4];
	memcpy(_data, image->getData(), _width * _height * 4);
}

/**
 * Create PImage from XImage.
 */
PImage::PImage(XImage *image, uchar opacity, ulong *trans_pixel)
	: _type(IMAGE_TYPE_FIXED),
	  _pixmap(None),
	  _mask(None),
	  _width(image->width),
	  _height(image->height),
	  _data(new uchar[image->width * image->height * 4]),
	  _use_alpha(trans_pixel != nullptr),
	  _trans_pixel(trans_pixel ? *trans_pixel : 0)
{
	pixelToRgb toRgb = getPixelToRgbFun(image);

	uint dst = 0;
	for (uint y = 0; y < _height; ++y) {
		for (uint x = 0; x < _width; ++x) {
			ulong pixel = XGetPixel(image, x, y);
			if (trans_pixel && pixel == *trans_pixel) {
				_data[dst] = 0;
			} else {
				_data[dst] = opacity;
			}
			toRgb(pixel,
			      _data[dst + 1],
			      _data[dst + 2],
			      _data[dst + 3]);
			dst += 4;
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
	unload();

	std::string ext(Util::getFileExt(file));
	if (! ext.size()) {
		USER_WARN("no file extension on " << file);
		return false;
	}

#ifdef PEKWM_HAVE_IMAGE_JPEG
	if (pekwm::ascii_ncase_equal(PImageLoaderJpeg::getExt(), ext)) {
		_data = PImageLoaderJpeg::load(file,
					       _width,
					       _height,
					       _use_alpha);
	} else
#endif // PEKWM_HAVE_IMAGE_JPEG
#ifdef PEKWM_HAVE_IMAGE_PNG
		if (pekwm::ascii_ncase_equal(PImageLoaderPng::getExt(), ext)) {
			_data = PImageLoaderPng::load(file,
						      _width,
						      _height,
						      _use_alpha);
		} else
#endif // PEKWM_HAVE_IMAGE_PNG
#ifdef PEKWM_HAVE_IMAGE_XPM
			if (pekwm::ascii_ncase_equal(PImageLoaderXpm::getExt(),
						     ext)) {
				_data = PImageLoaderXpm::load(file,
							      _width,
							      _height,
							      _use_alpha);
			} else
#endif // PEKWM_HAVE_IMAGE_XPM
				{
					// no loader matched
					_data = nullptr;
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
 * @param rend Renderer used for drawing.
 * @param x Destination x coordinate.
 * @param y Destination y coordinate.
 * @param width Destination width, defaults to 0 which expands to image size.
 * @param height Destination height, defaults to 0 which expands to image size.
 */
void
PImage::draw(Render &rend, PSurface *surface, int x, int y,
	     uint width, uint height)
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
			drawAlphaFixed(rend, surface, x, y, width, height,
				       _data);
		} else {
			drawFixed(rend, x, y, width, height);
		}
	} else if (_type == IMAGE_TYPE_SCALED) {
		if (_use_alpha) {
			drawAlphaScaled(rend, surface, x, y, width, height);
		} else {
			drawScaled(rend, x, y, width, height);
		}
	} else if (_type == IMAGE_TYPE_TILED) {
		if (_use_alpha) {
			drawAlphaTiled(rend, surface, x, y, width, height);
		} else {
			drawTiled(rend, x, y, width, height);
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
		if (_pixmap == None) {
			_pixmap = createPixmap(_data, width, height);
		}
		pix = _pixmap;
	} else {
		uchar *scaled_data = getScaledData(width, height, SCALE_SMOOTH);
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
		if (_mask == None) {
			_mask = createMask(_data, width, height);
		}
		pix = _mask;
	} else {
		uchar *scaled_data = getScaledData(width, height, SCALE_SMOOTH);
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
 * Scales image by the given factor.
 */
void
PImage::scale(float factor, ScaleType type)
{
	uint width = static_cast<uint>(factor * _width);
	uint height = static_cast<uint>(factor * _height);
	scale(width, height, type);
}

/**
 * Scales image to size.
 *
 * @param width Width to scale image to.
 * @param height Height to scale image to.
 */
void
PImage::scale(uint width, uint height, ScaleType type)
{
	// Invalid width or height or no need to scale.
	if (! width || ! height || ((width == _width) && (height == _height))) {
		return;
	}

	uchar *scaled_data = getScaledData(width, height, type);
	if (scaled_data) {
		// Free old resources.
		unload();

		// Set data pointer and create pixmap and mask at new size.
		_data = scaled_data;
		_width = width;
		_height = height;
	}
}

/**
 * Draw image at position, not scaling.
 */
void
PImage::drawAlphaFixed(Render &rend, PSurface *surface, int x, int y,
		       uint width, uint height, uchar* data)
{
	if (! surface->clip(x, y, width, height)) {
		// coordinates are completely outside of the surface, nothing
		// to render
		return;
	}

	XImage *dest_image = rend.getImage(x, y, width, height);
	if (! dest_image) {
		P_ERR("failed to get image for area x " << x << " y " << y
		      << " width " << width << " height " << height);
		return;
	}

	drawAlphaFixed(dest_image, dest_image, x, y, width, height, data);

	rend.putImage(dest_image, x, y, width, height);
	rend.destroyImage(dest_image);
}

void
PImage::drawAlphaFixed(XImage *src_image, XImage *dest_image,
		       int x, int y, uint width, uint height, uchar* data)
{
	// Get mask from visual
	Visual *visual = X11::getVisual();
	src_image->red_mask = visual->red_mask;
	src_image->green_mask = visual->green_mask;
	src_image->blue_mask = visual->blue_mask;
	dest_image->red_mask = visual->red_mask;
	dest_image->green_mask = visual->green_mask;
	dest_image->blue_mask = visual->blue_mask;

	pixelToRgb toRgb = getPixelToRgbFun(src_image);
	rgbToPixel toPixel = getRgbToPixelFun(dest_image);

	uchar *src = data;
	for (uint i_y = 0; i_y < height; ++i_y) {
		for (uint i_x = 0; i_x < width; ++i_x) {
			// Get pixel value, copy them directly if alpha is set
			// to 255.
			uchar a = *src++;
			uchar r = *src++;
			uchar g = *src++;
			uchar b = *src++;

			if (a == 0) {
				// Not visible, skip blending
				toRgb(XGetPixel(src_image, i_x, i_y), r, g, b);
			} else if (a != 255) {
				// Not solid, blend
				uchar d_r = 0, d_g = 0, d_b = 0;
				toRgb(XGetPixel(src_image, i_x, i_y),
				      d_r, d_g, d_b);

				float a_percent = static_cast<float>(a) / 255;
				float a_percent_inv = 1 - a_percent;
				r = static_cast<uchar>((a_percent_inv * d_r)
						       + (a_percent * r));
				g = static_cast<uchar>((a_percent_inv * d_g)
						       + (a_percent * g));
				b = static_cast<uchar>((a_percent_inv * d_b)
						       + (a_percent * b));
			}

			XPutPixel(dest_image, i_x, i_y, toPixel(r, g, b));
		}
	}
}


/**
 * Draw image at position, not scaling.
 */
void
PImage::drawFixed(Render &rend, int x, int y, uint width, uint height)
{
	width = std::min(width, _width);
	height = std::min(width, _height);

	if (rend.getDrawable() == None) {
		XImage *ximage = createXImage(_data, _width, _height);
		if (ximage) {
			rend.putImage(ximage, x, y, width, height);
			destroyXImage(ximage);
		}
	} else {
		// Plain copy of the pixmap onto Drawable.
		bool need_free;
		X11::copyArea(getPixmap(need_free), rend.getDrawable(),
			      0, 0, width, height, x, y);
	}
}

/**
 * Draw image scaled to fit width and height.
 */
void
PImage::drawScaled(Render &rend, int x, int y, uint width, uint height)
{
	// Create scaled representation of image.
	uchar *scaled_data = getScaledData(width, height, SCALE_SMOOTH);
	if (scaled_data) {
		XImage *ximage = createXImage(scaled_data, width, height);
		delete [] scaled_data;
		if (ximage) {
			rend.putImage(ximage, x, y, width, height);
			destroyXImage(ximage);
		}
	}
}

struct RenderAndXImage
{
	RenderAndXImage(Render &_rend, XImage *_ximage)
		: rend(_rend),
		  ximage(_ximage)
	{
	}

	Render &rend;
	XImage *ximage;
};

static void
renderWithXImageRender(int x, int y, uint width, uint height, void *opaque)
{
	RenderAndXImage *raxi = reinterpret_cast<RenderAndXImage*>(opaque);
	raxi->rend.putImage(raxi->ximage, x, y, width, height);
}

/**
 * Draw image tiled to fit width and height.
 */
void
PImage::drawTiled(Render &rend, int x, int y, uint width, uint height)
{
	if (rend.getDrawable() == None) {
		XImage *ximage = createXImage(_data, _width, _height);
		if (ximage) {
			RenderAndXImage raxi(rend, ximage);
			renderTiled(x, y, width, height, _width, _height,
				    renderWithXImageRender,
				    reinterpret_cast<void*>(&raxi));
			destroyXImage(ximage);
		}
	} else {
		Drawable dest = rend.getDrawable();
		bool need_free;
		// Create a GC with _pixmap as tile and tiled fill style.
		XGCValues gv;
		gv.fill_style = FillTiled;
		gv.tile = getPixmap(need_free);
		gv.ts_x_origin = x;
		gv.ts_y_origin = y;

		ulong gv_mask =
			GCFillStyle|GCTile|GCTileStipXOrigin|GCTileStipYOrigin;
		GC gc = XCreateGC(X11::getDpy(), dest, gv_mask, &gv);

		// Tile the image onto drawable.
		XFillRectangle(X11::getDpy(), dest, gc, x, y, width, height);

		XFreeGC(X11::getDpy(), gc);
	}
}

/**
 * Draw image scaled to fit width and height.
 */
void
PImage::drawAlphaScaled(Render &rend, PSurface *surface, int x, int y,
			uint width, uint height)
{
	uchar *scaled_data = getScaledData(width, height, SCALE_SMOOTH);
	if (scaled_data) {
		drawAlphaFixed(rend, surface, x, y, width, height,
			       scaled_data);
		delete [] scaled_data;
	}
}

struct ImageRenderAndData
{
	ImageRenderAndData(PImage *_image, Render &_rend, PSurface *_surface,
			   uchar *_data)
		: image(_image),
		  rend(_rend),
		  surface(_surface),
		  data(_data)
	{
	}

	PImage *image;
	Render &rend;
	PSurface *surface;
	uchar *data;
};

static void
renderWithAlphaFixed(int x, int y, uint width, uint height, void *opaque)
{
	ImageRenderAndData *irad =
		reinterpret_cast<ImageRenderAndData*>(opaque);
	irad->image->drawAlphaFixed(irad->rend, irad->surface,
				    x, y, width, height, irad->data);
}

/**
 * Draw image tiled to fit width and height.
 */
void
PImage::drawAlphaTiled(Render &rend, PSurface *surface, int x, int y,
		       uint width, uint height)
{
	ImageRenderAndData irad(this, rend, surface, _data);
	renderTiled(x, y, width, height, _width, _height,
		    renderWithAlphaFixed, reinterpret_cast<void*>(&irad));
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

	XImage *ximage = createXImage(data, width, height);
	if (ximage) {
		pix = X11::createPixmap(width, height);
		X11::putImage(pix, X11::getGC(), ximage,
			      0, 0, 0, 0, width, height);
		destroyXImage(ximage);
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


	X11_XImage ximage(1, width, height);
	if (! *ximage) {
		 P_ERR("failed to create XImage(1, " << width << ", "
		       << height << ")");
		 return None;
	}
	ulong pixel_trans = X11::getBlackPixel();
	ulong pixel_solid = X11::getWhitePixel();
	uchar *src = data;
	for (uint y = 0; y < height; ++y) {
		for (uint x = 0; x < width; ++x) {
			XPutPixel(*ximage, x, y,
				  (*src > 127) ? pixel_solid : pixel_trans);
			src += 4; // Skip A, R, G, and B
		}
	}

	Pixmap pix = X11::createPixmapMask(width, height);
	X11_GC gc(pix, 0, nullptr);
	X11::putImage(pix, *gc, *ximage, 0, 0, 0, 0, width, height);
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
	XImage *ximage = X11::createImage(nullptr, width, height);
	if (! ximage) {
		P_ERR("failed to create XImage " << width << "x" << height);
		return nullptr;
	}

	// Allocate ximage data storage.
	ximage->data = new char[ximage->bytes_per_line * height];

	uchar *src = data;

	rgbToPixel toPixel = getRgbToPixelFun(ximage);

	// Put data into XImage.
	for (uint y = 0; y < height; ++y) {
		for (uint x = 0; x < width; ++x) {
			uchar a = *src++;
			uchar r = *src++;
			uchar g = *src++;
			uchar b = *src++;
			if (a == 0) {
				XPutPixel(ximage, x, y, _trans_pixel);
			} else {
				XPutPixel(ximage, x, y, toPixel(r, g, b));
			}
		}
	}

	return ximage;
}

static inline uchar
scalePixel(const uchar* data, int pos, int width, float x_diff, float y_diff)
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
PImage::getScaledData(uint dwidth, uint dheight, ScaleType type)
{
	if (type == SCALE_SQUARE) {
		float wfactor = static_cast<float>(dwidth) / _width;
		float hfactor = static_cast<float>(dheight) / _height;
		if (wfactor == hfactor && fmod(wfactor, 1.0) == 0.0) {
			return getScaledDataSquare(static_cast<uint>(wfactor));
		}
	}
	return getScaledDataSmooth(dwidth, dheight);
}

uchar*
PImage::getScaledDataSmooth(uint dwidth, uint dheight)
{
	if (dwidth < 1 || dheight < 1) {
		return nullptr;
	}

	uchar *scaled_data = new uchar[dwidth * dheight * 4] ;
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
				scalePixel(_data, spos, _width,
					   x_diff, y_diff); // A
			scaled_data[dst++] =
				scalePixel(_data + 1, spos, _width,
					   x_diff, y_diff); // R
			scaled_data[dst++] =
				scalePixel(_data + 2, spos, _width,
					   x_diff, y_diff); // G
			scaled_data[dst++] =
				scalePixel(_data + 3, spos, _width,
					   x_diff, y_diff); // B
		}
	}

	return scaled_data;
}

uchar*
PImage::getScaledDataSquare(uint factor)
{
	if (factor < 2) {
		return nullptr;
	}

	uint dwidth = _width * factor;
	uint dheight = _height * factor;
	uchar *scaled_data = new uchar[dwidth * dheight * 4] ;
	for (uint y = 0; y < _height; y++) {
		uchar *src = _data + (y * _width * 4);
		uchar *dst = scaled_data + (y * dwidth * factor * 4);
		for (uint x = 0; x < _width; x++) {
			setScaledDataSquare(factor, src, dst);
			src += 4;
			dst += factor * 4;
		}
	}
	return scaled_data;
}

void
PImage::setScaledDataSquare(uint factor, const uchar *src, uchar *dst)
{
	for (uint y = 0; y < factor; y++) {
		uchar *p = dst;
		for (uint x = 0; x < factor; x++) {
			*p++ = src[0];
			*p++ = src[1];
			*p++ = src[2];
			*p++ = src[3];
		}
		dst += _width * factor * 4;
	}
}
