//
// PXftColor.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PXFTCOLOR_HH_
#define _PEKWM_PXFTCOLOR_HH_

#include "config.h"

#if defined(PEKWM_HAVE_XFT) || defined(PEKWM_HAVE_PANGO_XFT)

extern "C" {
#include <X11/Xft/Xft.h>
}

/**
 * XftColor wrapper.
 */
class PXftColor {
public:
	PXftColor();
	~PXftColor();

	XftColor* operator*() { return &_color; }

	void set(ushort r, ushort g, ushort b, uint alpha);
	void unset();
	bool isSet() const { return _is_set; }

private:
	PXftColor(const PXftColor&);
	PXftColor& operator=(const PXftColor&);

private:
	XftColor _color;
	bool _is_set;
};

#endif // defined(PEKWM_HAVE_XFT) || defined(PEKWM_HAVE_PANGO_XFT)

#endif // _PEKWM_PXFTCOLOR_HH_
