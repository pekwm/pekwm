//
// pekwm.hh for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PEKWM_HH_
#define _PEKWM_PEKWM_HH_

#include "config.h"

#include <string>

#include "AppCtrl.hh"
#include "EventLoop.hh"
#include "Os.hh"
#include "Timeouts.hh"

class Config;
class RootWO;

namespace pekwm
{
	void initNoDisplay(void);
	void cleanupNoDisplay(void);

	bool init(AppCtrl* app_ctrl, EventLoop* event_loop, Os *os,
		  Display* dpy, const std::string& config_file,
		  bool replace, bool synchronous, bool standalone);
	void cleanup(void);

	bool isStarting(void);
	void setStarted(void);

	Timeouts *timeouts();

	void setConfig(Config* cfg);
	void setRootWO(RootWO* root_wo);
}

#endif // _PEKWM_PEKWM_HH_
