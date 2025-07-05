//
// PImageSvg.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PIMAGESVG_HH_
#define _PEKWM_PIMAGESVG_HH_

#include "config.h"

#ifdef PEKWM_HAVE_IMAGE_SVG

#include "PImage.hh"

extern "C" {
#include <librsvg/rsvg.h>
}

class PImageSvg : public PImage {
public:
	PImageSvg(const std::string &path);
	PImageSvg(const PImageSvg *image);
	virtual ~PImageSvg();

	virtual void unload();

	virtual void draw(Render &rend, PSurface *surface, int x, int y,
			  uint width = 0, uint height = 0);
	virtual Pixmap getPixmap(bool &need_free,
				 uint width = 0, uint height = 0);
	virtual Pixmap getMask(bool &need_free,
			       uint width = 0, uint height = 0);
	virtual void scale(float factor, ScaleType type = SCALE_SMOOTH);
	virtual void scale(uint width, uint height,
			   ScaleType type = SCALE_SMOOTH);

private:
	PImageSvg(const PImageSvg &image);
	PImageSvg &operator=(const PImageSvg &image);

	RsvgHandle *_handle;
};

#endif // PEKWM_HAVE_IMAGE_SVG

#endif // _PEKWM_PIMAGESVG_HH_
