//
// PTexture.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PTexture.hh"
#include "x11.hh"

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
    if (getPixel(pixel)) {
        // set background pixel
        X11::setWindowBackground(draw, pixel);
    } else if (width > 0 && height > 0) {
        auto pix = X11::createPixmap(width, height);
        render(pix, x, y, width, height);
        X11::setWindowBackgroundPixmap(draw, pix);
        X11::freePixmap(pix);
    }
}
