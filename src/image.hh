//
// image.hh for pekwm
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

#ifndef _IMAGE_HH_
#define _IMAGE_HH_

#include <string>
#include <X11/Xlib.h>
#include <X11/xpm.h>

class Image
{
public:
	enum ImageType {
		TILED = 1, SCALED, TRANSPARENT, NO_TYPE = 0
	};

	Image(Display *d);
	~Image();

	bool load(std::string file);
	void unload(void);

	void draw(Drawable dest, int x, int y,
						unsigned int width = 0, unsigned int height = 0);
	void scale(unsigned int width, unsigned int height);
	Image* getScaled(unsigned int width, unsigned int height);

	inline Pixmap getPixmap(void) { return m_pixmap; }

	inline unsigned int getWidth(void) const { return m_width; }
	inline unsigned int getHeight(void) const { return m_height; }
	inline ImageType getType(void) const { return m_image_type; }

	inline void setImageType(ImageType it) { m_image_type = it; }

private:
	inline void setWidth(unsigned int w) { m_width = w; }
	inline void setHeight(unsigned int h) { m_height = h; }

	inline XpmImage* getXpmImage(void) { return &m_xpm_image; }

	inline void setPixmap(Pixmap p) { m_pixmap = p; }

private:
	Display *dpy;

	XpmImage m_xpm_image;

	Pixmap m_pixmap, m_shape_pixmap;

	unsigned int m_width;
	unsigned int m_height;

	ImageType m_image_type;
};

#endif // _IMAGE_HH_
