//
// PTexturePlain.hh for pekwm
// Copyright © 2004-2009 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//
// $Id$
//

#ifndef _PTEXTURE_PLAIN_HH_
#define _PTEXTURE_PLAIN_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <string>

#include "pekwm.hh"
#include "PImage.hh"

// PTextureSolid

class PTextureSolid : public PTexture {
public:
    PTextureSolid(Display *dpy, const std::string &color);
    virtual ~PTextureSolid(void);

    // START - PTexture interface.
    virtual void render(Drawable draw, int x, int y, uint width, uint height);
    // END - PTexture interface.

    inline XColor *getColor(void) { return _xc; }
    bool setColor(const std::string &color);
    void unsetColor(void);


private:
    GC _gc;
    XColor *_xc;
};

// PTextureSolidRaised

class PTextureSolidRaised : public PTexture {
public:
    PTextureSolidRaised(Display *dpy, const std::string &base, const std::string &hi, const std::string &lo);
    virtual ~PTextureSolidRaised(void);

    // START - PTexture interface.
    virtual void render(Drawable draw, int x, int y, uint width, uint height);
    // END - PTexture interface.

    inline void setLineOff(uint loff) { _loff = loff; _loff2 = loff * 2; }
    inline void setDraw(bool top, bool bottom, bool left, bool right) {
        _draw_top = top;
        _draw_bottom = bottom;
        _draw_left = left;
        _draw_right = right;
    }

    void setLineWidth(uint lw);
    bool setColor(const std::string &base, const std::string &hi, const std::string &lo);
    void unsetColor(void);

private:
    GC _gc;

    XColor *_xc_base, *_xc_hi, *_xc_lo;

    uint _lw, _loff, _loff2;
    bool _draw_top;
    bool _draw_bottom;
    bool _draw_left;
    bool _draw_right;
};

// PTextureImage

class PTextureImage : public PTexture {
public:
    PTextureImage(Display *dpy);
    PTextureImage(Display *dpy, const std::string &image);
    virtual ~PTextureImage(void);

    // START - PTexture interface.
    virtual void render(Drawable draw, int x, int y, uint width, uint height);
    virtual Pixmap getMask(uint width, uint height, bool &do_free);
    // END - PTexture interface.

    bool setImage(const std::string &image);
    void setImage(PImage *image);
    void unsetImage(void);

private:
    PImage *_image;
};

#endif // _PTEXTURE_PLAIN_HH_
