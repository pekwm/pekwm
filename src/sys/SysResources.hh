//
// SysResources.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_SYS_RESOURCES_HH_
#define _PEKWM_SYS_RESOURCES_HH_

#include <vector>

#include "Daytime.hh"
#include "SysConfig.hh"
#include "X11.hh"

class SysResources {
public:
	SysResources(const SysConfig &cfg);
	~SysResources();

	void setLocationCountry(const std::string &location_country)
	{
		_location_country = location_country;
	}
	void setLocationCity(const std::string &location_city)
	{
		_location_city = location_city;
	}

	void update(const Daytime &daytime, TimeOfDay tod);

private:
	void setXAtoms(const char *theme_variant);
	void setXResources(const Daytime &daytime, TimeOfDay tod,
			   const char *daylight, const char *theme_variant);
	void notifyXTerms();
	void notifyXTerm(Window win);
	bool readClientList(std::vector<Window> &windowsv);

	const SysConfig &_cfg;
	std::string _location_country;
	std::string _location_city;
};

#endif // _PEKWM_SYS_RESOURCES_HH_
