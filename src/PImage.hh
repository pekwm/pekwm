//
// PImage.hh for pekwm
// Copyright (C) 2004-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#include "pekwm.hh"

#include <string>

//! @brief Image baseclass defining interface for image handling.
class PImage {
public:
    PImage(const std::string &path);
    PImage(PImage *image);
    PImage(XImage *image, uchar opacity=255);
    virtual ~PImage(void);

    //! @brief Returns type of image.
    inline ImageType getType(void) const { return _type; }
    //! @brief Sets type of image.
    inline void setType(ImageType type) { _type = type; }

    inline uchar* getData(void) { return _data; }
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


    static void drawAlphaFixed(Drawable dest,
                               int x, int y, uint width, uint height,
                               uchar* data);
    static void drawAlphaFixed(XImage *dest_image,
                               int x, int y, uint width, uint height,
                               uchar* data);

protected:
    PImage(void);

    void drawFixed(Drawable dest, int x, int y, uint width, uint height);
    void drawScaled(Drawable dest, int x, int y, uint widht, uint height);
    void drawTiled(Drawable dest, int x, int y, uint widht, uint height);
    void drawAlphaScaled(Drawable dest, int x, int y, uint widht, uint height);
    void drawAlphaTiled(Drawable dest, int x, int y, uint widht, uint height);

    Pixmap createPixmap(uchar* data, uint width, uint height);
    Pixmap createMask(uchar* data, uint width, uint height);

private:
    XImage* createXImage(uchar* data, uint width, uint height);
    uchar* getScaledData(uint width, uint height);

protected:
    ImageType _type; //!< Type of image.

    Pixmap _pixmap; //!< Pixmap representation of image.
    Pixmap _mask; //!< Pixmap representation of image shape mask.

    uint _width; //!< Width of image.
    uint _height; //!< Height of image.

    /** ARGB image data. */
    uchar *_data;
    /** If all pixels have 100% alpha, this is set to false. */
    bool _use_alpha;
};
