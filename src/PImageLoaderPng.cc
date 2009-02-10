//
// PImageLoaderPng.cc for pekwm
// Copyright © 2005-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_IMAGE_PNG

#include "PImageLoaderPng.hh"

#include <iostream>

extern "C" {
#include <png.h>
}

using std::string;
using std::cerr;
using std::endl;

const int PImageLoaderPng::PNG_SIG_BYTES = 8;

//! @brief PImageLoaderPng constructor.
PImageLoaderPng::PImageLoaderPng(void)
        : PImageLoader("PNG")
{
}

//! @brief PImageLoaderPng destructor.
PImageLoaderPng::~PImageLoaderPng(void)
{
}

//! @brief Loads file into data.
//! @param file File to load data from.
//! @param width Set to the width of image.
//! @param height Set to the height of image.
//! @param alpha Set to wheter image has alpha channel.
//! @return Pointer to data on success, else 0.
uchar*
PImageLoaderPng::load(const std::string &file, uint &width, uint &height,
                            bool &alpha, bool &use_alpha)
{
    FILE *fp;

    fp = fopen(file.c_str(), "rb");
    if (! fp) {
        cerr << " *** WARNING: unable to open " << file << " for reading!" << endl;
        return 0;
    }

    if (! checkSignature(fp)) {
        cerr << " *** WARNING: " << file << " not a PNG file!" << endl;
        fclose(fp);
        return 0;
    }

    // Start PNG loading.
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (! png_ptr) {
        cerr << " *** ERROR: out of memory, png_create_read_struct failed!" << endl;
        fclose(fp);
        return 0;
    }
    
    info_ptr = png_create_info_struct(png_ptr);
    if (! info_ptr) {
        cerr << " *** ERROR: out of memory, png_create_info_struct failed!" << endl;
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
    png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bpp, &color_type, 0, 0, 0);

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
        png_set_gray_1_2_4_to_8(png_ptr);
    }

    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha(png_ptr);
    }
    
    if (bpp == 16) {
        png_set_strip_16(png_ptr);
    }
    // gray, gray alpha -> to RGB
    if ((color_type == PNG_COLOR_TYPE_GRAY) || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
        png_set_gray_to_rgb(png_ptr);
    }

    // Now load image data.
    uchar *data;
    size_t data_size;
    png_uint_32 rowbytes, channels;
    png_bytepp row_pointers;

    png_read_update_info(png_ptr, info_ptr);

    rowbytes = png_get_rowbytes(png_ptr, info_ptr);
    channels = png_get_channels(png_ptr, info_ptr);

    data_size = rowbytes * height / sizeof(uchar);
    data = new uchar[data_size];
    row_pointers = new png_bytep[height];

    for (png_uint_32 y = 0; y < height; ++y) {
        row_pointers[y] = data + y * rowbytes;
    }

    png_read_image(png_ptr, row_pointers);

    delete [] row_pointers;

    png_read_end(png_ptr, 0);

    // Clean up.
    png_destroy_read_struct(&png_ptr, &info_ptr, 0);

    fclose(fp);

    alpha = (channels == 4);
    use_alpha = false;

    if (alpha) {
        uchar *p = data + 3, *end = data + data_size;
        for (; p != end && ! use_alpha; p += 4) {
            if (*p != 255) {
                use_alpha = true;
            }
        }
    }

    return data;
}

//! @brief Checks file signature to see if it's a PNG file.
//! @param fp Pointer to open (rb) FILE.
//! @return true if fp is a PNG file, else false.
bool
PImageLoaderPng::checkSignature(FILE *fp)
{
    int status;
    uchar sig[PImageLoaderPng::PNG_SIG_BYTES];

    status = fread(sig, 1, PImageLoaderPng::PNG_SIG_BYTES, fp);
    if (status == PImageLoaderPng::PNG_SIG_BYTES) {
        return (png_check_sig(sig, PImageLoaderPng::PNG_SIG_BYTES) != 0);
    }
    return false;
}

#endif // HAVE_IMAGE_PNG
