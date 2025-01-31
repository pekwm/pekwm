//
// PXftColor.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "PXftColor.hh"
#include "X11.hh"

#if defined(PEKWM_HAVE_XFT) || defined(PEKWM_HAVE_PANGO_XFT)

PXftColor::PXftColor()
	: _is_set(false)
{
}

PXftColor::~PXftColor()
{
	unset();
}

void
PXftColor::set(ushort r, ushort g, ushort b, uint alpha)
{
	unset();

	XRenderColor xrender_color;
	xrender_color.red = r;
	xrender_color.green = g;
	xrender_color.blue = b;
	xrender_color.alpha = alpha;

	XftColorAllocValue(X11::getDpy(), X11::getVisual(),
			   X11::getColormap(), &xrender_color,
			   &_color);
	_is_set = true;
}

void
PXftColor::unset()
{
	if (_is_set) {
		XftColorFree(X11::getDpy(), X11::getVisual(),
			     X11::getColormap(), &_color);
		_is_set = false;
	}
}

#endif // defined(PEKWM_HAVE_XFT) || defined(PEKWM_HAVE_PANGO_XFT)
