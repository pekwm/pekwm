//
// PTexture.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PTexture.hh"
#include "PImage.hh"
#include "x11.hh"

/**
 * Render texture onto drawable.
 */
void
PTexture::render(Drawable draw,
                 int x, int y, uint width, uint height,
                 int root_x, int root_y)
{
    if (width == 0) {
        width = _width;
    }
    if (height == 0) {
        height = _height;
    }

    if (width && height) {
        doRender(draw, x, y, width, height);
        if (_opacity != 255) {
            renderOnBackground(draw, x, y, width, height, root_x, root_y);
        }
    }
}

/**
 * Set background on drawable using the current texture, for
 * solid/empty textures this sets the background pixel instead of
 * rendering a Pixmap.
 */
void
PTexture::setBackground(Drawable draw,
                        int x, int y, uint width, uint height)
{
    ulong pixel;
    if (getPixel(pixel) && _opacity == 255) {
        // set background pixel
        X11::setWindowBackground(draw, pixel);
    } else if (width > 0 && height > 0) {
        auto pix = X11::createPixmap(width, height);
        render(pix, x, y, width, height);
        X11::setWindowBackgroundPixmap(draw, pix);
        X11::freePixmap(pix);
    }
}

void
PTexture::renderOnBackground(Drawable draw,
                            int x, int y, uint width, uint height,
                            int root_x, int root_y)
{
    auto src_ximage = X11::getImage(draw, x, y, width, height,
                                    AllPlanes, ZPixmap);
    if (src_ximage) {
        auto src_image = PImage(src_ximage, _opacity);
        X11::destroyImage(src_ximage);

        long pix;
        if (X11::getLong(X11::getRoot(), XROOTPMAP_ID, pix, XA_PIXMAP)) {
            auto dest_ximage = X11::getImage(pix,
                                             root_x, root_y,
                                             width, height,
                                             AllPlanes, ZPixmap);
            if (dest_ximage) {
                // read background pixmap for area
                PImage::drawAlphaFixed(dest_ximage, x, y, width, height,
                                       src_image.getData());

                X11::putImage(draw, X11::getGC(), dest_ximage,
                              0, 0, x, y, width, height);
                X11::destroyImage(dest_ximage);
            }
        }
    }
}
