//
// PImageNativeLoaderXpm.cc for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HAVE_IMAGE_XPM

#include "PImageNativeLoaderXpm.hh"

#include <iostream>

using std::cerr;
using std::endl;
using std::string;

const uint PImageNativeLoaderXpm::CHANNELS = 4;
const uint PImageNativeLoaderXpm::ALPHA_SOLID = 255;
const uint PImageNativeLoaderXpm::ALPHA_TRANSPARENT = 0;
const char *PImageNativeLoaderXpm::COLOR_DEFAULT = "#000000";

//! @brief PImageNativeLoaderXpm constructor.
PImageNativeLoaderXpm::PImageNativeLoaderXpm(void) : PImageNativeLoader("XPM")
{
}

//! @brief PImageNativeLoaderXpm destructor.
PImageNativeLoaderXpm::~PImageNativeLoaderXpm(void)
{
}

//! @brief Loads file into data.
//! @param file File to load data from.
//! @param width Set to the width of image.
//! @param height Set to the height of image.
//! @param alpha Set to wheter image has alpha channel.
//! @return Pointer to data on success, else NULL.
uchar*
PImageNativeLoaderXpm::load(const std::string &file, uint &width, uint &height,
                            bool &alpha)
{
    XpmImage xpm_image;
    XpmInfo xpm_info;

    // Read XPM to XpmImage format.
    if (XpmReadFileToXpmImage((char*) file.c_str(),
                              &xpm_image, &xpm_info) != Success) {
        cerr << " *** WARNING: " << file << " not a valid XPM file!" << endl;
        return NULL;
    }

    if (!xpm_image.ncolors || !xpm_image.data
            || !xpm_image.width || !xpm_image.height) {
        cerr << " *** WARNING: " << file << " invalid file information!" << endl;
        return NULL;
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

    // Fill data.
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            *dest++ = xpm_to_rgba[*src * CHANNELS]; // R
            *dest++ = xpm_to_rgba[*src * CHANNELS + 1]; // G
            *dest++ = xpm_to_rgba[*src * CHANNELS + 2]; // B
            *dest++ = xpm_to_rgba[*src * CHANNELS + 3]; // A

            *src++;
        }
    }

    // Cleanup.
    delete [] xpm_to_rgba;

    XpmFreeXpmImage(&xpm_image);
    XpmFreeXpmInfo(&xpm_info);

    alpha = true;

    return data;
}

//! @brief Creates an color number to RGBA conversion table.
//! @param xpm_image Pointer to XpmImage.
//! @return Pointer to color number to RGBA conversion table.
uchar*
PImageNativeLoaderXpm::createXpmToRgbaTable(XpmImage *xpm_image)
{
    uint c_tmp; // Temporary color value.
    char c_buf[3] = { '\0', '\0', '\0' }; // Temporary hex color string.
    const char *color;
    uchar *xpm_to_rgba, *dest;

    xpm_to_rgba = new uchar[xpm_image->ncolors * CHANNELS];
    dest = xpm_to_rgba;
    for (uint i = 0; i < xpm_image->ncolors; ++i) {
        if (xpm_image->colorTable[i].c_color)
            color = xpm_image->colorTable[i].c_color;
        else if (xpm_image->colorTable[i].g_color)
            color = xpm_image->colorTable[i].g_color;
        else if (xpm_image->colorTable[i].g4_color)
            color = xpm_image->colorTable[i].g4_color;
        else if (xpm_image->colorTable[i].m_color)
            color = xpm_image->colorTable[i].m_color;
        else
            color = COLOR_DEFAULT;

        // Color in format #RRGGBB.
        if (color && (strlen(color) == 7)) {
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

            // Invalid format or None, set it transparent
        } else {
            *dest++ = 0; // R
            *dest++ = 0; // G
            *dest++ = 0; // B
            *dest++ = ALPHA_TRANSPARENT; // A
        }
    }

    return xpm_to_rgba;
}

#endif // HAVE_IMAGE_XPM
