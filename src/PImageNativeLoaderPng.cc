//
// PImageNativeLoaderPng.cc for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifdef HAVE_IMAGE_PNG

#include "PImageNativeLoaderPng.hh"

#include <iostream>

extern "C" {
#include <png.h>
}

using std::string;
using std::cerr;
using std::endl;

const int PImageNativeLoaderPng::PNG_SIG_BYTES = 8;

//! @brief PImageNativeLoaderPng constructor.
PImageNativeLoaderPng::PImageNativeLoaderPng(void)
        : PImageNativeLoader("PNG")
{
}

//! @brief PImageNativeLoaderPng destructor.
PImageNativeLoaderPng::~PImageNativeLoaderPng(void)
{
}

//! @brief Loads file into data.
//! @param file File to load data from.
//! @param width Set to the width of image.
//! @param height Set to the height of image.
//! @param alpha Set to wheter image has alpha channel.
//! @return Pointer to data on success, else NULL.
uchar*
PImageNativeLoaderPng::load(const std::string &file, uint &width, uint &height,
                            bool &alpha)
{
    FILE *fp;

    fp = fopen (file.c_str (), "rb");
    if (!fp) {
        cerr << " *** WARNING: unable to open " << file << " for reading!" << endl;
        return NULL;
    }

    if (!checkSignature (fp)) {
        cerr << " *** WARNING: " << file << " not a PNG file!" << endl;
        fclose (fp);
        return NULL;
    }

    // Start PNG loading.
    png_structp png_ptr;
    png_infop info_ptr;

    png_ptr = png_create_read_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        cerr << " *** ERROR: out of memory, png_create_read_struct failed!" << endl;
        fclose (fp);
        return NULL;
    }
    
    info_ptr = png_create_info_struct (png_ptr);
    if (!info_ptr) {
        cerr << " *** ERROR: out of memory, png_create_info_struct failed!" << endl;
        png_destroy_read_struct (&png_ptr, NULL, NULL);
        fclose (fp);
        return NULL;
    }

    // Setup error handling.
    if (setjmp (png_jmpbuf (png_ptr))) {
        png_destroy_read_struct (&png_ptr, &info_ptr, NULL);
        fclose (fp);
        return NULL;
    }

    png_init_io (png_ptr, fp);
    png_set_sig_bytes (png_ptr, PImageNativeLoaderPng::PNG_SIG_BYTES);
    png_read_info (png_ptr, info_ptr);

    int color_type, bpp;
    png_uint_32 png_width = 1, png_height = 1;
    png_get_IHDR (png_ptr, info_ptr, &png_width, &png_height,
                  &bpp, &color_type, NULL, NULL, NULL);

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
        png_set_gray_1_2_4_to_8 (png_ptr);
    }

    if (png_get_valid (png_ptr, info_ptr, PNG_INFO_tRNS)) {
        png_set_tRNS_to_alpha (png_ptr);
    }
    
    if (bpp == 16) {
        png_set_strip_16 (png_ptr);
    }
    // gray, gray alpha -> to RGB
    if ((color_type == PNG_COLOR_TYPE_GRAY)
            || (color_type == PNG_COLOR_TYPE_GRAY_ALPHA))
    {
        png_set_gray_to_rgb (png_ptr);
    }

    // Now load image data.
    uchar *data;
    png_uint_32 rowbytes, channels;
    png_bytepp row_pointers;

    png_read_update_info (png_ptr, info_ptr);

    rowbytes = png_get_rowbytes (png_ptr, info_ptr);
    channels = png_get_channels (png_ptr, info_ptr);

    data = new uchar[rowbytes * height / sizeof(uchar)];
    row_pointers = new png_bytep[height];

    for (png_uint_32 y = 0; y < height; ++y)
        row_pointers[y] = data + y * rowbytes;

    png_read_image(png_ptr, row_pointers);

    delete [] row_pointers;

    png_read_end (png_ptr, NULL);

    // Clean up.
    png_destroy_read_struct (&png_ptr, &info_ptr, NULL);

    fclose (fp);

    alpha = (channels == 4);

    return data;
}

//! @brief Checks file signature to see if it's a PNG file.
//! @param fp Pointer to open (rb) FILE.
//! @return true if fp is a PNG file, else false.
bool
PImageNativeLoaderPng::checkSignature(FILE *fp)
{
    int status;
    uchar sig[PImageNativeLoaderPng::PNG_SIG_BYTES];

    status = fread(sig, 1, PImageNativeLoaderPng::PNG_SIG_BYTES, fp);
    if (status == PImageNativeLoaderPng::PNG_SIG_BYTES) {
        return (png_check_sig(sig, PImageNativeLoaderPng::PNG_SIG_BYTES) != 0);
    }
    return false;
}

#endif // HAVE_IMAGE_PNG
