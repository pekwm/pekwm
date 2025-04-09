//
// SysConfig.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "CfgParser.hh"
#include "Debug.hh"
#include "SysConfig.hh"
#include "Util.hh"

extern "C" {
#include <math.h>
}

SysConfig::SysConfig(Os *os)
	: _os(os),
	  _enable_xsettings(true),
	  _xsettings_path("~/.pekwm/xsettings.save"),
	  _location_lookup(false),
	  _latitude(NAN),
	  _longitude(NAN)
{
	Util::expandFileName(_xsettings_path);
}

SysConfig::SysConfig(const SysConfig &cfg)
	: _os(cfg._os),
	  _enable_xsettings(cfg._enable_xsettings),
	  _xsettings_path(cfg._xsettings_path),
	  _location_lookup(cfg._location_lookup),
	  _latitude(cfg._latitude),
	  _longitude(cfg._longitude),
	  _tod(cfg._tod),
	  _daytime_commands(cfg._daytime_commands),
	  _location_commands(cfg._location_commands),
	  _net_theme(cfg._net_theme),
	  _net_icon_theme(cfg._net_icon_theme),
	  _x_resources_dawn(cfg._x_resources_dawn),
	  _x_resources_day(cfg._x_resources_day),
	  _x_resources_dusk(cfg._x_resources_dusk),
	  _x_resources_night(cfg._x_resources_night)
{
}

SysConfig::~SysConfig()
{
}

bool
SysConfig::parseConfig()
{
	std::string config_file;
	if (! _os->getEnv("PEKWM_CONFIG_FILE", config_file)) {
		return false;
	}

	CfgParser cfg(CfgParserOpt(""));
	if (! cfg.parse(config_file, CfgParserSource::SOURCE_FILE, true)) {
		return false;
	}

	CfgParserKeys keys;
	keys.add_bool("XSETTINGS", _enable_xsettings, true);
	keys.add_path("XSETTINGSPATH", _xsettings_path,
		      "~/.pekwm/xsettings.save");
	keys.add_bool("LOCATIONLOOKUP", _location_lookup, false);
	keys.add_numeric<double>("LATITUDE", _latitude, NAN, -90.0, 90.0);
	keys.add_numeric<double>("LONGITUDE", _longitude, NAN, -180.0, 180.0);
	keys.add_string("TIMEOFDAY", _tod, "AUTO");
	keys.add_string("NETTHEME", _net_theme, "");
	keys.add_string("NETICONTHEME", _net_icon_theme, "");

	CfgParser::Entry *sys = cfg.getEntryRoot()->findSection("SYS");
	CfgParser::Entry *xresources;
	if (sys) {
		sys->parseKeyValues(keys.begin(), keys.end());
		parseCommands(sys->findSection("DAYTIMECOMMANDS"),
			      _daytime_commands);
		parseCommands(sys->findSection("LOCATIONCOMMANDS"),
			      _location_commands);
		xresources = sys->findSection("XRESOURCES");
	} else {
		keys.set_defaults();

		_daytime_commands.clear();
		_location_commands.clear();
		xresources = nullptr;
	}

	parseConfigXResources(xresources, _x_resources_dawn, "DAWN");
	parseConfigXResources(xresources, _x_resources_day, "DAY");
	parseConfigXResources(xresources, _x_resources_dusk, "DUSK");
	parseConfigXResources(xresources, _x_resources_night, "NIGHT");

	return true;
}

void
SysConfig::parseCommands(CfgParser::Entry *section, string_vector &commands)
{
	commands.clear();
	if (! section) {
		return;
	}

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		std::string command = (*it)->getValue();
		Util::expandFileName(command);
		commands.push_back(command);
	}
}

void
SysConfig::parseConfigXResources(CfgParser::Entry *xresources,
				 string_map &resources, const std::string &key)
{
	resources.clear();
	CfgParser::Entry *section =
		xresources ? xresources->findSection(key) : nullptr;
	if (! section) {
		return;
	}

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		resources[(*it)->getName()] = (*it)->getValue();
	}
}
