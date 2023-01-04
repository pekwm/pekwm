//
// PImage.cc for pekwm
//
// Copyright (C) 2022-2023 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2005-2021 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "PImage.hh"
#include "PImageLoaderJpeg.hh"
#include "PImageLoaderPng.hh"
#include "PImageLoaderXpm.hh"
#include "String.hh"
#include "Util.hh"

#include <cstring>
#include <memory>

extern "C" {
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

// cppcheck-suppress unusedFunction
static ulong rgbToPixel15bitLSB(uchar r, uchar g, uchar b)
{
	return ((r << 7) & 0x7c00)
		| ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f);
}

// cppcheck-suppress unusedFunction
static ulong rgbToPixel16bitLSB(uchar r, uchar g, uchar b)
{
	return ((r << 8) &  0xf800)
		| ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f);
}

// cppcheck-suppress unusedFunction
static ulong rgbToPixel24bitLSB(uchar r, uchar g, uchar b)
{
	return ((r << 16) & 0xff0000)
		| ((g << 8) & 0x00ff00) | (b & 0x0000ff);
}

// cppcheck-suppress unusedFunction
static ulong rgbToPixel15bitMSB(uchar r, uchar g, uchar b)
{
	return ((b << 7) & 0x7c00)
		| ((g << 2) & 0x03e0) | ((r >> 3) & 0x001f);
}

// cppcheck-suppress unusedFunction
static ulong rgbToPixel16bitMSB(uchar r, uchar g, uchar b)
{
	return ((b << 8) &  0xf800)
		| ((g << 3) & 0x07e0) | ((r >> 3) & 0x001f);
}

// cppcheck-suppress unusedFunction
static ulong rgbToPixel24bitMSB(uchar r, uchar g, uchar b)
{
	return ((b << 16) & 0xff0000)
		| ((g << 8) & 0x00ff00) | (r & 0x0000ff);
}

// cppcheck-suppress unusedFunction
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

// cppcheck-suppress unusedFunction
static void pixelToRgb15bitLSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel >> 7) & 0x7c;
	g = (pixel >> 2) & 0x3e;
	b = (pixel << 3) & 0x1f;
}

// cppcheck-suppress unusedFunction
static void pixelToRgb16bitLSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel >> 8) & 0xf8;
	g = (pixel >> 3) & 0x7e;
	b = (pixel << 3) & 0x1f;
}

// cppcheck-suppress unusedFunction
static void pixelToRgb24bitLSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel >> 16) & 0xff;
	g = (pixel >> 8) & 0xff;
	b = pixel & 0xff;
}

// cppcheck-suppress unusedFunction
static void pixelToRgb15bitMSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel << 3) & 0x1f;
	g = (pixel >> 2) & 0x3e;
	b = (pixel >> 7) & 0x7c;
}

// cppcheck-suppress unusedFunction
static void pixelToRgb16bitMSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = (pixel << 3) & 0x1f;
	g = (pixel >> 3) & 0x7e;
	b = (pixel >> 8) & 0xf8;
}

// cppcheck-suppress unusedFunction
static void pixelToRgb24bitMSB(ulong pixel, uchar &r, uchar &g, uchar &b)
{
	r = pixel & 0xff;
	g = (pixel >> 8) & 0xff;
	b = (pixel >> 16) & 0xff;
}

// cppcheck-suppress unusedFunction
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
getPixelToRgbFun(XImage *ximage)
{
	if  ((ximage->red_mask == 0xff0000)
	     && (ximage->green_mask == 0xff00)
	     && (ximage->blue_mask == 0xff)) {
		return pixelToRgb24bitLSB;
	} else if ((ximage->red_mask == 0xf800)
		   && (ximage->green_mask == 0x07e0)
		   && (ximage->blue_mask == 0x001f)) {
		return pixelToRgb16bitLSB;
	} else if ((ximage->red_mask == 0x7c00)
		   && (ximage->green_mask == 0x3e0)
		   && (ximage->blue_mask == 0x1f)) {
		return pixelToRgb15bitLSB;
	} else if  ((ximage->red_mask == 0xff)
		    && (ximage->green_mask == 0xff00)
		    && (ximage->blue_mask == 0xff0000)) {
		return pixelToRgb24bitMSB;
	} else if ((ximage->red_mask == 0x001f)
		   && (ximage->green_mask == 0x07e0)
		   && (ximage->blue_mask == 0xf800)) {
		return pixelToRgb16bitMSB;
	} else if ((ximage->red_mask == 0x1f)
		   && (ximage->green_mask == 0x3e0)
		   && (ximage->blue_mask == 0x7c00)) {
		return pixelToRgb15bitMSB;
	} else {
		return pixelToRgbUnknown;
	}
}

PImage::PImage(void)
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
	  _use_alpha(false)
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
	  _use_alpha(image->_use_alpha)
{
	_data = new uchar[_width * _height];
	memcpy(_data, image->getData(), _width * _height);
}

/**
 * Create PImage from XImage.
 */
PImage::PImage(XImage *image, uchar opacity)
	: _type(IMAGE_TYPE_FIXED),
	  _pixmap(None),
	  _mask(None),
	  _width(image->width),
	  _height(image->height),
	  _data(new uchar[image->width * image->height * 4]),
	  _use_alpha(false)
{
	pixelToRgb toRgb = getPixelToRgbFun(image);

	uint dst = 0;
	for (uint y = 0; y < _height; ++y) {
		for (uint x = 0; x < _width; ++x) {
			_data[dst] = opacity; // A
			toRgb(XGetPixel(image, x, y),
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
PImage::draw(Render &rend, int x, int y, size_t width, size_t height)
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
			drawAlphaFixed(rend, x, y, width, height, _data);
		} else {
			drawFixed(rend, x, y, width, height);
		}
	} else if (_type == IMAGE_TYPE_SCALED) {
		if (_use_alpha) {
			drawAlphaScaled(rend, x, y, width, height);
		} else {
			drawScaled(rend, x, y, width, height);
		}
	} else if (_type == IMAGE_TYPE_TILED) {
		if (_use_alpha) {
			drawAlphaTiled(rend, x, y, width, height);
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
PImage::getPixmap(bool &need_free, size_t width, size_t height)
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
		uchar *scaled_data = getScaledData(width, height);
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
PImage::getMask(bool &need_free, size_t width, size_t height)
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
		uchar *scaled_data = getScaledData(width, height);
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
PImage::scale(size_t width, size_t height)
{
	// Invalid width or height or no need to scale.
	if (! width || ! height || ((width == _width) && (height == _height))) {
		return;
	}

	uchar *scaled_data = getScaledData(width, height);
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
PImage::drawAlphaFixed(Render &rend, int x, int y, size_t width, size_t height,
		       uchar* data)
{
	XImage *dest_image = rend.getImage(x, y, width, height);
	if (! dest_image) {
		if (Debug::isLevel(Debug::LEVEL_ERR)) {
			std::ostringstream msg;
			msg << "failed to get image for area ";
			msg << " x " << x << " y " << y;
			msg << " width " << width << " height " << height;
			P_ERR(msg.str());
		}
		return;
	}

	drawAlphaFixed(dest_image, dest_image, x, y, width, height, data);

	rend.putImage(dest_image, x, y, width, height);
	rend.destroyImage(dest_image);
}

void
PImage::drawAlphaFixed(XImage *src_image, XImage *dest_image,
		       int x, int y, size_t width, size_t height,
		       uchar* data)
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
	for (size_t i_y = 0; i_y < height; ++i_y) {
		for (size_t i_x = 0; i_x < width; ++i_x) {
			// Get pixel value, copy them directly if alpha is set
			// to 255.
			uchar a = *src++;
			uchar r = *src++;
			uchar g = *src++;
			uchar b = *src++;

			// Alpha not 100% solid, blend
			if (a != 255) {
				// Get RGB values from pixel.
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
PImage::drawFixed(Render &rend, int x, int y, size_t width, size_t height)
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
PImage::drawScaled(Render &rend, int x, int y, size_t width, size_t height)
{
	// Create scaled representation of image.
	uchar *scaled_data = getScaledData(width, height);
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
PImage::drawTiled(Render &rend, int x, int y, size_t width, size_t height)
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
PImage::drawAlphaScaled(Render &rend, int x, int y, size_t width, size_t height)
{
	uchar *scaled_data = getScaledData(width, height);
	if (scaled_data) {
		drawAlphaFixed(rend, x, y, width, height, scaled_data);
		delete [] scaled_data;
	}
}

struct ImageRenderAndData
{
	ImageRenderAndData(PImage *_image, Render &_rend, uchar *_data)
		: image(_image),
		  rend(_rend),
		  data(_data)
	{
	}

	PImage *image;
	Render &rend;
	uchar *data;
};

static void
renderWithAlphaFixed(int x, int y, uint width, uint height, void *opaque)
{
	ImageRenderAndData *irad =
		reinterpret_cast<ImageRenderAndData*>(opaque);
	irad->image->drawAlphaFixed(irad->rend,
				    x, y, width, height, irad->data);
}

/**
 * Draw image tiled to fit width and height.
 */
void
PImage::drawAlphaTiled(Render &rend, int x, int y, size_t width, size_t height)
{
	ImageRenderAndData irad(this, rend, _data);
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
PImage::createPixmap(uchar* data, size_t width, size_t height)
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
PImage::createMask(uchar* data, size_t width, size_t height)
{
	if (! _use_alpha) {
		return None;
	}

	// Create XImage
	XImage *ximage = XCreateImage(X11::getDpy(), X11::getVisual(),
				      1, ZPixmap, 0, 0, width, height, 32, 0);
	if (! ximage) {
		P_ERR("failed to create XImage " << width << "x" << height);
		return None;
	}

	// Alocate ximage data storage.
	ximage->data = new char[ximage->bytes_per_line * height];

	uchar *src = data;

	ulong pixel_trans = X11::getBlackPixel();
	ulong pixel_solid = X11::getWhitePixel();
	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			XPutPixel(ximage, x, y,
				  (*src > 127) ? pixel_solid : pixel_trans);
			src += 4; // Skip A, R, G, and B
		}
	}

	Pixmap pix = X11::createPixmapMask(width, height);
	GC gc = XCreateGC(X11::getDpy(), pix, 0, 0);
	X11::putImage(pix, gc, ximage, 0, 0, 0, 0, width, height);
	XFreeGC(X11::getDpy(), gc);

	delete [] ximage->data;
	ximage->data = 0;
	X11::destroyImage(ximage);

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
PImage::createXImage(uchar* data, size_t width, size_t height)
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
	for (size_t y = 0; y < height; ++y) {
		for (size_t x = 0; x < width; ++x) {
			// skip alpha
			src++;
			uchar r = *src++;
			uchar g = *src++;
			uchar b = *src++;
			XPutPixel(ximage, x, y, toPixel(r, g, b));
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
PImage::getScaledData(size_t dwidth, size_t dheight)
{
	if (dwidth < 1 || dheight < 1) {
		return nullptr;
	}

	uchar *scaled_data = new uchar[dwidth * dheight * 4] ;
	float x_ratio = static_cast<float>(_width - 1) / dwidth ;
	float y_ratio = static_cast<float>(_height - 1) / dheight;

	size_t dst = 0 ;
	for (size_t dy = 0; dy < dheight; dy++) {
		for (size_t dx = 0; dx < dwidth; dx++) {
			size_t sx = static_cast<size_t>(x_ratio * dx);
			size_t sy = static_cast<size_t>(y_ratio * dy);

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
