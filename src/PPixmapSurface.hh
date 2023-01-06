#ifndef _PEKWM_PPIXMAP_SURFACE_HH_
#define _PEKWM_PPIXMAP_SURFACE_HH_

#include "PSurface.hh"
#include "X11.hh"

class PPixmapSurface : public PSurface {
public:
	PPixmapSurface()
		: PSurface(),
		  _width(0),
		  _height(0),
		  _pixmap(None)
	{
	}

	PPixmapSurface(uint width, uint height)
		: PSurface(),
		  _width(width),
		  _height(height),
		  _pixmap(X11::createPixmap(_width, _height))
	{
	}

	virtual ~PPixmapSurface()
	{
		if (_pixmap) {
			X11::freePixmap(_pixmap);
		}
	}

	virtual Drawable getDrawable() const { return _pixmap; }
	virtual int getX() const { return 0; }
	virtual int getY() const { return 0; }
	virtual uint getWidth() const { return _width; }
	virtual uint getHeight() const { return _height; }

	/**
	 * Update pixmap size to match the given size.
	 */
	bool resize(uint width, uint height)
	{
		if (width == _width && height == _height) {
			return false;
		}
		_width = width;
		_height = height;
		if (_pixmap) {
			X11::freePixmap(_pixmap);
		}
		_pixmap = X11::createPixmap(_width, _height);
		return true;
	}

private:
	PPixmapSurface(const PPixmapSurface&);
	PPixmapSurface& operator=(const PPixmapSurface&);

private:
	uint _width;
	uint _height;
	Pixmap _pixmap;
};

#endif // _PEKWM_PPIXMAP_SURFACE_HH_
