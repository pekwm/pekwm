//
// PImageLoaderJpeg.cc for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifdef HAVE_IMAGE_JPEG

#include "Debug.hh"
#include "PImageLoaderJpeg.hh"

extern "C" {
#include <stdio.h>
#include <jpeglib.h>
}

PImageLoaderJpeg::PImageLoaderJpeg(void)
    : PImageLoader("JPG")
{
}

PImageLoaderJpeg::~PImageLoaderJpeg(void)
{
}

/**
 * Loads file into data.
 *
 * @param file File to load data from.
 * @param width Set to the width of image.
 * @param height Set to the height of image.
 * @param use_alpha Set to true if pixels have < 100% alpha
 * @return Pointer to data on success, else 0.
 */
uchar*
PImageLoaderJpeg::load(const std::string &file, uint &width, uint &height,
                       bool &use_alpha)
{
    auto fp= fopen(file.c_str(), "rb");
    if (! fp) {
        USER_WARN("failed to open " << file << " for reading");
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
    if (cinfo.out_color_space != JCS_RGB) {
        cinfo.out_color_space = JCS_RGB;
    }

    jpeg_start_decompress(&cinfo);

    width = cinfo.output_width;
    height = cinfo.output_height;
    uint channels = cinfo.output_components;

    // Allocate image data.
    auto data = new uchar[width * height * 4];
    auto row_data = new uchar[width * channels];

    // Read image and convet to ARGB
    int pos = 0;
    JSAMPROW row;
    for (uint y = 0; y < height; ++y) {
        row = row_data;
        uint src = 0;
        jpeg_read_scanlines(&cinfo, &row, 1);
        for (uint x = 0; x < width; ++x) {
            data[pos++] = 0xff;
            data[pos++] = row[src++];
            data[pos++] = row[src++];
            data[pos++] = row[src++];
        }
    }
    delete [] row_data;

    // Clean up.
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    fclose(fp);

    // Jpeg doesn't support alpha.
    use_alpha = false;

    return data;
}

#endif // HAVE_IMAGE_JPEG
