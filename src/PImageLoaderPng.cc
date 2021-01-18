//
// PImageLoaderPng.cc for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifdef HAVE_IMAGE_PNG

#include "Debug.hh"
#include "PImageLoaderPng.hh"

extern "C" {
#include <png.h>
}

const int PImageLoaderPng::PNG_SIG_BYTES = 8;

static void
convertRgbaToArgb(uchar* data, uint width, uint height, bool& use_alpha)
{
    // alpha channel, no need to add alpha channel, check if an
    // any alpha is < 100%
    int pos = 0;
    uchar alpha;
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            alpha = data[pos + 3];
            data[pos + 3] = data[pos + 2];
            data[pos + 2] = data[pos + 1];
            data[pos + 1] = data[pos];
            data[pos] = alpha;
            if (! use_alpha && alpha != 255) {
                use_alpha = true;
            }
            pos += 4;
        }
    }
}

uchar*
convertRgbToArgb(uchar* data_rgb, uint width, uint height)
{
    auto data_argb = new uchar[width * height * 4];
    int src = 0, dst = 0;
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            data_argb[dst++] = 0xff; // A
            data_argb[dst++] = data_rgb[src++]; // R
            data_argb[dst++] = data_rgb[src++]; // G
            data_argb[dst++] = data_rgb[src++]; // B
        }
    }
    return data_argb;
}

PImageLoaderPng::PImageLoaderPng(void)
        : PImageLoader("PNG")
{
}

PImageLoaderPng::~PImageLoaderPng(void)
{
}

/**
 * Loads file into data.
 *
 * @param file File to load data from.
 * @param width Set to the width of image.
 * @param height Set to the height of image.
 * @param alpha Set to wheter image has alpha channel.
 * @return Pointer to data on success, else 0.
 */
uchar*
PImageLoaderPng::load(const std::string &file, uint &width, uint &height,
                      bool &use_alpha)
{
    auto fp = fopen(file.c_str(), "rb");
    if (! fp) {
        USER_WARN("failed to open " << file << " for reading");
        return 0;
    }

    if (! checkSignature(fp)) {
        USER_WARN(file << " is not a PNG file");
        fclose(fp);
        return 0;
    }

    // Start PNG loading.
    auto png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (! png_ptr) {
        ERR("out of memory, png_create_read_struct failed");
        fclose(fp);
        return 0;
    }

    auto info_ptr = png_create_info_struct(png_ptr);
    if (! info_ptr) {
        ERR("out of memory, png_create_info_struct failed");
        png_destroy_read_struct(&png_ptr, 0, 0);
        fclose(fp);
        return 0;
    }

    // Setup error handling.
    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_read_struct(&png_ptr, &info_ptr, 0);
        fclose(fp);
        return 0;
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, PImageLoaderPng::PNG_SIG_BYTES);
    png_read_info(png_ptr, info_ptr);

    int color_type, bpp;
    png_uint_32 png_width = 1, png_height = 1;
    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bpp, &color_type,
                 0, 0, 0);

    width = png_width;
    height = png_height;

    // Setup read information, we want to make sure data get read in
    // 16 bit RGB(A)

    // palette -> RGB mode
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
        png_set_palette_to_rgb(png_ptr);
    }

    // gray -> 8 bit gray
    if (color_type == PNG_COLOR_TYPE_GRAY && (bpp < 8)) {
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }

    if (bpp == 16) {
        png_set_strip_16(png_ptr);
    }
    // gray, gray alpha -> to RGB
    if ((color_type == PNG_COLOR_TYPE_GRAY)
        || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
        png_set_gray_to_rgb(png_ptr);
    }

    // Now load image data.
    png_read_update_info(png_ptr, info_ptr);

    auto rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    auto channels = png_get_channels(png_ptr, info_ptr);

    size_t data_size = rowbytes * height;
    uchar* data = new uchar[data_size];
    png_bytepp row_pointers = new png_bytep[height];

    for (png_uint_32 y = 0; y < height; ++y) {
        row_pointers[y] = data + y * rowbytes;
    }

    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, 0);
    delete [] row_pointers;

    // Clean up.
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    fclose(fp);

    use_alpha = false;
    if (channels < 4) {
        auto data_argb = convertRgbToArgb(data, width, height);
        delete [] data;

        data = data_argb;
        use_alpha = false;
    } else {
        convertRgbaToArgb(data, width, height, use_alpha);
    }

    return data;
}

/**
 * Checks file signature to see if it's a PNG file.
 *
 * @param fp Pointer to open (rb) FILE.
 * @return true if fp is a PNG file, else false.
 */
bool
PImageLoaderPng::checkSignature(FILE *fp)
{
    uchar sig[PImageLoaderPng::PNG_SIG_BYTES];

    int status = fread(sig, 1, PImageLoaderPng::PNG_SIG_BYTES, fp);
    if (status == PImageLoaderPng::PNG_SIG_BYTES) {
        return (png_sig_cmp(sig,0,PImageLoaderPng::PNG_SIG_BYTES) == 0);
    }
    return false;
}

#endif // HAVE_IMAGE_PNG
