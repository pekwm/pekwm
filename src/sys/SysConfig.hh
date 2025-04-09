//
// SysConfig.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_SYS_CONFIG_HH_
#define _PEKWM_SYS_CONFIG_HH_

#include "CfgParser.hh"
#include "Daytime.hh"
#include "Os.hh"

extern "C" {
#include <math.h>
}

class SysConfig {
public:
	typedef std::vector<std::string> string_vector;
	typedef std::map<std::string, std::string> string_map;

	SysConfig(Os *os);
	SysConfig(const SysConfig &cfg);
	~SysConfig();

	bool parseConfig();

	bool isXSettingsEnabled() const { return _enable_xsettings; }
	const std::string& getXSettingsPath() const { return _xsettings_path; }

	bool isLocationLookup() const { return _location_lookup; }
	bool haveLocation() const
	{
		return ! isnan(_latitude) && ! isnan(_longitude);
	}
	double getLatitude() const { return _latitude; }
	void setLatitude(double latitude) { _latitude = latitude; }
	double getLongitude() const { return _longitude; }
	void setLongitude(double longitude) { _longitude = longitude; }
	const std::string &getTimeOfDay() const { return _tod; }

	const string_vector &getDaytimeCommands() const {
		return _daytime_commands;
	}
	const string_vector &getLocationCommands() const {
		return _location_commands;
	}

	const std::string &getNetTheme() const { return _net_theme; }
	const std::string &getNetIconTheme() const { return _net_icon_theme; }

	const string_map &getXResources(TimeOfDay tod) const {
		if (tod == TIME_OF_DAY_DAY) {
			return _x_resources_day;
		} else if (tod == TIME_OF_DAY_DAWN
			   && !_x_resources_dawn.empty()) {
			return _x_resources_dawn;
		} else if (tod == TIME_OF_DAY_DUSK
			   && !_x_resources_dawn.empty()) {
			return _x_resources_dusk;
		} else {
			return _x_resources_night;
		}
	}

	void setXResources(TimeOfDay tod, const string_map &x_resources) {
		if (tod == TIME_OF_DAY_DAY) {
			_x_resources_day = x_resources;
		} else if (tod == TIME_OF_DAY_DAWN) {
			_x_resources_dawn = x_resources;
		} else if (tod == TIME_OF_DAY_DUSK) {
			_x_resources_dusk = x_resources;
		} else {
			_x_resources_night = x_resources;
		}
	}

	const string_map &getThemeXResources() const {
		return _theme_x_resources;
	}
	void setThemeXResources(const string_map &theme_x_resources) {
		_theme_x_resources = theme_x_resources;
	}

	static void parseConfigXResources(CfgParser::Entry *section,
					  string_map &resources,
					  const std::string &key);

private:
	void parseCommands(CfgParser::Entry *section, string_vector &commands);

	Os *_os;

	bool _enable_xsettings;
	std::string _xsettings_path;

	bool _location_lookup;
	double _latitude;
	double _longitude;
	std::string _tod;

	/* Commands run whenever daytime changes */
	string_vector _daytime_commands;
	/* Commands run whenever location changes */
	string_vector _location_commands;

	/* If set, update Net/ThemeName XSetting with no suffix or -dark
	 * suffix depending on time of day */
	std::string _net_theme;
	/* Set Net/IconThemeName if non-empty */
	std::string _net_icon_theme;

	/* X resources set on dawn */
	string_map _x_resources_dawn;
	/* X resources set on day */
	string_map _x_resources_day;
	/* X resources set on dusk */
	string_map _x_resources_dusk;
	/* X resources set on night */
	string_map _x_resources_night;
	/* X resources set by current theme */
	string_map _theme_x_resources;
};

#endif // _PEKWM_SYS_CONFIG_HH_
