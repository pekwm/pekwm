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
 * Image interface.
 */
class PImage {
public:
	enum ScaleType {
		SCALE_SQUARE,
		SCALE_SMOOTH
	};

	PImage()
		: _type(IMAGE_TYPE_NO),
		  _width(0),
		  _height(0)
	{
	}
	PImage(ImageType type, uint width, uint height)
		: _type(type),
		  _width(width),
		  _height(height)
	{
	}
	virtual ~PImage() { }

	inline ImageType getType() const { return _type; }
	inline void setType(ImageType type) { _type = type; }

	inline uint getWidth() const { return _width; }
	inline uint getHeight() const { return _height; }

	virtual void unload() = 0;

	virtual void draw(Render &rend, PSurface *surface, int x, int y,
			  uint width = 0, uint height = 0) = 0;
	virtual Pixmap getPixmap(bool &need_free,
				 uint width = 0, uint height = 0) = 0;
	virtual Pixmap getMask(bool &need_free,
			       uint width = 0, uint height = 0) = 0;
	virtual void scale(float factor, ScaleType type = SCALE_SMOOTH) = 0;
	virtual void scale(uint width, uint height,
			   ScaleType type = SCALE_SMOOTH) = 0;

protected:
	ImageType _type; //!< Type of image.
	uint _width; //!< Width of image.
	uint _height; //!< Height of image.
};

/**
 * (A)RGB Image, X11 drawing support, scaling etc.
 */
class PImageData : public PImage {
public:
	PImageData(uchar *data, uint width, uint height, bool use_alpha);
	PImageData(PImageData *image);
	PImageData(XImage *image, uchar opacity=255,
		   ulong *trans_pixel=nullptr);
	virtual ~PImageData();
	
	uchar* getData() { return _data; }

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

	static void drawAlphaFixed(Render &rend, PSurface *surface,
				   int x, int y, uint width, uint height,
				   uchar* data);
	static void drawAlphaFixed(XImage *src_image, XImage *dest_image,
				   int x, int y, uint width, uint height,
				   uchar* data);

protected:
	PImageData();

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

	/** ARGB image data. */
	uchar *_data;
	/** If all pixels have 100% alpha, this is set to false. */
	bool _use_alpha;
	/** If use alpha is true, trans pixel represent data that should not
	 * be drawn. */
	long _trans_pixel;

	/** Pixmap representation of image. */
	Pixmap _pixmap;
	/** Pixmap representation of image shape mask. */
	Pixmap _mask;

private:
	PImageData(const PImageData&);
	PImageData& operator=(const PImageData&);

	XImage* createXImage(uchar* data, uint width, uint height);
	uchar* getScaledData(uint width, uint height, ScaleType type);
	uchar* getScaledDataSmooth(uint width, uint height);
	uchar* getScaledDataSquare(uint factor);
	void setScaledDataSquare(uint factor, const uchar *src, uchar *dst);
};

#endif // _PEKWM_PIMAGE_HH_
