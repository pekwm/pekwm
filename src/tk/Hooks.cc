//
// Hooks.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "Hooks.hh"
#include "Mem.hh"

static const char*
_hook_type_to_name(enum HookType type)
{
	switch (type) {
	case PEKWM_HOOK_ON_MONITOR_CHANGE:
		return "OnMonitorChange";
	case PEKWM_HOOK_ON_MONITOR_CONFIGURATION_CHANGE:
		return "OnMonitorConfigurationChange";
	default:
		return "unknown";
	}
}

Hooks::Hooks(Os *os)
	: _os(os)
{
}

Hooks::~Hooks()
{
}

bool
Hooks::load(const std::string &file)
{
	CfgParser cfg(CfgParserOpt(""));
	if (! cfg.parse(file, CfgParserSource::SOURCE_FILE, true)) {
		return false;
	}
	return load(cfg.getEntryRoot()->findSection("HOOKS"));
}

bool
Hooks::load(CfgParser::Entry *section)
{
	_hooks.clear();
	if (section == nullptr) {
		return false;
	}

	std::string on_monitor_change;
	std::string on_monitor_configuration_change;
	CfgParserKeys keys;
	keys.add_string("ONMONITORCHANGE", on_monitor_change, "");
	keys.add_string("ONMONITORCONFIGURATIONCHANGE",
			on_monitor_configuration_change, "");
	section->parseKeyValues(keys.begin(), keys.end());

	_hooks[PEKWM_HOOK_ON_MONITOR_CHANGE] = on_monitor_change;
	_hooks[PEKWM_HOOK_ON_MONITOR_CONFIGURATION_CHANGE] =
		on_monitor_configuration_change;

	return true;
}

bool
Hooks::run(enum HookType type, const std::map<std::string, std::string> &args)
{
	map::iterator hit(_hooks.find(type));
	if (hit == _hooks.end()) {
		P_TRACE("no hook configured for " << _hook_type_to_name(type));
		return false;
	}

	std::vector<std::string> cmd;
	cmd.push_back(hit->second);
	OsEnv env;
	std::map<std::string, std::string>::const_iterator ait(args.begin());
	for (; ait != args.end(); ++ait) {
		env.override(ait->first, ait->second);
	}

	P_TRACE("executing hook script " << hit->second);
	Destruct<ChildProcess> child(_os->childExec(cmd, 0, &env));
	if (*child == nullptr) {
		return false;
	}

	P_TRACE("waiting for hook script to finish");
	int exitcode;
	child->wait(exitcode);
	return exitcode == 0;
}
