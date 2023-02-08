//
// PImageIcon.hh for pekwm
// Copyright (C) 2007-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PIMAGEICON_HH_
#define _PEKWM_PIMAGEICON_HH_

#include "config.h"

#include "PImage.hh"
#include "X11.hh"

/**
 * Image loading from X11 windows.
 */
class PImageIcon : public PImage {
public:
	PImageIcon(PImage *image);
	virtual ~PImageIcon(void);

	void setOnWindow(Window win);

	static PImageIcon *newFromWindow(Window win);
	static void setOnWindow(Window win,
				size_t width, size_t height, uchar *data);

private:
	PImageIcon(void);

private:
	bool setImageFromData(uchar *data, ulong actual);

	static Cardinal* newCardinals(size_t width, size_t height, uchar *data);
	static void fromCardinals(size_t pixels,
				  Cardinal *from_data, uchar *to_data);
	static void toCardinals(size_t pixels,
				uchar *from_data, Cardinal *to_data);

private:
	Cardinal *_cardinals;
};

#endif // _PEKWM_PIMAGEICON_HH_
