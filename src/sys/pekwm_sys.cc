//
// pekwm_sys.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm_sys.hh"
#include "Calendar.hh"
#include "Compat.hh"
#include "CfgParser.hh"
#include "Debug.hh"
#include "String.hh"
#include "SysMonitorConfig.hh"
#include "Location.hh"
#include "Mem.hh"
#include "Util.hh"
#include "X11.hh"

#include "../tk/Hooks.hh"
#include "../tk/ThemeUtil.hh"

extern "C" {
#include <getopt.h>
#include <signal.h>
}

enum PekwmSysAction {
	PEKWM_SYS_DAY_CHANGED
};

static bool _is_sigchld = false;
static Hooks *_hooks = nullptr;

#ifndef UNITTEST

static const char *progname = nullptr;

extern "C" {

static void sigHandler(int signal)
{
	switch (signal) {
	case SIGCHLD:
		_is_sigchld = true;
		break;
	default:
		// do nothing
		break;
	}
}

}

#endif // UNITTEST

/** static pekwm resources, accessed via the pekwm namespace. */
static std::string _config_script_path;

namespace pekwm
{
	const std::string& configScriptPath()
	{
		return _config_script_path;
	}

	Hooks *hooks()
	{
		return _hooks;
	}
}

PekwmSys::PekwmSys(const std::string &config_file, bool interactive, Os *os)
	: _stop(false),
	  _interactive(interactive),
	  _os(os),
	  _select(mkOsSelect()),
	  _cfg(config_file, os),
	  _resources(_cfg),
	  _tod(static_cast<TimeOfDay>(-1)),
	  _tod_override(static_cast<TimeOfDay>(-1))
{
}

PekwmSys::~PekwmSys()
{
	delete _select;
}

int
PekwmSys::main(const std::string &theme)
{
	if (! _cfg.parseConfig()) {
		std::cerr << "failed to parse configuration" << std::endl;
		return 1;
	}
	if (! pekwm::ascii_ncase_equal(_cfg.getTimeOfDay(), "AUTO")) {
		P_TRACE("using static time of day " << _cfg.getTimeOfDay());
		time_of_day_from_string(_cfg.getTimeOfDay(), _tod_override);
	}

	// load theme before time of day detection, to get the right resources
	// set the first time around
	handleTheme(theme);

	// get location in order to have correct theme and X resources setup.
	updateLocation();

	// init time of day
	TimeOfDay tod = updateDaytime(time(NULL));
	_tod = timeOfDayChanged(getEffectiveTimeOfDay(tod));

	// run after first time of day change to get properties initial values
	// set.
	if (! _cfg.getXSettingsPath().empty()
	    && _os->pathExists(_cfg.getXSettingsPath())) {
		_xsettings.load(_cfg.getXSettingsPath());
	}
	if (_cfg.isXSettingsEnabled()) {
		_xsettings.setServerOwner();
		if (! _cfg.getNetIconTheme().empty()) {
			setNetIconTheme();
		}
		setDpi();
		_xsettings.updateServer();
	}

	_select->add(STDIN_FILENO, OsSelect::OS_SELECT_READ);
	_select->add(X11::getFd(), OsSelect::OS_SELECT_READ);
	if (_monitor_change.getFd() >= 0) {
		P_TRACE("listening for udev changes on "
			<< _monitor_change.getFd());
		_select->add(_monitor_change.getFd(), OsSelect::OS_SELECT_READ);
	}

	P_TRACE("Enter event loop.");
	do {
		if (_is_sigchld) {
			handleSigchld();
			_is_sigchld = false;
		}

		XEvent ev;
		struct timeval *tv;
		TimeoutAction action;
		if (_timeouts.getNextTimeout(&tv, action)) {
			// only action handled is time of day change
			tod = updateDaytime(time(NULL));
			_tod = timeOfDayChanged(getEffectiveTimeOfDay(tod));
		} else if (X11::pending() > 0) {
			X11::getNextEvent(ev);
			handleXEvent(ev);
		} else if (_select->wait(tv)) {
			if (_select->isSet(X11::getFd(),
					   OsSelect::OS_SELECT_READ)) {
				X11::getNextEvent(ev);
				handleXEvent(ev);
			} else if (_select->isSet(STDIN_FILENO,
						  OsSelect::OS_SELECT_READ)) {
				handleStdin();
			} else if (_select->isSet(_monitor_change.getFd(),
						  OsSelect::OS_SELECT_READ)) {
				handleMonitorChange();
			}
		}
	} while (! _stop);

	return 0;
}

void
PekwmSys::handleSigchld()
{
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0) {
	}
}

void
PekwmSys::handleXEvent(XEvent &ev)
{
	P_TRACE("got X11 event " << X11::getEventTypeString(ev.type));
	switch (ev.type) {
	case DestroyNotify:
		if (_cfg.isXSettingsEnabled() && _xsettings.setServerOwner()) {
			_xsettings.updateServer();
		}
		break;
	case SelectionClear:
		if (ev.xselectionclear.selection == _xsettings.getAtom()) {
			_xsettings.setOwner(false);
			_xsettings.selectOwnerDestroyInput();
			P_LOG("no longer owner of xsettings");
		}
		break;
	default:
		break;
	};
}

void
PekwmSys::handleStdin()
{
	std::string buf;
	std::vector<std::string> args;
	if (_interactive) {
		std::getline(std::cin, buf);
		args = StringUtil::shell_split(buf);
	} else {
		uint32_t len;
		std::cin.read(reinterpret_cast<char*>(&len), sizeof(len));

		buf.resize(len + 1);
		std::cin.read(const_cast<char*>(buf.data()), len);
		args = StringUtil::shell_split(buf);
	}

	std::string command = args[0];
	args.erase(args.begin());

	if (pekwm::ascii_ncase_equal(command, "EXIT")) {
		_stop = true;
	} else if (pekwm::ascii_ncase_equal(command, "RELOAD")) {
		reload();
	} else if (pekwm::ascii_ncase_equal(command, "THEME")) {
		handleTheme(StringView(buf, 0, 6));
		_resources.notifyXTerms();
	} else if (pekwm::ascii_ncase_equal(command, "TIMEOFDAY")) {
		handleSetTimeOfDay(args);
	} else if (pekwm::ascii_ncase_equal(command, "DPI")) {
		handleSetDpi(args);
	} else if (pekwm::ascii_ncase_equal(command, "MONLOAD")) {
		handleMonLoad(args);
	} else if (pekwm::ascii_ncase_equal(command, "MONSAVE")) {
		handleMonSave(args);
	} else if (pekwm::ascii_ncase_equal(command, "XSET")) {
		handleSetXSETTING(args, XSETTING_TYPE_STRING);
	} else if (pekwm::ascii_ncase_equal(command, "XSETCOLOR")) {
		handleSetXSETTING(args, XSETTING_TYPE_COLOR);
	} else if (pekwm::ascii_ncase_equal(command, "XSETINT")) {
		handleSetXSETTING(args, XSETTING_TYPE_INTEGER);
	} else if (pekwm::ascii_ncase_equal(command, "XSAVE")) {
		handleXSave();
	} else {
		// unknown command
		P_DBG("unknown command: " << command);
	}
}

void
PekwmSys::handleMonitorChange()
{
	if (_monitor_change.handle()) {
		bool loaded = false;
		if (_cfg.isMonitorLoadOnChange()) {
			loaded = monLoad();
		}
		if (! loaded && _cfg.isMonitorAutoConfig()) {
			monAutoConfig();
		}
		std::map<std::string, std::string> args;
		pekwm::hooks()->run(PEKWM_HOOK_ON_MONITOR_CHANGE, args);
	}
}

void
PekwmSys::handleSetXSETTING(const std::vector<std::string> &args,
			    enum XSettingType type)
{
	if (args.size() != 2) {
		P_TRACE("Sys XSet* expected 2 arguments, got " << args.size());
		return;
	}

	bool updated;
	switch (type) {
	case XSETTING_TYPE_INTEGER:
		updated = setXSettingInteger(args[0], args[1]);
		break;
	case XSETTING_TYPE_COLOR:
		updated = setXSettingColor(args[0], args[1]);
		break;
	case XSETTING_TYPE_STRING:
		updated = setXSettingString(args[0], args[1]);
		break;
	}
	if (updated) {
		_xsettings.updateServer();
	}
}

bool
PekwmSys::setXSettingInteger(const std::string &key, const std::string &sval)
{
	int val;
	try {
		val = std::stoi(sval);
	} catch (std::invalid_argument&) {
		return false;
	}
	_xsettings.setInt32(key, val);
	return true;
}

static uint16_t
parseHex(const char *p)
{
	char val[3] = {p[0], p[1], 0};
	return static_cast<uint16_t>(strtol(val, NULL, 16));
}

bool
PekwmSys::setXSettingColor(const std::string &key, const std::string &sval)
{
	uint16_t r, g, b, a;
	const char *p = sval.c_str();
	if (*p != '#' || (sval.size() != 7 && sval.size() != 9)) {
		return false;
	}

	// #RRGGBB(AA)
	r = parseHex(p + 1) << 8;
	g = parseHex(p + 3) << 8;
	b = parseHex(p + 5) << 8;
	if (sval.size() == 7) {
		a = UINT16_MAX;
	} else {
		a = parseHex(p + 7) << 8;
	}
	_xsettings.setColor(key, r, g, b, a);
	return true;
}

bool
PekwmSys::setXSettingString(const std::string &key, const std::string &sval)
{
	_xsettings.setString(key, sval);
	return true;
}

void
PekwmSys::handleXSave()
{
	if (_cfg.getXSettingsPath().empty()) {
		P_WARN("can not save XSETTINGS, path not set");
	} else {
		_xsettings.save(_cfg.getXSettingsPath());
	}
}

bool
PekwmSys::handleSetTimeOfDay(const std::vector<std::string> &args)
{
	if (args.size() != 1) {
		P_TRACE("Set TimeOfDay expected 1 argument, got "
			<< args.size());
		return false;
	}

	if (pekwm::ascii_ncase_equal(args[0], "AUTO")) {
		_tod_override = static_cast<TimeOfDay>(-1);
	} else if (pekwm::ascii_ncase_equal(args[0], "TOGGLE")) {
		// toggle between day and night
		if (_tod == TIME_OF_DAY_DAY) {
			_tod_override = TIME_OF_DAY_NIGHT;
		} else {
			_tod_override = TIME_OF_DAY_DAY;
		}
	} else if (pekwm::ascii_ncase_equal(args[0], "NEXT")) {
		// go to next time of day including dawn and dusk
		_tod_override = static_cast<TimeOfDay>(_tod + 1);
		if (_tod_override > TIME_OF_DAY_DUSK) {
			_tod_override = TIME_OF_DAY_NIGHT;
		}
	} else if (! time_of_day_from_string(args[0], _tod_override)) {
		return false;
	}

	_tod = timeOfDayChanged(getEffectiveTimeOfDay(_daytime.getTimeOfDay()));
	return true;
}

void
PekwmSys::handleSetDpi(const std::vector<std::string> &args)
{
	if (args.size() != 1) {
		P_TRACE("Set Dpi expected 1 argument, got " << args.size());
		return;
	}

	double dpi;
	try {
		dpi = std::stod(args[0]);
	} catch (std::invalid_argument&) {
		P_LOG("Set Dpi " << args[0] << " is not a valid number");
		return;
	}

	if (dpi > 0.0) {
		_cfg.setDpiOverride(dpi);
		_resources.setXResourceDpi();
		setDpi();
		_xsettings.updateServer();
		P_TRACE("dpi override set to " << dpi);
	} else {
		P_LOG("Set Dpi " << dpi << " must be greater than 0.0");
	}
}

void
PekwmSys::handleMonLoad(const std::vector<std::string> &args)
{
	monLoad();
}

void
PekwmSys::handleMonSave(const std::vector<std::string> &args)
{
	const std::string &path = _cfg.getMonitorsPath();
	P_TRACE("save monitor configuration to " << path);
	SysMonitorConfig config;
	config.load(path);
	SysMonitorConfig::MonitorsConfig monitors;
	if (config.mk(monitors)) {
		config.add(monitors);
		config.save(path);
	} else {
		P_TRACE("failed to create configuration monitor configuration");
	}
}

void
PekwmSys::handleTheme(const StringView &theme)
{
	std::map<std::string, std::string> x_resources;
	std::string theme_dir = Util::getDir(theme.str());

	CfgParser cfg(CfgParserOpt(""));
	if (! theme.empty()
	    && ThemeUtil::load(cfg, theme_dir, theme.str(), true)) {
		SysConfig::parseConfigXResources(cfg.getEntryRoot(),
						 x_resources, "XRESOURCES");
	}
	_cfg.setThemeXResources(x_resources);

	X11::loadXrmResources();
	_resources.setConfiguredXResources(_tod);
	X11::saveXrmResources();
}

void
PekwmSys::reload()
{
	SysConfig old_cfg(_cfg);
	if (! _cfg.parseConfig()) {
		P_WARN("failed to parse configuration");
		return;
	}

	if (old_cfg.isXSettingsEnabled() != _cfg.isXSettingsEnabled()) {
		P_TRACE("XSETTINGS changed to " << _cfg.isXSettingsEnabled());
		if (_cfg.isXSettingsEnabled()) {
			_xsettings.setServerOwner();
		} else {
			_xsettings.clearServerOwner();
		}
	}

	bool update_tod =
		old_cfg.getLatitude() != _cfg.getLatitude()
		|| old_cfg.getLongitude() != _cfg.getLongitude();
	if (! old_cfg.isLocationLookup() && _cfg.isLocationLookup()) {
		P_TRACE("location lookup changed to "
			<< _cfg.isLocationLookup());
		updateLocation();
		update_tod = true;
	}
	if (old_cfg.getTimeOfDay() != _cfg.getTimeOfDay()) {
		P_TRACE("time of day changed to " << _cfg.getTimeOfDay());
		if (pekwm::ascii_ncase_equal(_cfg.getTimeOfDay(), "AUTO")) {
			_tod_override = static_cast<TimeOfDay>(-1);
			update_tod = true;
		} else if (time_of_day_from_string(_cfg.getTimeOfDay(),
						   _tod_override)) {
			update_tod = true;
		} else {
			P_TRACE("invalid time of day " << _cfg.getTimeOfDay());
		}
	}

	if (update_tod) {
		_tod = updateDaytime(time(NULL));
	}

	TimeOfDay tod = getEffectiveTimeOfDay();
	if (old_cfg.getNetTheme() != _cfg.getNetTheme()
	    || old_cfg.getNetIconTheme() != _cfg.getNetIconTheme()) {
		P_TRACE("NetTheme/NetIconTheme changed to "
			<< _cfg.getNetTheme() << "/" << _cfg.getNetIconTheme());
		setNetTheme(tod);
		setNetIconTheme();
		_xsettings.updateServer();
	}
	if (old_cfg.getXResources(tod) != _cfg.getXResources(tod)) {
		P_TRACE("X resources for changed");
		_resources.setConfiguredXResources(tod);
	}
}

bool
PekwmSys::monLoad()
{
	const std::string &path = _cfg.getMonitorsPath();
	P_TRACE("apply monitor configuration from " << path);
	SysMonitorConfig config;
	SysMonitorConfig::MonitorsConfig monitors;
	if (config.load(path) && config.find(monitors)) {
		return config.apply(monitors);
	}
	return false;
}

bool
PekwmSys::monAutoConfig()
{
	SysMonitorConfig config;
	return config.autoConfig();
}

void
PekwmSys::updateLocation()
{
	if (! _cfg.isLocationLookup()) {
		P_TRACE("location lookup is disabled");
		return;
	}

	Location location(mkHttpClient());
	double latitude, longitude;
	P_TRACE("get location from Location service");
	if (location.get(latitude, longitude)) {
		P_TRACE("got location latitude: " << latitude
			<< " longitude: " << longitude);
		_cfg.setLatitude(latitude);
		_cfg.setLongitude(longitude);
		_resources.setLocationCountry(location.getCountry());
		_resources.setLocationCity(location.getCity());
	}
}

TimeOfDay
PekwmSys::updateDaytime(time_t now)
{
	TimeOfDay tod = TIME_OF_DAY_DAY;
	time_t next = Calendar(now).nextDay().getTimestamp();
	if (isTimeOfDayOverride()) {
		tod = _tod_override;
	} else if (_cfg.haveLocation()) {
		_daytime =
			Daytime(now, _cfg.getLatitude(), _cfg.getLongitude());
		next = std::min(next, _daytime.getTimeOfDayEnd(now));
		tod = _daytime.getTimeOfDay(now);
	}
	_timeouts.replaceTime(PEKWM_SYS_DAY_CHANGED, next);
	return tod;
}

enum TimeOfDay
PekwmSys::timeOfDayChanged(enum TimeOfDay tod)
{
	if (_tod == static_cast<TimeOfDay>(-1)) {
		P_LOG("initial time of day: " << time_of_day_to_string(tod));
	} else if (_tod != tod) {
		P_LOG("time of day changed from "
		      << time_of_day_to_string(_tod) << " to "
		      << time_of_day_to_string(tod));
	} else {
		P_TRACE("not updating time of day, not changed");
		return tod;
	}
	_resources.update(_daytime, tod);

	// automatic XSETTINGS theme switch
	if (! _cfg.getNetTheme().empty()) {
		setNetTheme(tod);
		_xsettings.updateServer();
	}

	// daytime switch commands
	OsEnv daytime_command_env;
	std::string tod_str = time_of_day_to_string(tod);
	Util::to_lower(tod_str);
	daytime_command_env.override("PEKWM_SYS_TIMEOFDAY", tod_str);
	const SysConfig::string_vector &commands = _cfg.getDaytimeCommands();
	SysConfig::string_vector::const_iterator it(commands.begin());
	for (; it != commands.end(); ++it) {
		std::vector<std::string> args = StringUtil::shell_split(*it);
		_os->processExec(args, &daytime_command_env);
	}

	return tod;
}

#ifndef UNITTEST

static void
usage(int ret)
{
	std::cout << "usage: " << progname << " [-dh]" << std::endl;
	std::cout << " -d --display    Display to connect to." << std::endl;
	std::cout << " -f --log-file   Set log file." << std::endl;
	std::cout << " -h --help       Show this info." << std::endl;
	std::cout << " -l --log-level  Set log level." << std::endl;
	std::cout << " -t --theme      Path to theme." << std::endl;
	exit(ret);
}

int
main(int argc, char *argv[])
{
	pledge_x11_required("");

	progname = argv[0];
	const char *display = NULL;
	std::string theme;
	std::string config_file = Util::getEnv("PEKWM_CONFIG_FILE");
	bool interactive = false;

	static struct option opts[] = {
		{const_cast<char*>("display"), required_argument, nullptr, 'd'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("log-level"), required_argument, nullptr,
		 'l'},
		{const_cast<char*>("log-file"), required_argument, nullptr,
		 'f'},
		{const_cast<char*>("theme"), required_argument, nullptr,
		 't'},
		{const_cast<char*>("interactive"), no_argument, nullptr, 'i'},
		{nullptr, 0, nullptr, 0}
	};

	int ch;
	while ((ch = getopt_long(argc, argv, "c:d:h:il:f:t:", opts, nullptr))
	       != -1) {
		switch (ch) {
		case 'c':
			config_file = optarg;
			break;
		case 'd':
			display = optarg;
			break;
		case 'h':
			usage(0);
			break;
		case 'f':
			if (! Debug::setLogFile(optarg)) {
				std::cerr << "Failed to open log file "
					  << optarg << std::endl;
			}
			break;
		case 'l':
			Debug::setLevel(Debug::getLevel(optarg));
			break;
		case 't':
			theme = optarg;
			break;
		case 'i':
			interactive = true;
			break;
		default:
			usage(1);
			break;
		}
	}

	if (! X11::init(display, std::cerr)) {
		return 1;
	}

	struct sigaction act;
	act.sa_handler = sigHandler;
	act.sa_mask = sigset_t();
	act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
	sigaction(SIGCHLD, &act, 0);

	if (config_file.empty()) {
		config_file = Util::getConfigDir() + "/config";
	}
	Util::expandFileName(config_file);

	Destruct<Os> os(mkOs());
	_hooks = new Hooks(*os);
	_hooks->load(config_file);
	Destruct<Hooks> hooks(_hooks);
	PekwmSys sys(config_file, interactive, *os);
	return sys.main(theme);
}

#endif // UNITTEST
