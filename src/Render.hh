//
// Render.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"
#include "X11.hh"
#include "pekwm.hh"

typedef void(*render_fun)(int, int, uint, uint, void *opaque);

void renderTiled(const int a_x, const int a_y,
                 const uint a_width, const uint a_height,
                 const uint r_width, const uint r_height,
                 render_fun render,
                 void *opaque);

/**
 * Graphics render interface, used for textures and images.
 */
class Render {
public:
    Render(void);
    virtual ~Render(void);

    virtual Drawable getDrawable(void) const = 0;
    virtual XImage *getImage(int x, int y, uint width, uint height) = 0;
    virtual void destroyImage(XImage *image) = 0;

    virtual void setColor(int pixel) = 0;
    virtual void setLineWidth(int lw) = 0;

    virtual void clear(int x, int y, uint width, uint height) = 0;
    virtual void line(int x1, int y1, int x2, int y2) = 0;
    virtual void rectangle(int x0, int y, uint width, uint height) = 0;
    virtual void fill(int x, int y, uint width, uint height) = 0;
    virtual void putImage(XImage *image, int dest_x, int dest_y,
                          uint width, uint height) = 0;
};

/**
 * Renderer using X11 primites for rendering onto a Drawable.
 */
class X11Render : public Render {
public:
    X11Render(Drawable draw);
    virtual ~X11Render(void);

    virtual Drawable getDrawable(void) const;
    virtual XImage *getImage(int x, int y, uint width, uint height);
    virtual void destroyImage(XImage *image);

    virtual void setColor(int pixel);
    virtual void setLineWidth(int lw);

    virtual void clear(int x, int y, uint width, uint height);
    virtual void line(int x1, int y1, int x2, int y2);
    virtual void rectangle(int x0, int y, uint width, uint height);
    virtual void fill(int x, int y, uint width, uint height);
    virtual void putImage(XImage *image, int dest_x, int dest_y,
                          uint width, uint height);

private:
    Drawable _draw;
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

    virtual void clear(int x, int y, uint width, uint height);
    virtual void line(int x1, int y1, int x2, int y2);
    virtual void rectangle(int x0, int y, uint width, uint height);
    virtual void fill(int x0, int y, uint width, uint height);
    virtual void putImage(XImage *image, int dest_x, int dest_y,
                          uint width, uint height);

private:
    XImage *_image;
    int _color;
    int _lw;
};
