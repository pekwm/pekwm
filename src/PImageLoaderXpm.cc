//
// PImageLoaderXpm.cc for pekwm
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifdef PEKWM_HAVE_IMAGE_XPM

#include "Debug.hh"
#include "PImageLoaderXpm.hh"
#include "X11.hh"

#include <cstring>
#include <cstdio>

/** Alpha value for no transperency. */
static const uint ALPHA_SOLID = 255;
/** Alpha value for fully transperency. */
static const uint ALPHA_TRANSPARENT = 0;
/** Default Color if translation fails. */
static const char *COLOR_DEFAULT = "#000000";

/**
 * Creates an color number to ARGB conversion table.
 *
 * @param xpm_image Pointer to XpmImage.
 * @return Pointer to color number to ARGB conversion table.
 */
static int32_t*
createXpmToArgbTable(XpmImage *xpm_image, bool &use_alpha)
{
	use_alpha = false;

	uint c_tmp; // Temporary color value.
	char c_buf[3] = { '\0', '\0', '\0' }; // Temporary hex color string.
	const char *color;
	XColor xcolor_exact;

	uchar *xpm_to_argb = new uchar[xpm_image->ncolors * 4];
	uchar *dest = xpm_to_argb;
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
			*dest++ = ALPHA_SOLID;
			for (uint j = 0; j < 3; ++j) {
				c_buf[0] = color[j * 2 + 1];
				c_buf[1] = color[j * 2 + 2];
				if (sscanf(c_buf, "%x", &c_tmp) == 1) {
					*dest++ = c_tmp;
				} else {
					*dest++ = 0;
					USER_WARN(c_buf
						  << " invalid color value");
				}
			}
		} else if (color
			   && XParseColor(X11::getDpy(), X11::getColormap(),
					  color, &xcolor_exact)) {
			*dest++ = ALPHA_SOLID;
			*dest++ = xcolor_exact.red;
			*dest++ = xcolor_exact.green;
			*dest++ = xcolor_exact.blue;
		} else {
			// Invalid format or None, set it transparent
			use_alpha = true;
			*dest++ = ALPHA_TRANSPARENT; // A
			*dest++ = 0; // R
			*dest++ = 0; // G
			*dest++ = 0; // B
		}
	}

	return reinterpret_cast<int32_t*>(xpm_to_argb);
}


namespace PImageLoaderXpm
{
	const char*
	getExt(void)
	{
		return "XPM";
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
	load(const std::string &file, size_t &width, size_t &height,
	     bool &use_alpha)
	{
		// Read XPM to XpmImage format.
		XpmImage xpm_image = {0};
		XpmInfo xpm_info = {0};
		if (XpmReadFileToXpmImage((char*) file.c_str(),
					  &xpm_image, &xpm_info) != Success) {
			return 0;
		}

		if (! xpm_image.ncolors
		    || ! xpm_image.data
		    || ! xpm_image.width
		    || ! xpm_image.height) {
			std::ostringstream msg;
			msg << file << " invalid file information.";
			msg << " ncolors: " << xpm_image.ncolors;
			msg << " width: " << xpm_image.width;
			msg << " height: " << xpm_image.height;
			USER_WARN(msg.str());

			return 0;
		}

		// Build XpmColor -> ARGB Table.
		int32_t *xpm_to_argb =
			createXpmToArgbTable(&xpm_image, use_alpha);

		width = xpm_image.width;
		height = xpm_image.height;

		// Allocate data.
		uchar* data = new uchar[width * height * 4];
		int32_t *dest = reinterpret_cast<int32_t*>(data);
		uint *src = xpm_image.data;

		use_alpha = false;

		// Fill data.
		uint color;
		for (size_t y = 0; y < height; ++y) {
			for (size_t x = 0; x < width; ++x) {
				color = *src++;
				if (color < xpm_image.ncolors) {
					*dest++ = xpm_to_argb[color];
				} else {
					*dest++ = 0xffffffff;
				}
			}
		}

		// Cleanup.
		delete [] xpm_to_argb;

		XpmFreeXpmImage(&xpm_image);
		XpmFreeXpmInfo(&xpm_info);

		return data;
	}
}

#endif // PEKWM_HAVE_IMAGE_XPM
