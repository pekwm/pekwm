//
// pekwm_sys.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_SYS_HH_
#define _PEKWM_SYS_HH_

#include "Compat.hh"
#include "Daytime.hh"
#include "SysConfig.hh"
#include "SysMonitorChange.hh"
#include "SysResources.hh"
#include "Timeouts.hh"
#include "XSettings.hh"

class PekwmSys {
public:
	PekwmSys(const std::string &config_file, bool interactive, Os *os);
	~PekwmSys();

	int main(const std::string &theme);

#ifndef UNITTEST
private:
#endif // UNITTEST
	void handleSigchld();
	void handleXEvent(XEvent &ev);
	void handleStdin();
	void handleSetXSETTING(const std::vector<std::string> &args,
			       enum XSettingType type);
	bool setXSettingInteger(const std::string &key,
				const std::string &sval);
	bool setXSettingColor(const std::string &key, const std::string &sval);
	bool setXSettingString(const std::string &key, const std::string &sval);
	void handleMonitorChange();
	void handleXSave();
	void handleSetTimeOfDay(const std::vector<std::string> &args);
	void handleSetDpi(const std::vector<std::string> &args);
	void handleMonLoad(const std::vector<std::string> &args);
	void handleMonSave(const std::vector<std::string> &args);
	void handleTheme(const StringView &theme);

	void reload();

	bool monLoad();
	bool monAutoConfig();

	TimeOfDay getEffectiveTimeOfDay() const
	{
		return getEffectiveTimeOfDay(_tod);
	}

	TimeOfDay getEffectiveTimeOfDay(TimeOfDay tod) const
	{
		return isTimeOfDayOverride() ? _tod_override : tod;
	}

	bool isTimeOfDayOverride() const
	{
		return _tod_override != static_cast<TimeOfDay>(-1);
	}

	void updateLocation();
	TimeOfDay updateDaytime(time_t now);
	enum TimeOfDay timeOfDayChanged(enum TimeOfDay tod);
	std::string themeSuffix(enum TimeOfDay tod)
	{
		return tod == TIME_OF_DAY_DAY ? "" : "-Dark";
	}

	void setNetTheme(TimeOfDay tod)
	{
		const std::string &theme = _cfg.getNetTheme();
		if (theme.empty()) {
			_xsettings.remove("Net/ThemeName");
		} else {
			_xsettings.setString("Net/ThemeName",
					     theme + themeSuffix(tod));
		}
	}
	void setNetIconTheme()
	{
		const std::string &theme = _cfg.getNetIconTheme();
		if (theme.empty()) {
			_xsettings.remove("Net/IconThemeName");
		} else {
			_xsettings.setString("Net/IconThemeName", theme);
		}
	}
	void setDpi()
	{
		if (_cfg.haveDpi()) {
			int dpi = static_cast<int>(_cfg.getDpi() * 1024);
			_xsettings.setInt32("Xft/DPI", dpi);
		}
	}

	bool _stop;
	Timeouts _timeouts;
	bool _interactive;
	Os *_os;
	OsSelect *_select;
	SysConfig _cfg;
	SysResources _resources;
	SysMonitorChange _monitor_change;
	XSettings _xsettings;
	Daytime _daytime;
	TimeOfDay _tod;
	TimeOfDay _tod_override;
};

#endif // _PEKWM_SYS_HH_
