//
// Render.hh for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_RENDER_HH_
#define _PEKWM_RENDER_HH_

#include "config.h"
#include "PSurface.hh"
#include "X11.hh"

#include <vector>

typedef void(*render_fun)(int, int, uint, uint, void *opaque);

void renderTiled(const int a_x, const int a_y,
		 const uint a_width, const uint a_height,
		 const uint r_width, const uint r_height,
		 render_fun render,
		 void *opaque);

class point_vector : public std::vector<XPoint> {
public:
	point_vector() { }
	virtual ~point_vector() { }

	void add(short x, short y)
	{
		XPoint point = { x, y };
		push_back(point);
	}
};

enum RenderFillStyle {
	RENDER_FILL_SOLID,
	RENDER_FILL_CHECKER
};

/**
 * Graphics render interface, used for textures and images.
 */
class Render {
public:
	Render();
	virtual ~Render();

	virtual Drawable getDrawable(void) const = 0;
	virtual XImage *getImage(int x, int y, uint width, uint height) = 0;
	virtual void destroyImage(XImage *image) = 0;

	virtual void setColor(int pixel) = 0;
	virtual void setFillStyle(enum RenderFillStyle style) = 0;
	virtual void setLineWidth(int lw) = 0;
	virtual void setClip(short x, short y, ushort width, ushort height) = 0;
	virtual void clearClip() = 0;

	virtual void clear(int x, int y, uint width, uint height) = 0;
	virtual void line(int x1, int y1, int x2, int y2) = 0;
	virtual void rectangle(int x0, int y, uint width, uint height) = 0;
	virtual void fill(int x, int y, uint width, uint height) = 0;
	virtual void poly(const std::vector<XPoint> &points, bool fill) = 0;
	virtual void putImage(XImage *image, int dest_x, int dest_y,
			      uint width, uint height) = 0;
};

/**
 * Render backed surface.
 */
class RenderSurface : public PSurface {
public:
	RenderSurface(Render& render, const Geometry& gm)
		: PSurface(),
		  _render(render),
		  _gm(gm)
	{
	}

	virtual ~RenderSurface()
	{
	}

	virtual Drawable getDrawable() const { return _render.getDrawable(); }

	virtual int getX() const { return _gm.x; }
	virtual int getY() const { return _gm.y; }
	virtual uint getWidth() const { return _gm.width; }
	virtual uint getHeight() const { return _gm.height; }

private:
	Render& _render;
	Geometry _gm;
};

/**
 * Renderer using X11 primites for rendering onto a Drawable.
 */
class X11Render : public Render {
public:
	X11Render(Drawable draw, Pixmap background=None);
	virtual ~X11Render(void);

	virtual Drawable getDrawable(void) const;
	virtual XImage *getImage(int x, int y, uint width, uint height);
	virtual void destroyImage(XImage *image);

	virtual void setColor(int pixel);
	virtual void setFillStyle(enum RenderFillStyle style);
	virtual void setLineWidth(int lw);
	virtual void setClip(short x, short y, ushort width, ushort height);
	virtual void clearClip();

	virtual void clear(int x, int y, uint width, uint height);
	virtual void line(int x1, int y1, int x2, int y2);
	virtual void rectangle(int x0, int y, uint width, uint height);
	virtual void fill(int x, int y, uint width, uint height);
	virtual void poly(const std::vector<XPoint> &points, bool fill);
	virtual void putImage(XImage *image, int dest_x, int dest_y,
			      uint width, uint height);

private:
	Drawable _draw;
	Pixmap _background;
	GC _gc;
};

/**
 * Renderer using XImage APIs for rendering onto a XImage.
 */
class XImageRender : public Render {
public:
	XImageRender(XImage *image);
	virtual ~XImageRender(void);

	virtual Drawable getDrawable(void) const;
	virtual XImage *getImage(int x, int y, uint width, uint height);
	virtual void destroyImage(XImage *image);

	virtual void setLineWidth(int lw);
	virtual void setColor(int pixel);
	virtual void setFillStyle(enum RenderFillStyle style);
	virtual void setClip(short x, short y, ushort width, ushort height);
	virtual void clearClip();

	virtual void clear(int x, int y, uint width, uint height);
	virtual void line(int x1, int y1, int x2, int y2);
	virtual void rectangle(int x0, int y, uint width, uint height);
	virtual void fill(int x0, int y, uint width, uint height);
	virtual void poly(const std::vector<XPoint> &points, bool fill);
	virtual void putImage(XImage *image, int dest_x, int dest_y,
			      uint width, uint height);

private:
	void fillChecker(int x0, int y, uint width, uint height);

private:
	XImage *_image;
	int _color;
	int _lw;
	enum RenderFillStyle _fill_style;
};

#endif // _PEKWM_RENDER_HH_
