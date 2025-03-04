//
// PSurface.hh for pekwm
// Copyright (C) 2023 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PSURFACE_HH_
#define _PEKWM_PSURFACE_HH_

#include "X11.hh"

/**
 * Surface interface used for drawing operations.
 */
class PSurface {
public:
	PSurface() { }
	virtual ~PSurface() { }

	virtual Drawable getDrawable() const = 0;

	virtual int getX() const = 0;
	virtual int getY() const = 0;
	virtual uint getWidth() const = 0;
	virtual uint getHeight() const = 0;
	int getRX() const { return getX() + static_cast<int>(getWidth()); }
	int getBY() const { return getY() + static_cast<int>(getHeight()); }

	bool clip(int x, int y, uint &width, uint &height)
	{
		if (x > getRX() || y > getBY()) {
			// nothing is visible
			return false;
		}

		if ((x + width) > (getX() + getWidth())) {
			// width needs clipping
			width = getWidth() + getX() - x;
		}
		if ((y + height) > (getY() + getHeight())) {
			// height needs clipping
			height = getHeight() + getY() - y;
		}
		return true;
	}
};

#endif // _PEKWM_PSURFACE_HH_
