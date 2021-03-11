//
// Render.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "X11.hh"

#include <functional>

void renderTiled(const int a_x, const int a_y,
                 const uint a_width, const uint a_height,
                 const uint r_width, const uint r_height,
                 std::function<void(int, int, uint, uint)> render);

/**
 * Graphics render interface, used for textures and images.
 */
class Render {
public:
    virtual ~Render(void) { }

    virtual Drawable getDrawable(void) const = 0;
    virtual XImage *getImage(int x, int y, uint width, uint height) = 0;
    virtual void destroyImage(XImage *image) = 0;

    virtual void setColor(int pixel) = 0;
    virtual void setLineWidth(int lw) = 0;

    virtual void line(int x1, int y1, int x2, int y2) = 0;
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
    virtual ~X11Render(void) { }

    virtual Drawable getDrawable(void) const override;
    virtual XImage *getImage(int x, int y, uint width, uint height) override;
    virtual void destroyImage(XImage *image) override;

    virtual void setColor(int pixel) override;
    virtual void setLineWidth(int lw) override;

    virtual void line(int x1, int y1, int x2, int y2) override;
    virtual void fill(int x, int y, uint width, uint height) override;
    virtual void putImage(XImage *image, int dest_x, int dest_y,
                          uint width, uint height) override;

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
    virtual ~XImageRender(void) { }

    virtual Drawable getDrawable(void) const override;
    virtual XImage *getImage(int x, int y, uint width, uint height) override;
    virtual void destroyImage(XImage *image) override;

    virtual void setLineWidth(int lw) override;
    virtual void setColor(int pixel) override;

    virtual void line(int x1, int y1, int x2, int y2) override;
    virtual void fill(int x0, int y, uint width, uint height) override;
    virtual void putImage(XImage *image, int dest_x, int dest_y,
                          uint width, uint height) override;

private:
    XImage *_image;
    int _color;
    int _lw;
};
