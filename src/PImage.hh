//
// PImage.hh for pekwm
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _PIMAGE_HH_
#define _PIMAGE_HH_

#include "pekwm.hh"

#include <string>

//! @brief Image baseclass defining interface for image handling.
class PImage {
public:
	//! @brief PImage constructor.
	PImage(Display *dpy) : _dpy(dpy), _type(IMAGE_TYPE_NO),
												 _pixmap(None), _mask(None),
												 _width(0), _height(0) { }
	//! @brief PImage destructor.
	virtual ~PImage(void) { unload(); }

	//! @brief Returns type of image.
	inline ImageType getType(void) const { return _type; }
	//! @brief Sets type of image.
	inline void setType(ImageType type) { _type = type; }

	//! @brief Returns width of image.
	inline uint getWidth(void) const { return _width; }
	//! @brief Returns height of image.
	inline uint getHeight(void) const { return _height; }

	//! @brief Loads image. (empty method, interface)
	virtual bool load(const std::string &file) { (&file); return false; }
	//! @brief Unloads image. (empty method, interface)
	virtual void unload(void) { }
	//! @brief Draw image on Drawable dest. (empty method, interface)
	virtual void draw(Drawable dest, int x, int y,
										uint width = 0, uint height = 0) {
		(&dest); (&x); (&y); (&width); (&height);
	}
	//! @brief Returns pixmap at size. (empty method, interace)
	virtual Pixmap getPixmap(bool &need_free, uint width = 0, uint height = 0) {
		(&width); (&height);
		need_free = false;
		return None;
	}
	//! @brief Returns shape mask at size, if any. (empty method, interface)
	virtual Pixmap getMask(bool &need_free, uint width = 0, uint height = 0) {
		(&width); (&height);
		need_free = false;
		return None;
	}
	//! @brief Scales image to size. (empty method, interface)
	virtual void scale(uint width, uint height) { (&width); (&height); }

protected:
	Display *_dpy; //!< Display image is on.

	ImageType _type; //!< Type of image.

	Pixmap _pixmap; //!< Pixmap representation of image.
	Pixmap _mask; //!< Pixmap representation of image shape mask.

	uint _width; //!< Width of image.
	uint _height; //!< Height of image.
};

#endif // _PIMAGE_HH_
