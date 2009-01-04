//
// PImage.hh for pekwm
// Copyright © 2005-2008 Claes Nasten <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef _PIMAGE_HH_
#define _PIMAGE_HH_

#include "pekwm.hh"
#include "PImageLoader.hh"

#include <list>
#include <string>

//! @brief Image baseclass defining interface for image handling.
class PImage {
public:
    //! @brief PImage constructor.
    PImage(Display *dpy, const std::string &path="") throw(LoadException&);
    //! @brief PImage destructor.
    virtual ~PImage(void);

    //! @brief Add loader to loader list.
    static void loaderAdd(PImageLoader *loader) {
        _loader_list.push_back(loader);
    }
    //! @brief Removes and frees all loaders.
    static void loaderClear(void) {
        while (_loader_list.size()) {
            delete _loader_list.back();
            _loader_list.pop_back();
        }
    }

    //! @brief Returns type of image.
    inline ImageType getType(void) const { return _type; }
    //! @brief Sets type of image.
    inline void setType(ImageType type) { _type = type; }

    //! @brief Returns width of image.
    inline uint getWidth(void) const { return _width; }
    //! @brief Returns height of image.
    inline uint getHeight(void) const { return _height; }

    virtual bool load(const std::string &file);
    virtual void unload(void);
    virtual void draw(Drawable dest, int x, int y,
                      uint width = 0, uint height = 0);
    virtual Pixmap getPixmap(bool &need_free, uint width = 0, uint height = 0);
    virtual Pixmap getMask(bool &need_free, uint width = 0, uint height = 0);
    virtual void scale(uint width, uint height);

protected:
    void drawFixed(Drawable dest, int x, int y, uint width, uint height);
    void drawScaled(Drawable dest, int x, int y, uint widht, uint height);
    void drawTiled(Drawable dest, int x, int y, uint widht, uint height);
    void drawAlphaFixed(Drawable dest, int x, int y, uint widht, uint height,
                        uchar *data = 0);
    void drawAlphaScaled(Drawable dest, int x, int y, uint widht, uint height);
    void drawAlphaTiled(Drawable dest, int x, int y, uint widht, uint height);

    Pixmap createPixmap(uchar *data, uint width, uint height);
    Pixmap createMask(uchar *data, uint width, uint height);

private:
    XImage *createXImage(uchar *data, uint width, uint height);
    uchar *getScaledData(uint width, uint height);

    /**
     * Create pixel value suitable for ximage from R, G and B.
     */
    inline ulong
    getPixelFromRgb(XImage *ximage, uchar r, uchar g, uchar b)
    {
        // 5 R, 5 G, 5 B (15 bit display)
        if ((ximage->red_mask == 0x7c00)
            && (ximage->green_mask == 0x3e0) && (ximage->blue_mask == 0x1f)) {
            return ((r << 7) & 0x7c00)
                | ((g << 2) & 0x03e0) | ((b >> 3) & 0x001f);

            // 5 R, 6 G, 5 B (16 bit display)
        } else  if ((ximage->red_mask == 0xf800)
                    && (ximage->green_mask == 0x07e0)
                    && (ximage->blue_mask == 0x001f)) {
            return ((r << 8) &  0xf800)
                | ((g << 3) & 0x07e0) | ((b >> 3) & 0x001f);

            // 8 R, 8 G, 8 B (24/32 bit display)
        } else if  ((ximage->red_mask == 0xff0000)
                    && (ximage->green_mask == 0xff00)
                    && (ximage->blue_mask == 0xff)) {
            return ((r << 16) & 0xff0000)
                | ((g << 8) & 0x00ff00) | (b & 0x0000ff);
        } else {
            return 0;
        }
    }

    /**
     * Fill in RGB values from pixel value from ximage.
     */
    inline void
    getRgbFromPixel(XImage *ximage, ulong pixel, uchar &r, uchar &g, uchar &b)
    {
        // 5 R, 5 G, 5 B (15 bit display)
        if ((ximage->red_mask == 0x7c00)
            && (ximage->green_mask == 0x3e0) && (ximage->blue_mask == 0x1f)) {
            r = g = b = 0;
            // 5 R, 6 G, 5 B (16 bit display)
        } else  if ((ximage->red_mask == 0xf800)
                    && (ximage->green_mask == 0x07e0)
                    && (ximage->blue_mask == 0x001f)) {
            r = g = b = 0;
            // 8 R, 8 G, 8 B (24/32 bit display)
        } else if  ((ximage->red_mask == 0xff0000)
                    && (ximage->green_mask == 0xff00)
                    && (ximage->blue_mask == 0xff)) {
            r = (pixel >> 16) & 0xff;
            g = (pixel >> 8) & 0xff;
            b = pixel & 0xff;
        } else {
            r = 0;
            g = 0;
            b = 0;
        }
    }

protected:
    Display *_dpy; //!< Display image is on.

    ImageType _type; //!< Type of image.

    Pixmap _pixmap; //!< Pixmap representation of image.
    Pixmap _mask; //!< Pixmap representation of image shape mask.

    uint _width; //!< Width of image.
    uint _height; //!< Height of image.

    uchar *_data; //!< Data describing image.
    bool _has_alpha; //!< Wheter image has alpha channel.
    bool _use_alpha; //!< Wheter image has alpha < 100%

private:
    static std::list<PImageLoader*> _loader_list; //!< List of loaders.
};

#endif // _PIMAGE_HH_
