//
// PTexture.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PTexture.hh"
#include "PImage.hh"
#include "X11.hh"

PTexture::PTexture(void)
	: _ok(false),
	  _width(0),
	  _height(0),
	  _opacity(255)
{
}

PTexture::~PTexture(void)
{
}

/**
 * Render texture onto drawable.
 */
void
PTexture::render(Drawable draw,
		 int x, int y, size_t width, size_t height,
		 int root_x, int root_y)
{
	X11Render rend(draw);
	render(rend, x, y, width, height, root_x, root_y);
}

/**
 * Render texture onto PSurface
 */
void
PTexture::render(PSurface *surface,
		 int x, int y, size_t width, size_t height,
		 int root_x, int root_y)
{
	X11Render rend(surface->getDrawable());
	render(rend, x, y, width, height, root_x, root_y);
}

/**
 * Render texture using render.
 */
void
PTexture::render(Render &rend,
		 int x, int y, size_t width, size_t height,
		 int root_x, int root_y)
{
	if (width == 0) {
		width = _width;
	}
	if (height == 0) {
		height = _height;
	}

	if (! width || ! height) {
		return;
	}

	if (_opacity == 255) {
		doRender(rend, x, y, width, height);
	} else {
		char *data = static_cast<char*>(malloc(width * height * 4));
		XImage *ximage = X11::createImage(data, width, height);
		if (ximage) {
			XImageRender x_rend(ximage);
			doRender(x_rend, 0, 0, width, height);
			if (renderOnBackground(ximage, x, y, width, height,
					       root_x, root_y)) {
				X11::putImage(rend.getDrawable(),
					      X11::getGC(), ximage,
					      0, 0, 0, 0, width, height);
			}
			X11::destroyImage(ximage);
		}
	}
}

/**
 * Set background on drawable using the current texture, for
 * solid/empty textures this sets the background pixel instead of
 * rendering a Pixmap.
 */
void
PTexture::setBackground(Drawable draw,
			int x, int y, size_t width, size_t height)
{
	ulong pixel;
	if (getPixel(pixel) && _opacity == 255) {
		// set background pixel
		X11::setWindowBackground(draw, pixel);
	} else if (width > 0 && height > 0) {
		Pixmap pix = X11::createPixmap(width, height);
		render(pix, x, y, width, height);
		X11::setWindowBackgroundPixmap(draw, pix);
		X11::freePixmap(pix);
	}
}

size_t
PTexture::getWidth(void) const
{
	return _width;
}

size_t
PTexture::getHeight(void) const
{
	return _height;
}

bool
PTexture::renderOnBackground(XImage *ximage,
			     int x, int y, size_t width, size_t height,
			     int root_x, int root_y)
{
	Cardinal pix;
	if (! X11::getCardinal(X11::getRoot(), XROOTPMAP_ID, pix, XA_PIXMAP)) {
		return false;
	}

	// read background pixmap for area
	XImage *root_ximage = X11::getImage(pix, root_x, root_y, width, height,
					    AllPlanes, ZPixmap);
	if (root_ximage == nullptr) {
		return false;
	}

	PImageData image(ximage, _opacity);
	PImageData::drawAlphaFixed(root_ximage, ximage, x, y, width, height,
				   image.getData());
	X11::destroyImage(root_ximage);

	return true;
}
