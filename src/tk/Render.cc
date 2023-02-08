//
// Render.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Render.hh"

extern "C" {
#include <assert.h>
}

// Render

void
renderTiled(const int a_x, const int a_y,
	    const uint a_width, const uint a_height,
	    const uint r_width, const uint r_height,
	    render_fun render, void *opaque)
{
	assert(r_width);
	assert(r_height);

	int x, y;
	uint width, height;

	y = a_y;
	height = a_height;
	while (height > r_height) {
		x = a_x;
		width = a_width;
		while (width > r_width) {
			render(x, y, r_width, r_height, opaque);
			x += r_width;
			width -= r_width;
		}
		render(x, y, width, r_height, opaque);

		y += r_height;
		height -= r_height;
	}

	x = a_x;
	width = a_width;
	while (width > r_width) {
		render(x, y, r_width, height, opaque);
		x += r_width;
		width -= r_width;
	}
	render(x, y, width, height, opaque);
}

// Render
Render::Render(void)
{
}

Render::~Render(void)
{
}

// X11Render

X11Render::X11Render(Drawable draw, Pixmap background)
	: _draw(draw),
	  _background(background),
	  _gc(X11::getGC())
{
}

X11Render::~X11Render(void)
{
}

Drawable
X11Render::getDrawable(void) const
{
	return _draw;
}

XImage*
X11Render::getImage(int x, int y, uint width, uint height)
{
	return X11::getImage(_draw, x, y, width, height, AllPlanes, ZPixmap);
}

void
X11Render::setColor(int pixel)
{
	XSetForeground(X11::getDpy(), _gc, pixel);
}

void
X11Render::setLineWidth(int lw)
{
	XGCValues gv;
	gv.line_width = lw < 1 ? 1 : lw;
	XChangeGC(X11::getDpy(), X11::getGC(), GCLineWidth, &gv);
}

void
X11Render::line(int x1, int y1, int x2, int y2)
{
	XDrawLine(X11::getDpy(), _draw, _gc, x1, y1, x2, y2);
}

void
X11Render::rectangle(int x, int y, uint width, uint height)
{
	XDrawRectangle(X11::getDpy(), _draw, _gc, x, y, width, height);
}

void
X11Render::clear(int x, int y, uint width, uint height)
{
	if (_background == None) {
		XClearArea(X11::getDpy(), _draw, x, y, width, height, False);
	} else {
		X11::copyArea(_background, _draw, x, y, width, height, x, y);
	}
}

void
X11Render::fill(int x, int y, uint width, uint height)
{
	XFillRectangle(X11::getDpy(), _draw, _gc, x, y, width, height);
}

void
X11Render::putImage(XImage *image, int dest_x, int dest_y,
		    uint width, uint height)
{
	X11::putImage(_draw, X11::getGC(), image, 0, 0,
		      dest_x, dest_y, width, height);
}

void
X11Render::destroyImage(XImage *image)
{
	X11::destroyImage(image);
}


// XImageRender

XImageRender::XImageRender(XImage *image)
	: _image(image),
	  _color(0),
	  _lw(1)
{
}

XImageRender::~XImageRender(void)
{
}

Drawable
XImageRender::getDrawable(void) const
{
	return None;
}

XImage*
XImageRender::getImage(int src_x, int src_y, uint width, uint height)
{
	XImage *image = X11::createImage(nullptr, width, height);
	if (image == nullptr) {
		return image;
	}
	image->data = new char[image->bytes_per_line * height];

	if (static_cast<int>(src_y + height) > _image->height) {
		height = std::min(height,
				  static_cast<uint>(_image->height - src_y));
	}
	if (static_cast<int>(src_x + width) > _image->width) {
		width = std::min(width,
				 static_cast<uint>(_image->width - src_x));
	}

	for (uint y = 0; y < height; y++) {
		for (uint x = 0; x < width; x++) {
			XPutPixel(image, x, y,
				  XGetPixel(_image, src_x + x, src_y + y));
		}
	}

	return image;
}

void
XImageRender::destroyImage(XImage *image)
{
	delete [] image->data;
	image->data = nullptr;
	X11::destroyImage(image);
}

void
XImageRender::setLineWidth(int lw)
{
	_lw = lw < 1 ? 1: lw;
}

void
XImageRender::setColor(int pixel)
{
	_color = pixel;
}

void
XImageRender::line(int x1, int y1, int x2, int y2)
{
	if (x1 == x2) {
		for (; y1 <= y2; y1++) {
			for (int lw = 0; lw < _lw; lw++) {
				XPutPixel(_image, x1, y1 + lw, _color);
			}
		}
	} else if (y1 == y2) {
		for (; x1 <= x2; x1++) {
			for (int lw = 0; lw < _lw; lw++) {
				XPutPixel(_image, x1 + lw, y1, _color);
			}
		}
	} else {
		// only horizontal and vertical lines are currently
		// supported.
	}
}

void
XImageRender::rectangle(int x, int y, uint width, uint height)
{
	int rx = x + width;
	int by = y + height;
	line(x, y, rx, y);
	line(x, by, rx, by);
	line(x, y, x, by);
	line(rx, y, rx, by);
}

void
XImageRender::clear(int x0, int y, uint width, uint height)
{
	for (; height; y++, height--) {
		for (uint x = x0; x < width; x++) {
			XPutPixel(_image, x, y, 0);
		}
	}
}

void
XImageRender::fill(int x0, int y0, uint width, uint height)
{
	x0 = std::max(0, x0);
	y0 = std::max(0, y0);
	for (int y = y0; y < static_cast<int>(height); y++) {
		for (int x = x0; x < static_cast<int>(width); x++) {
			XPutPixel(_image, x, y, _color);
		}
	}
}

void
XImageRender::putImage(XImage *image, int dest_x, int dest_y,
		       uint width, uint height)
{
	int max_y =
		std::min(static_cast<int>(height), dest_y + _image->height);
	int max_x =
		std::min(static_cast<int>(width), dest_x + _image->width);
	for (int y = 0; y < max_y; y++) {
		for (int x = 0; x < max_x; x++) {
			XPutPixel(_image,
				  dest_x + x,
				  dest_y + y,
				  XGetPixel(image, x, y));
		}
	}
}
