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
	PImage(XImage *image, uchar opacity=255, ulong *trans_pixel=nullptr);
	virtual ~PImage();

	//! @brief Returns type of image.
	inline ImageType getType() const { return _type; }
	//! @brief Sets type of image.
	inline void setType(ImageType type) { _type = type; }

	inline uchar* getData(void) { return _data; }
	//! @brief Returns width of image.
	inline uint getWidth(void) const { return _width; }
	//! @brief Returns height of image.
	inline uint getHeight(void) const { return _height; }

	bool load(const std::string &file);
	void unload(void);

	void draw(Render &rend, PSurface *surface, int x, int y,
		  uint width = 0, uint height = 0);
	Pixmap getPixmap(bool &need_free, uint width = 0, uint height = 0);
	Pixmap getMask(bool &need_free, uint width = 0, uint height = 0);
	void scale(float factor, ScaleType type = SCALE_SMOOTH);
	void scale(uint width, uint height, ScaleType type = SCALE_SMOOTH);

	static void drawAlphaFixed(Render &rend, PSurface *surface,
				   int x, int y, uint width, uint height,
				   uchar* data);
	static void drawAlphaFixed(XImage *src_image, XImage *dest_image,
				   int x, int y, uint width, uint height,
				   uchar* data);

protected:
	PImage(void);

	void drawFixed(Render &rend,
		       int x, int y, uint width, uint height);
	void drawScaled(Render &rend,
			int x, int y, uint width, uint height);
	void drawTiled(Render &rend,
		       int x, int y, uint width, uint height);
	void drawAlphaScaled(Render &rend, PSurface *surface,
			     int x, int y, uint width, uint height);
	void drawAlphaTiled(Render &rend, PSurface *surface,
			    int x, int y, uint widht, uint height);

	Pixmap createPixmap(uchar* data, uint width, uint height);
	Pixmap createMask(uchar* data, uint width, uint height);

private:
	PImage(const PImage&);
	PImage& operator=(const PImage&);

	XImage* createXImage(uchar* data, uint width, uint height);
	uchar* getScaledData(uint width, uint height, ScaleType type);
	uchar* getScaledDataSmooth(uint width, uint height);
	uchar* getScaledDataSquare(uint factor);
	void setScaledDataSquare(uint factor, const uchar *src, uchar *dst);

protected:
	ImageType _type; //!< Type of image.

	Pixmap _pixmap; //!< Pixmap representation of image.
	Pixmap _mask; //!< Pixmap representation of image shape mask.

	uint _width; //!< Width of image.
	uint _height; //!< Height of image.

	/** ARGB image data. */
	uchar *_data;
	/** If all pixels have 100% alpha, this is set to false. */
	bool _use_alpha;
	/** If use alpha is true, trans pixel represent data that should not
	 * be drawn. */
	long _trans_pixel;
};

#endif // _PEKWM_PIMAGE_HH_
