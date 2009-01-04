//
// PImageLoaderXpm.cc for pekwm
// Copyright © 2005-2008 Claes Nästén <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_IMAGE_XPM

#include "PScreen.hh"
#include "PImageLoaderXpm.hh"

#include <iostream>
#include <cstring>
#include <cstdio>

using std::cerr;
using std::endl;
using std::string;

const uint PImageLoaderXpm::CHANNELS = 4;
const uint PImageLoaderXpm::ALPHA_SOLID = 255;
const uint PImageLoaderXpm::ALPHA_TRANSPARENT = 0;
const char *PImageLoaderXpm::COLOR_DEFAULT = "#000000";

//! @brief PImageLoaderXpm constructor.
PImageLoaderXpm::PImageLoaderXpm(void)
    : PImageLoader("XPM")
{
}

//! @brief PImageLoaderXpm destructor.
PImageLoaderXpm::~PImageLoaderXpm(void)
{
}

//! @brief Loads file into data.
//! @param file File to load data from.
//! @param width Set to the width of image.
//! @param height Set to the height of image.
//! @param alpha Set to wheter image has alpha channel.
//! @return Pointer to data on success, else 0.
uchar*
PImageLoaderXpm::load(const std::string &file, uint &width, uint &height,
                      bool &alpha, bool &use_alpha)
{
    XpmImage xpm_image;
    XpmInfo xpm_info;

    // Read XPM to XpmImage format.
    if (XpmReadFileToXpmImage((char*) file.c_str(), &xpm_image, &xpm_info) != Success) {
        cerr << " *** WARNING: " << file << " not a valid XPM file!" << endl;
        return 0;
    }

    if (! xpm_image.ncolors || ! xpm_image.data || ! xpm_image.width || ! xpm_image.height) {
        cerr << " *** WARNING: " << file << " invalid file information!" << endl;
        return 0;
    }

    // Build XpmColor -> RGBA Table.
    uchar *xpm_to_rgba;
    xpm_to_rgba = createXpmToRgbaTable(&xpm_image);

    width = xpm_image.width;
    height = xpm_image.height;

    // Allocate data.
    uchar *data, *dest;
    uint *src;

    data = new uchar[width * height * CHANNELS];
    dest = data;
    src = xpm_image.data;

    alpha = true;
    use_alpha = false;

    // Fill data.
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            *dest++ = xpm_to_rgba[*src * CHANNELS]; // R
            *dest++ = xpm_to_rgba[*src * CHANNELS + 1]; // G
            *dest++ = xpm_to_rgba[*src * CHANNELS + 2]; // B
            *dest++ = xpm_to_rgba[*src * CHANNELS + 3]; // A

            // Check for active use of alpha
            if (! use_alpha && xpm_to_rgba[*src * CHANNELS + 3] != 255) {
                use_alpha = true;
            }

            *src++;
        }
    }

    // Cleanup.
    delete [] xpm_to_rgba;

    XpmFreeXpmImage(&xpm_image);
    XpmFreeXpmInfo(&xpm_info);

    return data;
}

//! @brief Creates an color number to RGBA conversion table.
//! @param xpm_image Pointer to XpmImage.
//! @return Pointer to color number to RGBA conversion table.
uchar*
PImageLoaderXpm::createXpmToRgbaTable(XpmImage *xpm_image)
{
    uint c_tmp; // Temporary color value.
    char c_buf[3] = { '\0', '\0', '\0' }; // Temporary hex color string.
    const char *color;
    uchar *xpm_to_rgba, *dest;
    XColor xcolor_exact;

    xpm_to_rgba = new uchar[xpm_image->ncolors * CHANNELS];
    dest = xpm_to_rgba;
    for (uint i = 0; i < xpm_image->ncolors; ++i) {
        if (xpm_image->colorTable[i].c_color) {
            color = xpm_image->colorTable[i].c_color;
        } else if (xpm_image->colorTable[i].g_color) {
            color = xpm_image->colorTable[i].g_color;
        } else if (xpm_image->colorTable[i].g4_color) {
            color = xpm_image->colorTable[i].g4_color;
        } else if (xpm_image->colorTable[i].m_color) {
            color = xpm_image->colorTable[i].m_color;
        } else {
            color = COLOR_DEFAULT;
        }

        if (color && color[0] == '#' && strlen(color) == 7) {
            // Color in format #RRGGBB
            for (uint j = 0; j < 3; ++j) {
                c_buf[0] = color[j * 2 + 1];
                c_buf[1] = color[j * 2 + 2];
                if (sscanf(c_buf, "%x", &c_tmp) == 1) {
                    *dest++ = c_tmp;
                } else {
                    *dest++ = 0;
                    cerr << " *** WARNING: " << c_buf << " invalid color value!" << endl;
                }
            }
            *dest++ = ALPHA_SOLID;

        } else if (color && XParseColor(PScreen::instance()->getDpy(), PScreen::instance()->getColormap(),
                                        color, &xcolor_exact)) {
            *dest++ = xcolor_exact.red;
            *dest++ = xcolor_exact.green;
            *dest++ = xcolor_exact.blue;
            *dest++ = ALPHA_SOLID;
        } else {
            // Invalid format or None, set it transparent
            *dest++ = 0; // R
            *dest++ = 0; // G
            *dest++ = 0; // B
            *dest++ = ALPHA_TRANSPARENT; // A
        }
    }

    return xpm_to_rgba;
}

#endif // HAVE_IMAGE_XPM
