//
// Image.hh for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _IMAGE_HH_
#define _IMAGE_HH_

#include "pekwm.hh"

#include <string>

extern "C" {
#include <X11/Xlib.h>
#include <X11/xpm.h>
}

class Image
{
public:

	Image(Display *d);
	~Image();

	inline Pixmap getPixmap(void) { return _pixmap; }

	inline unsigned int getWidth(void) const { return _width; }
	inline unsigned int getHeight(void) const { return _height; }
	inline ImageType getType(void) const { return _image_type; }

	inline void setImageType(ImageType it) { _image_type = it; }

	bool load(std::string file);
	void unload(void);

	void draw(Drawable dest, int x, int y,
						unsigned int width = 0, unsigned int height = 0);
	void scale(unsigned int width, unsigned int height);
	Image* getScaled(unsigned int width, unsigned int height);

private:
	Display *_dpy;

	XpmImage _xpm_image;

	Pixmap _pixmap, _shape_pixmap;

	unsigned int _width;
	unsigned int _height;

	ImageType _image_type;
};

#endif // _IMAGE_HH_
