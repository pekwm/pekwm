//
// Hooks.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_HOOKS_HH_
#define _PEKWM_HOOKS_HH_

#include "CfgParser.hh"
#include "Os.hh"

#include <map>
#include <string>

enum HookType {
	PEKWM_HOOK_ON_MONITOR_CHANGE,
	PEKWM_HOOK_ON_MONITOR_CONFIGURATION_CHANGE
};

class Hooks {
public:
	typedef std::map<enum HookType, std::string> map;

	Hooks(Os *os);
	~Hooks();

	bool load(const std::string& file);
	bool load(CfgParser::Entry *section);

	bool run(enum HookType hook,
		 const std::map<std::string, std::string> &args);

private:
	Os *_os;
	map _hooks;
};

namespace pekwm
{
	Hooks *hooks();
}

#endif // _PEKWM_HOOKS_HH_
