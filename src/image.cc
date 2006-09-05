//
// image.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "image.hh"

using std::string;

Image::Image(Display *d) :
dpy(d),
m_pixmap(None), m_shape_pixmap(None),
m_width(0), m_height(0)
{
	m_xpm_image.data = NULL; // so that we can track if it's loaded or not
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
		XpmReadFileToPixmap(dpy, DefaultRootWindow(dpy), (char *) file.c_str(),
												&m_pixmap, &m_shape_pixmap, &attr);

	// Image loaded okay, now let's convert it into a XImage and setup the sizes
	if (status == XpmSuccess) {
		m_width = attr.width;
		m_height = attr.height; 

		// move the pixmap into a xpmimage
		XpmCreateXpmImageFromPixmap(dpy, m_pixmap, m_shape_pixmap,
																&m_xpm_image, NULL);
		return true;
	}

	return false;
}

//! @fn    void unload(void)
//! @brief Unloads the image
void
Image::unload(void)
{
	if (m_xpm_image.data) {
		XpmFreeXpmImage(&m_xpm_image);
		m_xpm_image.data = NULL;
	}
	if (m_pixmap != None) {
		XFreePixmap(dpy, m_pixmap);
		m_pixmap = None;
	}
	if (m_shape_pixmap != None) {
		XFreePixmap(dpy, m_shape_pixmap);
		m_shape_pixmap = None;
	}

	m_width = m_height = 0;
}

//! @fn    void draw(Drawable dest, int x, int y,
//!                  unsigned int width unsigned int height)
//! @brief Draws the image on the Drawable dest
void
Image::draw(Drawable dest, int x, int y,
						unsigned int width, unsigned int height)
{
	if (!m_xpm_image.data)
		return;

	// Can't have 0 wide or tall thing
	if (width == 0)
		width = m_width;
	if (height == 0)
		height = m_height;

	// Check for "sane" sizes
	if (width > 4096)
		width = 4096;
	if (height > 4096)
		height = 4096;

	// Setup a GC
	XGCValues gv;
	GC gc;

	gv.fill_style = FillTiled;

	Image *image = NULL;
	if (m_image_type == SCALED) {
		image = getScaled(width, height);
		if (image)
			gv.tile = image->getPixmap();
		else 
		 gv.tile = m_pixmap;

	} else {
		gv.tile = m_pixmap;
	}

	gc = XCreateGC(dpy, dest, GCTile|GCFillStyle, &gv);
	XFillRectangle(dpy, dest, gc, x, y, width, height);

	// Free memory
	XFreeGC(dpy, gc);
	if (image)
		delete image;
}

//! @fn
//! @brief
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
	if (!m_xpm_image.data)
		return NULL;

	// setup destination image
	XpmImage dest;
	dest.width = width;
	dest.height = height;
	dest.cpp = m_xpm_image.cpp;
	dest.ncolors = m_xpm_image.ncolors;
	dest.colorTable = m_xpm_image.colorTable;
	dest.data = new unsigned int[width * height];

	// scale ratios
	float rx = (float) m_width / (float) width;

	// counters
	float sx;
	unsigned int x, y, loff = 0;

	unsigned int *data = dest.data;
	// scale the image
 	for (y = 0; y < height; ++y, loff += m_width) {
 		for (x = 0, sx = loff; x < width; ++x, sx += rx) {
 			*data++ = m_xpm_image.data[(int) sx];
 		}
 	}

	// now create the new scaled pixmap
	Image *image = new Image(dpy);
	XpmCreatePixmapFromXpmImage(dpy, DefaultRootWindow(dpy), &dest,
															&image->m_pixmap, &image->m_shape_pixmap, NULL);
	image->setWidth(width);
	image->setHeight(height);

	// free the memory allocated
	delete [] dest.data;

	return image;
}
