//
// PImage.hh for pekwm
// Copyright (C) 2004-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PIMAGE_HH_
#define _PEKWM_PIMAGE_HH_

#include "config.h"
#include "Render.hh"
#include "pekwm_types.hh"

#include <string>

/**
 * (A)RGB Image, X11 drawing support, scaling etc.
 */
class PImage {
public:
	enum ScaleType {
		SCALE_SQUARE,
		SCALE_SMOOTH
	};

	PImage(const std::string &path);
	PImage(PImage *image);
	PImage(XImage *image, uchar opacity=255);
	virtual ~PImage(void);

	//! @brief Returns type of image.
	inline ImageType getType() const { return _type; }
	//! @brief Sets type of image.
	inline void setType(ImageType type) { _type = type; }

	inline uchar* getData(void) { return _data; }
	//! @brief Returns width of image.
	inline size_t getWidth(void) const { return _width; }
	//! @brief Returns height of image.
	inline size_t getHeight(void) const { return _height; }

	bool load(const std::string &file);
	void unload(void);

	void draw(Render &rend, int x, int y,
		  size_t width = 0, size_t height = 0);
	Pixmap getPixmap(bool &need_free, size_t width = 0, size_t height = 0);
	Pixmap getMask(bool &need_free, size_t width = 0, size_t height = 0);
	void scale(float factor, ScaleType type = SCALE_SMOOTH);
	void scale(size_t width, size_t height, ScaleType type = SCALE_SMOOTH);

	static void drawAlphaFixed(Render &rend,
				   int x, int y, size_t width, size_t height,
				   uchar* data);
	static void drawAlphaFixed(XImage *src_image, XImage *dest_image,
				   int x, int y, size_t width, size_t height,
				   uchar* data);

protected:
	PImage(void);

	void drawFixed(Render &rend,
		       int x, int y, size_t width, size_t height);
	void drawScaled(Render &rend,
			int x, int y, size_t width, size_t height);
	void drawTiled(Render &rend,
		       int x, int y, size_t width, size_t height);
	void drawAlphaScaled(Render &rend,
			     int x, int y, size_t widht, size_t height);
	void drawAlphaTiled(Render &rend,
			    int x, int y, size_t widht, size_t height);

	Pixmap createPixmap(uchar* data, size_t width, size_t height);
	Pixmap createMask(uchar* data, size_t width, size_t height);

private:
	PImage(const PImage&);
	PImage& operator=(const PImage&);

	XImage* createXImage(uchar* data, size_t width, size_t height);
	uchar* getScaledData(size_t width, size_t height, ScaleType type);
	uchar* getScaledDataSmooth(size_t width, size_t height);
	uchar* getScaledDataSquare(uint factor);
	void setScaledDataSquare(uint factor, const uchar *src, uchar *dst);

protected:
	ImageType _type; //!< Type of image.

	Pixmap _pixmap; //!< Pixmap representation of image.
	Pixmap _mask; //!< Pixmap representation of image shape mask.

	size_t _width; //!< Width of image.
	size_t _height; //!< Height of image.

	/** ARGB image data. */
	uchar *_data;
	/** If all pixels have 100% alpha, this is set to false. */
	bool _use_alpha;
};

#endif // _PEKWM_PIMAGE_HH_
