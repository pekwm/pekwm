//
// Image.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "Image.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::string;

Image::Image(Display *d) :
_dpy(d),
_pixmap(None), _shape_pixmap(None),
_width(0), _height(0),
_image_type(IMAGE_TILED)
{
	_xpm_image.data = NULL; // so that we can track if it's loaded or not
}

Image::~Image()
{
	unload();
}

//! @fn    bool load(string file)
//! @brief Loads an xpm image into the image
//! @param file File to load
//! @return true if image was successfully loaded, else false.
bool
Image::load(string file)
{
	if (!file.size())
		return false;

	// Unload allready loaded image
	unload();

	// Setup loading attributes attributes
	XpmAttributes attr;
	attr.valuemask = XpmSize;

	int status =
		XpmReadFileToPixmap(_dpy, DefaultRootWindow(_dpy), (char *) file.c_str(),
												&_pixmap, &_shape_pixmap, &attr);

	// Image loaded okay, now let's convert it into a XImage and setup the sizes
	if (status == XpmSuccess) {
		_width = attr.width;
		_height = attr.height;

		// move the pixmap into a xpmimage
		XpmCreateXpmImageFromPixmap(_dpy, _pixmap, _shape_pixmap,
																&_xpm_image, NULL);
		return true;
	}

	return false;
}

//! @fn    void unload(void)
//! @brief Unloads the image
void
Image::unload(void)
{
	if (_xpm_image.data) {
		XpmFreeXpmImage(&_xpm_image);
		_xpm_image.data = NULL;
	}
	if (_pixmap != None) {
		XFreePixmap(_dpy, _pixmap);
		_pixmap = None;
	}
	if (_shape_pixmap != None) {
		XFreePixmap(_dpy, _shape_pixmap);
		_shape_pixmap = None;
	}

	_width = _height = 0;
}

//! @fn    void draw(Drawable dest, int x, int y,
//!                  unsigned int width unsigned int height)
//! @brief Draws the image on the Drawable dest
void
Image::draw(Drawable dest, int x, int y,
						unsigned int width, unsigned int height)
{
	if (!_xpm_image.data)
		return;

	// Can't have 0 wide or tall thing
	if (width == 0)
		width = _width;
	if (height == 0)
		height = _height;

	// Check for "sane" sizes
	if (width > 4096)
		width = 4096;
	if (height > 4096)
		height = 4096;

	if (_image_type == IMAGE_SCALED) {
		Image *image = getScaled(width, height);
		if (image) {
			XCopyArea(_dpy, image->_pixmap, dest,
								DefaultGC(_dpy, DefaultScreen(_dpy)),
								0, 0, width, height, x, y);
			delete image;
		}
	} else {
		XGCValues gv;
		gv.fill_style = FillTiled;
		gv.tile = _pixmap;
		gv.ts_x_origin = x; // to make the tile start at the correct position
		gv.ts_y_origin = y;

		GC gc = XCreateGC(_dpy, dest, GCTile|GCFillStyle|GCTileStipXOrigin, &gv);
		XFillRectangle(_dpy, dest, gc, x, y, width, height);

		XFreeGC(_dpy, gc);
	}
}

//! @fn    void scale(unsigned int width, unsigned int height)
//! @brief Scales the image permanent.
void
Image::scale(unsigned int width, unsigned int height)
{
	// TO-DO: Permanent scaling
}

//! @fn    Image* getScaled(unsigned int width, unsigned int height)
//! @brief Returns a scaled version of the Image
//! @todo Put the Destination XpmImage back.
Image*
Image::getScaled(unsigned int width, unsigned int height)
{
	if (!_xpm_image.data)
		return NULL;

	// setup destination image
	XpmImage dest;
	dest.width = width;
	dest.height = height;
	dest.cpp = _xpm_image.cpp;
	dest.ncolors = _xpm_image.ncolors;
	dest.colorTable = _xpm_image.colorTable;
	dest.data = new unsigned int[width * height];

	// scale ratios
	float rx = float(_width) / float(width);
	float ry = float(_height) / float(height);

	// counters
	float sx, sy;
	unsigned int x, y, loff = 0;

	unsigned int *data = dest.data;
	// scale the image
	for (y = 0, sy = 0; y < height; ++y, sy += ry) {
		loff = ((int) sy) * _width;
		for (x = 0, sx = loff; x < width; ++x, sx += rx) {
			*data++ = _xpm_image.data[(int) sx];
		}
	}

	// now create the new scaled pixmap
	Image *image = new Image(_dpy);
	XpmCreatePixmapFromXpmImage(_dpy, DefaultRootWindow(_dpy), &dest,
															&image->_pixmap, &image->_shape_pixmap, NULL);
	image->_width = width;
	image->_height = height;

	// free memory
	delete [] dest.data;
	return image;
}
