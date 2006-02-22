//
// ImageHandler.hh for pekwm
// Copyright (C)  2003-2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _IMAGE_HANDLER_HH_
#define _IMAGE_HANDLER_HH_

#include "Handler.hh"
#include "ParseUtil.hh"

#include <string>
#include <list>

class PImage;

//! @brief ImageHandler, a caching and image type transparent image handler.
class ImageHandler {
public:
	ImageHandler(void);
	~ImageHandler(void);

	//! @brief Returns the ImageHandler instance pointer.
	static inline ImageHandler *instance(void) { return _instance; }

	//! @brief Sets loading base path.
	inline void setDir(const std::string &dir) { _dir = dir; }

	PImage *getImage(const std::string &file);
	void returnImage(PImage *image);

	void freeUnref(void);

private:
	std::string _dir;

	std::list<HandlerEntry<PImage*> > _image_list;

	bool _free_on_return;

	std::map<ParseUtil::Entry, ImageType> _image_type_map;

	static ImageHandler *_instance;
};

#endif // _IMAGE_HANDLER_HH_
