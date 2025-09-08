//
// SysMonitorChange.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_SYS_MONITOR_CHANGE_HH_
#define _PEKWM_SYS_MONITOR_CHANGE_HH_

#include "config.h"

#include "Compat.hh"

extern "C" {
#ifdef PEKWM_HAVE_LIBUDEV
#include <libudev.h>
#endif // PEKWM_HAVE_LIBUDEV
}

class SysMonitorChange {
public:
	SysMonitorChange();
	~SysMonitorChange();

	int getFd() const { return _fd; }

	bool handle();

private:
	SysMonitorChange(const SysMonitorChange&);
	SysMonitorChange& operator=(const SysMonitorChange&);

	int _fd;
#ifdef PEKWM_HAVE_LIBUDEV
	struct udev *_udev;
	struct udev_monitor *_mon;
#endif // PEKWM_HAVE_LIBUDEV
};

#endif // _PEKWM_SYS_MONITOR_CHANGE_HH_
