//
// PTexture.hh for pekwm
// Copyright (C) 2004-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"
#include "x11.hh"

class Render {
public:
    virtual ~Render(void) { }

    virtual Drawable getDrawable(void) const = 0;

    virtual void setColor(int pixel) = 0;
    virtual void setLineWidth(int lw) = 0;

    virtual void line(int x1, int y1, int x2, int y2) = 0;
    virtual void fill(int x, int y, uint width, uint height) = 0;
};

class X11Render : public Render {
public:
    X11Render(Drawable draw)
        : _draw(draw),
          _gc(X11::getGC())
    {
    }
    virtual ~X11Render(void) { }

    virtual Drawable getDrawable(void) const override {
        return _draw;
    };

    virtual void setColor(int pixel) override {
        XSetForeground(X11::getDpy(), _gc, pixel);
    }

    virtual void setLineWidth(int lw) override {
        XGCValues gv;
        gv.line_width = lw < 1 ? 1 : lw;
        XChangeGC(X11::getDpy(), X11::getGC(), GCLineWidth, &gv);
    }

    virtual void line(int x1, int y1, int x2, int y2) override {
        XDrawLine(X11::getDpy(), _draw, _gc, x1, y1, x2, y2);
    }

    virtual void fill(int x, int y, uint width, uint height) override {
        XFillRectangle(X11::getDpy(), _draw, _gc, x, y, width, height);
    }

private:
    Drawable _draw;
    GC _gc;
};

class XImageRender : public Render {
public:
    XImageRender(XImage *image)
        : _image(image),
          _color(0),
          _lw(1)
    {
    }
    virtual ~XImageRender(void) { }

    virtual Drawable getDrawable(void) const override {
        return None;
    }

    virtual void setLineWidth(int lw) override {
        _lw = lw < 1 ? 1: lw;
    }

    virtual void setColor(int pixel) override {
        _color = pixel;
    }

    virtual void line(int x1, int y1, int x2, int y2) override {
        if (x1 == x2) {
            for (; y1 <= y2; y1++) {
                XPutPixel(_image, x1, y1, _color);
            }
        } else if (y1 == y2) {
            for (; x1 <= x2; x1++) {
                XPutPixel(_image, x1, y1, _color);
            }
        } else {
            // only horizontal and vertical lines are currently
            // supported.
        }
    }

    virtual void fill(int x0, int y, uint width, uint height) override {
        for (; height; y++, height--) {
            for (uint x = x0; x < width; x++) {
                XPutPixel(_image, x, y, _color);
            }
        }
    }

private:
    XImage *_image;
    int _color;
    int _lw;
};

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

    PTexture()
        : _ok(false),
          _width(0),
          _height(0),
          _type(PTexture::TYPE_NO),
          _opacity(255)
    {
    }
    virtual ~PTexture(void)
    {
    }

    void render(Drawable draw,
                int x, int y, uint width, uint height,
                int root_x=0, int root_y=0);
    void render(Render &rend,
                int x, int y, uint width, uint height,
                int root_x=0, int root_y=0);
    virtual void doRender(Render &rend,
                          int x, int y, uint width, uint height) = 0;
    virtual bool getPixel(ulong &pixel) const = 0;
    virtual Pixmap getMask(uint width, uint height, bool &do_free) {
        return None;
    }

    void setBackground(Drawable draw,
                       int x, int y, uint width, uint height);

    bool isOk(void) const { return _ok; }
    uint getWidth(void) const { return _width; }
    uint getHeight(void) const { return _height; }
    PTexture::Type getType(void) const { return _type; }

    void setWidth(uint width) { _width = width; }
    void setHeight(uint height) { _height = height; }

    uchar getOpacity(void) const { return _opacity; }
    void setOpacity(uchar opacity) { _opacity = opacity; }

private:
    bool renderOnBackground(XImage *src_ximage,
                            int x, int y, uint width, uint height,
                            int root_x, int root_y);

protected:
    bool _ok; // Texture successfully loaded
    uint _width, _height; // for images etc, 0 for infinite like in stretch
    PTexture::Type _type; // Type of texture
    uchar _opacity; // Texture opacity, blended onto background pixmap
};
