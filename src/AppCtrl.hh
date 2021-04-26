//
// AppCtrl.hh for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_APPCTRL_HH_
#define _PEKWM_APPCTRL_HH_

#include <string>

/**
 * Application control interface, allows for reloading/stopping the
 * application.
 */
class AppCtrl {
public:
    virtual void reload(void) = 0;
    virtual void restart(void) = 0;
    virtual void restart(std::string cmd) = 0;
    virtual void shutdown(void) = 0;
};

#endif // _PEKWM_APPCTRL_HH_
