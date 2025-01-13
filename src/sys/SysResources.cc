//
// SysResources.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "SysResources.hh"
#include "X11.hh"

SysResources::SysResources(const SysConfig &cfg)
	: _cfg(cfg)
{
}

SysResources::~SysResources()
{
}

void
SysResources::update(TimeOfDay tod)
{
	const char *daylight;
	const char *theme_variant;
	switch (tod) {
	case TIME_OF_DAY_DAWN:
		daylight = "dawn";
		theme_variant = "dark";
		break;
	case TIME_OF_DAY_DUSK:
		daylight = "dusk";
		theme_variant = "dark";
		break;
	case TIME_OF_DAY_NIGHT:
		daylight = "night";
		theme_variant = "dark";
		break;
	case TIME_OF_DAY_DAY:
	default:
		daylight = "day";
		theme_variant = "light";
		break;
	};

	setXAtoms(theme_variant);
	setXResources(tod, daylight, theme_variant);
	X11::flush();
}

void
SysResources::setXAtoms(const char *theme_variant)
{
	X11::setString(X11::getRoot(), PEKWM_THEME_VARIANT, theme_variant);
}

void
SysResources::setXResources(TimeOfDay tod, const char *daylight,
			    const char *theme_variant)
{
	X11::loadXrmResources();
	X11::setXrmString("pekwm.daylight", daylight);
	X11::setXrmString("pekwm.theme.variant", theme_variant);
	X11::setXrmString("pekwm.location.latitude",
			  std::to_string(_cfg.getLatitude()));
	X11::setXrmString("pekwm.location.longitude",
			  std::to_string(_cfg.getLongitude()));
	X11::setXrmString("pekwm.location.country", _location_country);
	X11::setXrmString("pekwm.location.city", _location_city);

	const SysConfig::string_map &x_resources = _cfg.getXResources(tod);
	SysConfig::string_map::const_iterator it(x_resources.begin());
	for (; it != x_resources.end(); ++it) {
		X11::setXrmString(it->first, it->second);
	}
	P_TRACE("set " << x_resources.size()
		<< " X resources from configuration");

	X11::saveXrmResources();
}
