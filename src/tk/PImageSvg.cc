//
// PImageSvg.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#ifdef PEKWM_HAVE_IMAGE_SVG

#include "Debug.hh"
#include "Exception.hh"
#include "PImageSvg.hh"

extern "C" {
#include <cairo/cairo-xlib.h>
}

PImageSvg::PImageSvg(const std::string &path)
{
	GError *error = nullptr;
	GFile *file = g_file_new_for_path(path.c_str());
	_handle = rsvg_handle_new_from_gfile_sync(file, RSVG_HANDLE_FLAGS_NONE,
						  nullptr, &error);
	g_object_unref(file);
	if (_handle == nullptr) {
		std::string message(error->message);
		g_object_unref(error);
		throw LoadException(message);
	}

	RsvgDimensionData dim = {};
	double width, height;
	if (rsvg_handle_get_intrinsic_size_in_pixels(_handle,
						     &width, &height)) {
		_width = static_cast<uint>(width);
		_height = static_cast<uint>(height);
	}
}

PImageSvg::PImageSvg(const PImageSvg *image)
{
}

PImageSvg::~PImageSvg()
{
	unload();
}

void
PImageSvg::unload()
{
	if (_handle) {
		g_object_unref(_handle);
		_handle = nullptr;
	}
}

void
PImageSvg::draw(Render &rend, PSurface *surface, int x, int y,
		uint width, uint height)
{
	cairo_surface_t *csurface =
		cairo_xlib_surface_create(X11::getDpy(),
					  surface->getDrawable(),
					  X11::getVisual(),
					  surface->getWidth(),
					  surface->getHeight());
	cairo_t *cr = cairo_create(csurface);

	// FIXME: Render according to the image type
	switch (_type) {
	case IMAGE_TYPE_TILED:
		break;
	case IMAGE_TYPE_SCALED:
		break;
	case IMAGE_TYPE_FIXED:
		break;
	default:
		break;
	}

	RsvgRectangle viewport = {
	  .x = static_cast<double>(x),
	  .y = static_cast<double>(y),
	  .width = static_cast<double>(_width),
	  .height = static_cast<double>(_height)
	};

	GError *error = nullptr;
	if (! rsvg_handle_render_document(_handle, cr, &viewport, &error)) {
		P_ERR("failed to render SVG: " << error->message);
		g_object_unref(error);
	}

	cairo_destroy(cr);
	cairo_surface_destroy(csurface);
}

Pixmap
PImageSvg::getPixmap(bool &need_free, uint width, uint height)
{
	return None;
}

Pixmap
PImageSvg::getMask(bool &need_free, uint width, uint height)
{
	return None;
}

void
PImageSvg::scale(float factor, ScaleType type)
{
}

void
PImageSvg::scale(uint width, uint height, ScaleType type)
{
}

#endif // PEKWM_HAVE_IMAGE_SVG
