//
// PImage.hh for pekwm
// Copyright (C) 2004-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"
#include "Render.hh"

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

    bool load(const std::string &file);
    void unload(void);

    void draw(Render &rend, int x, int y,
              uint width = 0, uint height = 0);
    Pixmap getPixmap(bool &need_free, uint width = 0, uint height = 0);
    Pixmap getMask(bool &need_free, uint width = 0, uint height = 0);
    void scale(uint width, uint height);

    static void drawAlphaFixed(Render &rend,
                               int x, int y, uint width, uint height,
                               uchar* data);
    static void drawAlphaFixed(XImage *src_image, XImage *dest_image,
                               int x, int y, uint width, uint height,
                               uchar* data);

protected:
    PImage(void);

    void drawFixed(Render &rend, int x, int y, uint width, uint height);
    void drawScaled(Render &rend, int x, int y, uint widht, uint height);
    void drawTiled(Render &rend, int x, int y, uint widht, uint height);
    void drawAlphaScaled(Render &rend, int x, int y, uint widht, uint height);
    void drawAlphaTiled(Render &rend, int x, int y, uint widht, uint height);

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
