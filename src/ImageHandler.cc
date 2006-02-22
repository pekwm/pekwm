//
// ImageHandler.cc for pekwm
// Copyright (C)  2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#include "Config.hh"
#include "PImage.hh"
#include "ImageHandler.hh"
#include "PImageNative.hh"
#include "PImageNativeLoaderJpeg.hh"
#include "PImageNativeLoaderPng.hh"
#include "PImageNativeLoaderXpm.hh"
#include "PScreen.hh"
#include "Util.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUG
using std::list;
using std::vector;
using std::string;

ImageHandler *ImageHandler::_instance = NULL;

//! @brief ImageHandler constructor
ImageHandler::ImageHandler(void) :
_free_on_return(false)
{
#ifdef DEBUG
  if (_instance != NULL) {
    cerr << __FILE__ << "@" << __LINE__ << ": "
         << "ImageHandler(" << this << ")::ImageHandler()"
         << " *** _instance allready set: " << _instance << endl;
  }
#endif // DEBUG
  _instance = this;

	// setup parsing maps
	_image_type_map[""] = IMAGE_TYPE_NO;
	_image_type_map["TILED"] = IMAGE_TYPE_TILED;
	_image_type_map["SCALED"] = IMAGE_TYPE_SCALED;
	_image_type_map["FIXED"] = IMAGE_TYPE_FIXED;

#ifdef HAVE_IMAGE_JPEG
	PImageNative::loaderAdd(new PImageNativeLoaderJpeg());
#endif // HAVE_IMAGE_JPEG
#ifdef HAVE_IMAGE_PNG
	PImageNative::loaderAdd(new PImageNativeLoaderPng());
#endif // HAVE_IMAGE_PNG
#ifdef HAVE_IMAGE_XPM
	PImageNative::loaderAdd(new PImageNativeLoaderXpm());
#endif // HAVE_IMAGE_XPM
}

//! @brief ImageHandler destructor
ImageHandler::~ImageHandler(void)
{
  PImageNative::loaderClear();
}

//! @brief Gets or creates a Image
PImage*
ImageHandler::getImage(const std::string &file)
{
	string real_file(_dir + file);

	// check cache
	list<HandlerEntry<PImage*> >::iterator it(_image_list.begin());
	for (; it != _image_list.end(); ++it) {
		if (*it == real_file) {
			it->incRef();
			return it->getData();
		}
	}

	// Create new PImageNative
	PImage *image;
	image = new PImageNative(PScreen::instance()->getDpy());

	vector<string> tok;
	if ((Util::splitString(file, tok, "#", 2)) == 2) {
		image->load(_dir + tok[0]);
		image->setType(ParseUtil::getValue<ImageType>(tok[1], _image_type_map));
	} else {
		image->load(real_file);
		image->setType(IMAGE_TYPE_TILED);
	}

	// create new entry
	HandlerEntry<PImage*> entry(real_file);
	entry.incRef();
	entry.setData(image);

	_image_list.push_back(entry);

	return image;
}

//! @brief Returns a Image
void
ImageHandler::returnImage(PImage *image)
{
	list<HandlerEntry<PImage*> >::iterator it(_image_list.begin());
	for (; it != _image_list.begin(); ++it) {
		if (it->getData() == image) {
			it->decRef();
			if ((it->getRef() == 0) && (_free_on_return == true)) {
				delete it->getData();
				_image_list.erase(it);
			}
			break;
		}
	}
}

//! @brief Frees all images not beeing in use
void
ImageHandler::freeUnref(void)
{
	list<HandlerEntry<PImage*> >::iterator it(_image_list.begin());
	while (it != _image_list.end()) {
		if (it->getRef() == 0) {
			delete it->getData();
			it = _image_list.erase(it);
		} else {
			++it;
		}
	}
}
