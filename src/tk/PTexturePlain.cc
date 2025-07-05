//
// PTexturePlain.cc for pekwm
// Copyright (C) 2004-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Color.hh"
#include "PTexture.hh"
#include "PTexturePlain.hh"
#include "PImage.hh"
#include "Render.hh"
#include "ImageHandler.hh"
#include "X11.hh"

#include <iostream>

extern "C" {
#include <assert.h>
}

// PTextureEmpty

void
PTextureEmpty::doRender(Render &rend,
			int x, int y, size_t width, size_t height)
{
}

bool
PTextureEmpty::getPixel(ulong &pixel) const
{
	pixel = X11::getWhitePixel();
	return true;
}

// PTextureSolid

PTextureSolid::PTextureSolid(const std::string &color)
	: PTexture(),
	  _xc(0)
{
	setColor(color);
}

PTextureSolid::~PTextureSolid(void)
{
	unsetColor();
}

// BEGIN - PTexture interface.

/**
 * Render single color on draw.
 */
void
PTextureSolid::doRender(Render &rend, int x, int y, size_t width, size_t height)
{
	rend.setColor(_xc->pixel);
	rend.fill(x, y, width, height);
}

// END - PTexture interface.

/**
 * Load color resources
 */
bool
PTextureSolid::setColor(const std::string &color)
{
	unsetColor(); // unload used resources

	_xc = pekwm::getColor(color);
	_ok = true;

	return _ok;
}

/**
 * Frees color resources used by texture
 */
void
PTextureSolid::unsetColor(void)
{
	X11::returnColor(_xc);
	_ok = false;
}

// PTextureSolidRaised

PTextureSolidRaised::PTextureSolidRaised(const std::string &base,
					 const std::string &hi,
					 const std::string &lo)
	: PTextureAreaRender(),
	  _xc_base(0),
	  _xc_hi(0),
	  _xc_lo(0),
	  _lw(1),
	  _loff(0),
	  _draw_top(true),
	  _draw_bottom(true),
	  _draw_left(true),
	  _draw_right(true)
{
	setColor(base, hi, lo);
}

PTextureSolidRaised::~PTextureSolidRaised(void)
{
	unsetColor();
}

// START - PTexture interface.

struct RenderAndTextureAreaRender
{
	RenderAndTextureAreaRender(Render &_rend, PTextureAreaRender *_tex)
		: rend(_rend),
		  tex(_tex)
	{
	}

	Render &rend;
	PTextureAreaRender *tex;
};

void
renderWithTexture(int x, int y, uint width, uint height, void *opaque)
{
	RenderAndTextureAreaRender *rat =
		reinterpret_cast<RenderAndTextureAreaRender*>(opaque);
	rat->tex->renderArea(rat->rend, x, y, width, height);
}

/**
 * Renders a "raised" rectangle onto draw.
 */
void
PTextureSolidRaised::doRender(Render &rend,
			      int x, int y, size_t width, size_t height)
{
	if (_width && _height) {
		// size was given in the texture, repeat the texture over the
		// provided geometry
		RenderAndTextureAreaRender rat(rend, this);
		renderTiled(x, y, width, height, _width, _height,
			    renderWithTexture, reinterpret_cast<void*>(&rat));
	} else {
		// no size given, treat provided geometry as area
		renderArea(rend, x, y, width, height);
	}
}

void
PTextureSolidRaised::renderArea(Render &rend,
				int x, int y, size_t width, size_t height)
{
	rend.setLineWidth(_lw);

	// base rectangle
	rend.setColor(_xc_base->pixel);
	rend.fill(x, y, width, height);

	// hi line ( consisting of two lines )
	rend.setColor(_xc_hi->pixel);
	if (_draw_top) {
		rend.line(x + _loff, y + _loff,
			  x + width - _loff - _lw, y + _loff);
	}
	if (_draw_left) {
		rend.line(x + _loff, y + _loff,
			  x + _loff, y + height - _loff - _lw);
	}

	// lo line ( consisting of two lines )
	rend.setColor(_xc_lo->pixel);
	if (_draw_bottom) {
		rend.line(x + _loff + _lw,
			  y + height - _loff - (_lw ? _lw : 1),
			  x + width - _loff - _lw,
			  y + height - _loff - (_lw ? _lw : 1));
	}
	if (_draw_right) {
		rend.line(x + width - _loff - (_lw ? _lw : 1),
			  y + _loff + _lw,
			  x + width - _loff - (_lw ? _lw : 1),
			  y + height - _loff - _lw);
	}
}

// END - PTexture interface.

/**
 * Sets line width
 */
void
PTextureSolidRaised::setLineWidth(uint lw)
{
	_lw = lw < 1 ? 1 : lw;
}

/**
 * Loads color resources
 */
bool
PTextureSolidRaised::setColor(const std::string &base,
			      const std::string &hi, const std::string &lo)
{
	unsetColor(); // unload used resources

	_xc_base = pekwm::getColor(base);
	_xc_hi = pekwm::getColor(hi);
	_xc_lo = pekwm::getColor(lo);

	_ok = true;

	return _ok;
}

/**
 * Free color resources
 */
void
PTextureSolidRaised::unsetColor(void)
{
	X11::returnColor(_xc_base);
	X11::returnColor(_xc_hi);
	X11::returnColor(_xc_lo);
	_ok = false;
}

// PTextureLines

PTextureLines::PTextureLines(float line_size, bool size_percent, bool horz,
			     std::vector<std::string>::const_iterator cbegin,
			     std::vector<std::string>::const_iterator cend)
	: _line_size(line_size),
	  _size_percent(size_percent),
	  _horz(horz)
{
	setColors(cbegin, cend);
}

PTextureLines::~PTextureLines()
{
}

void
PTextureLines::doRender(Render &rend, int x, int y, size_t width, size_t height)
{
	if (_width && _height) {
		// size was given in the texture, repeat the texture over the
		// provided geometry
		RenderAndTextureAreaRender rat(rend, this);
		renderTiled(x, y, width, height, _width, _height,
			    renderWithTexture, reinterpret_cast<void*>(&rat));
	} else {
		// no size given, treat provided geometry as area
		renderArea(rend, x, y, width, height);
	}
}

void
PTextureLines::renderArea(Render &rend, int x, int y,
			  size_t width, size_t height)
{
	if (_horz) {
		renderHorz(rend, x, y, width, height);
	} else {
		renderVert(rend, x, y, width, height);
	}
}

void
PTextureLines::renderHorz(Render &rend, int x, int y,
			  size_t width, size_t height)
{
	size_t line_height;
	if (_size_percent) {
		line_height = static_cast<size_t>(static_cast<float>(height)
						  * _line_size);
	} else {
		line_height = static_cast<size_t>(_line_size);
	}

	// ensure code does not get stuck never increasing pos
	if (line_height < 1) {
		line_height = 1;
	}

	size_t pos = 0;
	while (pos < height) {
		std::vector<XColor*>::iterator it = _colors.begin();
		for (; pos < height && it != _colors.end(); ++it) {
			rend.setColor((*it)->pixel);
			rend.fill(x, y + pos,
				  width, std::min(line_height, height - pos));
			pos += line_height;
		}
	}
}

void
PTextureLines::renderVert(Render &rend, int x, int y,
			  size_t width, size_t height)
{
	size_t line_width;
	if (_size_percent) {
		line_width = static_cast<size_t>(static_cast<float>(width)
						 * _line_size);
	} else {
		line_width = static_cast<size_t>(_line_size);
	}

	size_t pos = 0;
	while (pos < width) {
		std::vector<XColor*>::iterator it = _colors.begin();
		for (; pos < width && it != _colors.end(); ++it) {
			rend.setColor((*it)->pixel);
			rend.fill(x + pos, y,
				  std::min(line_width, width - pos), height);
			pos += line_width;
		}
	}
}

void
PTextureLines::setColors(std::vector<std::string>::const_iterator cbegin,
			 std::vector<std::string>::const_iterator cend)
{
	unsetColors();

	std::vector<std::string>::const_iterator it(cbegin);
	for (; it != cend; ++it) {
		_colors.push_back(pekwm::getColor(*it));
	}
	_ok = ! _colors.empty();
}

void
PTextureLines::unsetColors()
{
	std::vector<XColor*>::iterator it = _colors.begin();
	for (; it != _colors.end(); ++it) {
		X11::returnColor(*it);
	}
	_colors.clear();
}

// PTextureImage

PTextureImage::PTextureImage(PImage *image)
	: _image(nullptr)
{
	setImage(image);
}

PTextureImage::PTextureImage(const std::string &image,
			     const std::string &colormap)
	: _image(nullptr),
	  _colormap(colormap)
{
	setImage(image, colormap);
}

PTextureImage::~PTextureImage(void)
{
	unsetImage();
}

/**
 * Renders image onto draw
 */
void
PTextureImage::doRender(Render &rend, int x, int y, size_t width, size_t height)
{
	Geometry gm(0, 0, x + width, y + height);
	RenderSurface surface(rend, gm);
	_image->draw(rend, &surface, x, y, width, height);
}

Pixmap
PTextureImage::getMask(size_t width, size_t height, bool &do_free)
{
	return _image->getMask(do_free, width, height);
}

/**
 * Set image resource
 */
void
PTextureImage::setImage(PImage *image)
{
	unsetImage();
	_image = image;
	_colormap = "";
	_width = _image->getWidth();
	_height = _image->getHeight();
	pekwm::imageHandler()->takeOwnership(image);
	_ok = true;
}

/**
 * Load image resource
 */
bool
PTextureImage::setImage(const std::string &image, const std::string &colormap)
{
	unsetImage();

	if (colormap.empty()) {
		_image = pekwm::imageHandler()->getImage(image);
	} else {
		_image = pekwm::imageHandler()->getMappedImage(image, colormap);
	}

	if (_image) {
		_colormap = colormap;
		_width = _image->getWidth();
		_height = _image->getHeight();
		_ok = true;
	} else {
		_ok = false;
	}
	return _ok;
}

/**
 * Free image resource
 */
void
PTextureImage::unsetImage(void)
{
	if (_ok) {
		if (_colormap.empty()) {
			pekwm::imageHandler()->returnImage(_image);
		} else {
			pekwm::imageHandler()->returnMappedImage(_image,
								 _colormap);
		}
	}
	_image = nullptr;
	_colormap = "";
	_width = 0;
	_height = 0;
	_ok = false;
}
