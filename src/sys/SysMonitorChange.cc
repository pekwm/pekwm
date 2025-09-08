//
// SysResources.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "SysMonitorChange.hh"

SysMonitorChange::SysMonitorChange()
	: _fd(-1)
{
#ifdef PEKWM_HAVE_LIBUDEV
	_udev = udev_new();
	if (_udev == nullptr) {
		P_ERR("failed to create udev");
		return;
	}
	_mon = udev_monitor_new_from_netlink(_udev, "udev");
	if (_mon == nullptr) {
		P_ERR("failed to create udev monitor");
		return;
	}
	udev_monitor_filter_add_match_subsystem_devtype(_mon, "drm", nullptr);
	udev_monitor_enable_receiving(_mon);
	_fd = udev_monitor_get_fd(_mon);
#endif // PEKWM_HAVE_LIBUDEV
}

SysMonitorChange::~SysMonitorChange()
{
#ifdef PEKWM_HAVE_LIBUDEV
	if (_mon) {
		udev_monitor_unref(_mon);
	}
	if (_udev) {
		udev_unref(_udev);
	}
#endif // PEKWM_HAVE_LIBUDEV
}

bool
SysMonitorChange::handle()
{
#ifdef PEKWM_HAVE_LIBUDEV
	struct udev_device *dev = udev_monitor_receive_device(_mon);
	if (dev == nullptr) {
		P_TRACE("no device received");
		return false;
	}
	const char *action = udev_device_get_action(dev);
	const char *sysname = udev_device_get_sysname(dev);
	const char *status = udev_device_get_sysattr_value(dev, "status");
	if (status == nullptr) {
		status = "unknown";
	}

	P_TRACE("libudev: " << action << " sysname: " << sysname << " status: "
		<< status);

	udev_device_unref(dev);
	return true;
#else // !PEKWM_HAVE_LIBUDEV
	return false;
#endif // PEKWM_HAVE_LIBUDEV
}
