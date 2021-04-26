//
// WmUtil.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_WMUTIL_HH_
#define _PEKWM_WMUTIL_HH_

class Config;
class ImageHandler;

/**
 * Resource handling for image loading paths including icon paths for
 * the current theme.
 */
class WithIconPath {
public:
    WithIconPath(const Config *cfg, ImageHandler *image_handler);
    ~WithIconPath(void);

private:
    ImageHandler *_image_handler;
};

#endif // _PEKWM_WMUTIL_HH_
