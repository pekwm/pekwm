//
// PImageIcon.cc for pekwm
// Copyright (C) 2007-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <cstring>
#include <iostream>

#include "PImageIcon.hh"

/**
 * New PImageIcon copying image data from image.
 */
PImageIcon::PImageIcon(PImage *image)
	: _cardinals(nullptr)
{
	_type = IMAGE_TYPE_SCALED;
	_use_alpha = true;
	_width = image->getWidth();
	_height = image->getHeight();
	_data = new uchar[_width * _height * 4];
	memcpy(_data, image->getData(), _width * _height * 4);
}

PImageIcon::PImageIcon(void)
	: _cardinals(nullptr)
{
	_type = IMAGE_TYPE_SCALED;
	_use_alpha = true;
}

/**
 * PImage destructor.
 */
PImageIcon::~PImageIcon(void)
{
	if (_cardinals) {
		delete [] _cardinals;
	}
}

/**
 * Load icon from window (if atom is set)
 */
PImageIcon*
PImageIcon::newFromWindow(Window win)
{
	PImageIcon *icon = nullptr;

	uchar *udata = 0;
	ulong expected = 2, actual;
	if (X11::getProperty(win, X11::getAtom(NET_WM_ICON), XA_CARDINAL,
			     expected, &udata, &actual)) {
		if (actual >= expected) {
			icon = new PImageIcon();
			if (! icon->setImageFromData(udata, actual)) {
				delete icon;
				icon = nullptr;
			}
		}
		X11::free(udata);
	}
	return icon;
}

/**
 * Set _NET_WM_ICON on window.
 */
void
PImageIcon::setOnWindow(Window win)
{
	if (_cardinals == nullptr) {
		_cardinals = newCardinals(_width, _height, _data);
	}
	size_t pixels = _width * _height;
	X11::setCardinals(win, NET_WM_ICON, _cardinals, pixels + 2);
}

/**
 * Set _NET_WM_ICON on window using image.data.
 */
void
PImageIcon::setOnWindow(Window win, size_t width, size_t height, uchar *data)
{
	size_t pixels = width * height;
	Cardinal *cardinals = newCardinals(width, height, data);
	X11::setCardinals(win, NET_WM_ICON, cardinals, pixels + 2);
	delete [] cardinals;
}

Cardinal*
PImageIcon::newCardinals(size_t width, size_t height, uchar *data)
{
	size_t pixels = width * height;
	Cardinal *cardinals = new Cardinal[pixels + 2];
	cardinals[0] = width;
	cardinals[1] = height;
	toCardinals(pixels, data, cardinals + 2);
	return cardinals;
}

/**
 * Do the actual reading and loading of the icon data in ARGB data.
 */
bool
PImageIcon::setImageFromData(uchar *udata, ulong actual)
{
	// Icon size successfully read, proceed with loading the actual
	// icon data.
	Cardinal *from_data = reinterpret_cast<Cardinal*>(udata);
	size_t width = from_data[0];
	size_t height = from_data[1];
	size_t pixels = width * height;
	if (actual < (pixels + 2)) {
		return false;
	}

	_width = width;
	_height = height;

	_data = new uchar[pixels * 4];
	fromCardinals(pixels, from_data, _data);
	_pixmap = createPixmap(_data, _width, _height);
	_mask =  createMask(_data, _width, _height);

	return true;
}

void
PImageIcon::fromCardinals(size_t pixels, Cardinal *from_data, uchar *to_data)
{
	Cardinal *src = from_data;
	uchar *dst = to_data;
	for (size_t i = 0; i < pixels; i += 1) {
		int pixel = *src++;
		*dst++ = pixel >> 24 & 0xff;
		*dst++ = pixel >> 16 & 0xff;
		*dst++ = pixel >> 8 & 0xff;
		*dst++ = pixel & 0xff;
	}
}

void
PImageIcon::toCardinals(size_t pixels, uchar *from_data, Cardinal *to_data)
{
	uchar *src = from_data;
	Cardinal *dst = to_data;
	for (size_t i = 0; i < pixels; i += 1) {
		int pixel = (src[0] << 24)
			| (src[1] << 16)
			| (src[2] << 8)
			| src[3];
		src += 4;
		*dst++ = pixel;
	}
}
