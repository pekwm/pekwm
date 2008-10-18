//
// PImageNativeLoaderJpeg.cc for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HAVE_IMAGE_JPEG

#include "PImageNativeLoaderJpeg.hh"

#include <iostream>

extern "C" {
#include <jpeglib.h>
}

using std::string;
using std::cerr;
using std::endl;

//! @brief PImageNativeLoaderJpeg constructor.
PImageNativeLoaderJpeg::PImageNativeLoaderJpeg(void)
        : PImageNativeLoader("JPG")
{
}

//! @brief PImageNativeLoaderJpeg destructor.
PImageNativeLoaderJpeg::~PImageNativeLoaderJpeg(void)
{
}

//! @brief Loads file into data.
//! @param file File to load data from.
//! @param width Set to the width of image.
//! @param height Set to the height of image.
//! @param alpha Set to wheter image has alpha channel.
//! @return Pointer to data on success, else 0.
uchar*
PImageNativeLoaderJpeg::load(const std::string &file, uint &width, uint &height, bool &alpha)
{
    FILE *fp;

    fp= fopen(file.c_str(), "rb");
    if (! fp) {
        cerr << " *** WARNING: unable to open " << file << " for reading!" << endl;
        return 0;
    }

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    jpeg_stdio_src(&cinfo, fp);

    // Read jpeg header.
    jpeg_read_header(&cinfo, TRUE);

    // Make sure we get data in 24bit RGB.
    if (cinfo.out_color_space != JCS_RGB)
        cinfo.out_color_space = JCS_RGB;

    jpeg_start_decompress(&cinfo);

    uint channels;
    uint rowbytes;

    width = cinfo.output_width;
    height = cinfo.output_height;
    channels = cinfo.output_components;
    rowbytes = width * channels;

    // Allocate image data.
    uchar *data = new uchar[width * height * channels];

    // Read image.
    JSAMPROW row;
    for (uint i = 0; i < height; ++i) {
        row = data + i * rowbytes;
        jpeg_read_scanlines(&cinfo, &row, 1);
    }

    // Clean up.
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(fp);

    alpha = false; // Jpeg doesn't support alpha.

    return data;
}

#endif // HAVE_IMAGE_JPEG
