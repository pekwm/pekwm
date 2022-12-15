//
// Color.cc for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Color.hh"
#include "Debug.hh"
#include "String.hh"
#include "Util.hh"

#include <set>

static std::map<std::string, std::string> _resources =
	std::map<std::string, std::string>();

static std::string stripResPrefix(const std::string& desc)
{
	return desc.substr(1);
}

static std::string
lookupResource(const std::string& desc)
{
	std::string res = stripResPrefix(desc);
	std::string val = X11::getXrmString(res);
	_resources[res] = val;
	return val;
}

static bool
isResourceDesc(const std::string& desc)
{
    return desc.size() > 0 && desc[0] == '&';
}

static void
logResourceLoop(const std::set<std::string> &visisted)
{
	std::ostringstream oss;
	oss << "resource loop detected" << std::endl;

	std::set<std::string>::const_iterator it(visisted.begin());
	for (; it != visisted.end(); ++it) {
		oss << "  " << *it << std::endl;
	}

	P_WARN(oss.str());
}

/**
 * Clear registered color resources from previous lookups.
 */
void
pekwm::clearColorResources(void)
{
	_resources.clear();
}

/**
 * Get map of registered color resources and their values.
 */
const std::map<std::string, std::string>&
pekwm::getColorResources(void)
{
	return _resources;
}

/**
 * Lookup color, expanding & prefix recursively from XResources.
 */
std::string
pekwm::getColorResource(std::string desc)
{
	std::set<std::string> visited;

	while (isResourceDesc(desc)) {
		std::set<std::string>::iterator it = visited.find(desc);
		if (it != visited.end()) {
			logResourceLoop(visited);
			return std::string();
		}
		visited.insert(desc);
		desc = lookupResource(desc);
	}
	return desc;
}

/**
 * Get XColor from color description, supports special & prefix
 * where the actual color value is looked up from XResources.
 * Nesting of & prefix is supported.
 */
XColor*
pekwm::getColor(const std::string& desc)
{
	if (isResourceDesc(desc)) {
		std::string color = getColorResource(desc);
		return X11::getColor(color);
	}
	return X11::getColor(desc);
}
