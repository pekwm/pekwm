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

const int PNG_SIG_BYTES = 8;

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

static uchar*
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

static uchar*
convertArgbToRgb(uchar* data_argb, uint width, uint height)
{
    auto data_rgb = new uchar[width * height * 4];
    int src = 0, dst = 0;
    for (uint y = 0; y < height; ++y) {
        for (uint x = 0; x < width; ++x) {
            src++; // A
            data_rgb[dst++] = data_argb[src++]; // R
            data_rgb[dst++] = data_argb[src++]; // G
            data_rgb[dst++] = data_argb[src++]; // B
        }
    }
    return data_rgb;
}


/**
 * Checks file signature to see if it's a PNG file.
 *
 * @param fp Pointer to open (rb) FILE.
 * @return true if fp is a PNG file, else false.
 */
static bool
checkSignature(FILE *fp)
{
    uchar sig[PNG_SIG_BYTES];

    int status = fread(sig, 1, PNG_SIG_BYTES, fp);
    if (status == PNG_SIG_BYTES) {
        return (png_sig_cmp(sig, 0, PNG_SIG_BYTES) == 0);
    }
    return false;
}


namespace PImageLoaderPng
{
    const char*
    getExt(void)
    {
        return "PNG";
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
    load(const std::string &file, uint &width, uint &height, bool &use_alpha)
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
        png_set_sig_bytes(png_ptr, PNG_SIG_BYTES);
        png_read_info(png_ptr, info_ptr);

        int color_type, bpp;
        png_uint_32 png_width = 1, png_height = 1;
        png_get_IHDR(png_ptr, info_ptr, &png_width, &png_height, &bpp, &color_type,
                     0, 0, 0);

        width = png_width;
        height = png_height;

        // Setup read information, read 24/32bit RGB/A

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

    bool
    save(const std::string& file, uchar *data, uint width, uint height)
    {
        auto png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
        if (! png_ptr) {
            ERR("out of memory, png_create_write_struct failed");
            return false;
        }

        auto info_ptr = png_create_info_struct(png_ptr);
        if (! info_ptr) {
            ERR("out of memory, png_create_info_struct failed");
            png_destroy_write_struct(&png_ptr, 0);
            return false;
        }

        // Setup png lib error handling
        if (setjmp(png_jmpbuf(png_ptr))) {
            png_destroy_write_struct(&png_ptr, &info_ptr);
            return false;
        }

        auto fp = fopen(file.c_str(), "wb");
        if (!fp) {
            USER_WARN("failed to open " << file << " for writing");
            png_destroy_write_struct(&png_ptr, &info_ptr);
            return false;
        }

        png_init_io(png_ptr, fp);

        // Setup write information, write 24bit RGB
        png_set_IHDR(png_ptr, info_ptr, width, height, 8,
                     PNG_COLOR_TYPE_RGB,
                     PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT,
                     PNG_FILTER_TYPE_DEFAULT);

        auto rgb_data = convertArgbToRgb(data, width, height);
        png_write_info(png_ptr, info_ptr);
        png_bytep row = rgb_data;
        for (uint y = 0; y < height; y++) {
            png_write_row(png_ptr, row);
            row += width * 3;
        }
        png_write_end(png_ptr, NULL);

        delete [] rgb_data;

        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);

        return true;
    }
}

#endif // HAVE_IMAGE_PNG
