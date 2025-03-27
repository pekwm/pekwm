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

SysConfig::SysConfig(Os *os)
	: _os(os),
	  _enable_xsettings(true),
	  _location_lookup(false),
	  _latitude(0.0),
	  _longitude(0.0)
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

	CfgParser::Entry *section = cfg.getEntryRoot()->findSection("SYS");
	if (section) {
		CfgParserKeys keys;
		keys.add_bool("XSETTINGS", _enable_xsettings, true);
		keys.add_bool("LOCATIONLOOKUP", _location_lookup, false);
		keys.add_numeric<double>("LATITUDE", _latitude, 0.0);
		keys.add_numeric<double>("LONGITUDE", _longitude, 0.0);
		keys.add_string("TIMEOFDAY", _tod, "AUTO");
		keys.add_string("NETTHEME", _net_theme, "");
		keys.add_string("NETICONTHEME", _net_icon_theme, "");
		section->parseKeyValues(keys.begin(), keys.end());

		parseCommands(section->findSection("DAYTIMECOMMANDS"),
			      _daytime_commands);
		parseCommands(section->findSection("LOCATIONCOMMANDS"),
			      _location_commands);

		CfgParser::Entry *xresources =
			section->findSection("XRESOURCES");
		if (xresources) {
			parseConfigXResources(xresources, _x_resources_dawn,
					      "DAWN");
			parseConfigXResources(xresources, _x_resources_day,
					      "DAY");
			parseConfigXResources(xresources, _x_resources_dusk,
					      "DUSK");
			parseConfigXResources(xresources, _x_resources_night,
					      "NIGHT");
		}
	}
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

	CfgParser::Entry *section = xresources->findSection(key);
	if (! section) {
		return;
	}

	CfgParser::Entry::entry_cit it(section->begin());
	for (; it != section->end(); ++it) {
		resources[(*it)->getName()] = (*it)->getValue();
	}
}
