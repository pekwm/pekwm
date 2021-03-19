//
// WmUtil.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Config.hh"
#include "ImageHandler.hh"
#include "WmUtil.hh"

WithIconPath::WithIconPath(const Config *cfg, ImageHandler *image_handler)
    : _image_handler(image_handler)
{
    _image_handler->path_push_back(cfg->getSystemIconPath());
    _image_handler->path_push_back(cfg->getIconPath());
    _image_handler->path_push_back(cfg->getThemeIconPath());
}

WithIconPath::~WithIconPath(void)
{
    _image_handler->path_pop_back();
    _image_handler->path_pop_back();
    _image_handler->path_pop_back();
}
