//
// PTexture.hh for pekwm
// Copyright (C) 2004-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PTEXTURE_HH_
#define _PEKWM_PTEXTURE_HH_

#include "config.h"

#include "PSurface.hh"
#include "Render.hh"
#include "X11.hh"

class PTexture {
public:
	enum Type {
		TYPE_SOLID,
		TYPE_SOLID_RAISED,
		TYPE_LINES_HORZ,
		TYPE_LINES_VERT,
		TYPE_IMAGE,
		TYPE_IMAGE_MAPPED,
		TYPE_EMPTY,
		TYPE_NO
	};

	PTexture(void);
	virtual ~PTexture(void);

	void render(Drawable draw,
		    int x, int y, size_t width, size_t height,
		    int root_x=0, int root_y=0);
	void render(PSurface *surface,
		    int x, int y, size_t width, size_t height,
		    int root_x=0, int root_y=0);

	void render(Render &rend,
		    int x, int y, size_t width, size_t height,
		    int root_x=0, int root_y=0);
	virtual void doRender(Render &rend,
			      int x, int y, size_t width, size_t height) = 0;
	virtual bool getPixel(ulong &pixel) const = 0;
	virtual Pixmap getMask(size_t, size_t, bool&) { return None; }

	void setBackground(Drawable draw,
			   int x, int y, size_t width, size_t height);

	bool isOk(void) const { return _ok; }
	size_t getWidth(void) const;
	size_t getHeight(void) const;
	PTexture::Type getType(void) const { return _type; }

	void setWidth(size_t width) { _width = width; }
	void setHeight(size_t height) { _height = height; }

	uchar getOpacity(void) const { return _opacity; }
	void setOpacity(uchar opacity) { _opacity = opacity; }

private:
	bool renderOnBackground(XImage *src_ximage,
				int x, int y, size_t width, size_t height,
				int root_x, int root_y);

protected:
	bool _ok; // Texture successfully loaded
	size_t _width;
	size_t _height;
	PTexture::Type _type; // Type of texture
	uchar _opacity; // Texture opacity, blended onto background pixmap
};

#endif // _PEKWM_PTEXTURE_HH_
