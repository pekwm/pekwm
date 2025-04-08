//
// pekwm_sys.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Calendar.hh"
#include "Compat.hh"
#include "CfgParser.hh"
#include "Daytime.hh"
#include "Debug.hh"
#include "String.hh"
#include "SysConfig.hh"
#include "SysResources.hh"
#include "Timeouts.hh"
#include "Location.hh"
#include "Mem.hh"
#include "X11.hh"
#include "XSettings.hh"

#include "../tk/ThemeUtil.hh"

extern "C" {
#include <getopt.h>
#include <signal.h>
}

enum PekwmSysAction {
	PEKWM_SYS_DAY_CHANGED
};

static const char *progname = nullptr;

extern "C" {

static bool _is_sigchld = false;

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

/** static pekwm resources, accessed via the pekwm namespace. */
static std::string _config_script_path;

namespace pekwm
{
	const std::string& configScriptPath()
	{
		return _config_script_path;
	}
}

class PekwmSys {
public:
	PekwmSys(Os *os);
	~PekwmSys();

	int main(const std::string &theme);

private:
	void handleSigchld();
	void handleXEvent(XEvent &ev);
	void handleStdin();
	void handleSetXSETTING(const std::vector<std::string> &args);
	void handleXSave();
	void handleSetTimeOfDay(const std::vector<std::string> &args);
	void handleTheme(const StringView &theme);

	void reload();

	TimeOfDay getEffectiveTimeOfDay() const
	{
		return isTimeOfDayOverride() ? _tod_override : _tod;
	}
	bool isTimeOfDayOverride() const
	{
		return _tod_override != static_cast<TimeOfDay>(-1);
	}

	void updateLocation();
	Daytime updateDaytime(time_t now);
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

	bool _stop;
	Timeouts _timeouts;
	Os *_os;
	OsSelect *_select;
	SysConfig _cfg;
	SysResources _resources;
	XSettings _xsettings;
	Daytime _daytime;
	TimeOfDay _tod;
	TimeOfDay _tod_override;
};

PekwmSys::PekwmSys(Os *os)
	: _stop(false),
	  _os(os),
	  _select(mkOsSelect()),
	  _cfg(os),
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
		_tod_override = time_of_day_from_string(_cfg.getTimeOfDay());
	}

	// load theme before time of day detection, to get the right resources
	// set the first time around
	handleTheme(theme);

	// get location in order to have correct theme and X resources setup.
	updateLocation();

	// init time of day
	_daytime = updateDaytime(time(NULL));
	_tod = timeOfDayChanged(isTimeOfDayOverride()
				? _tod_override : _daytime.getTimeOfDay());

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
			_xsettings.updateServer();
		}
	}

	_select->add(STDIN_FILENO, OsSelect::OS_SELECT_READ);
	_select->add(X11::getFd(), OsSelect::OS_SELECT_READ);

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
			_daytime = updateDaytime(time(NULL));
			_tod = timeOfDayChanged(isTimeOfDayOverride()
						? _tod_override : _daytime.getTimeOfDay());
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
	uint32_t len;
	std::cin.read(reinterpret_cast<char*>(&len), sizeof(len));

	Buf<char> buf(len + 1);
	std::cin.read(*buf, len);
	(*buf)[len] = '\0';

	std::vector<std::string> args = StringUtil::shell_split(*buf);
	std::string command = args[0];
	args.erase(args.begin());

	if (pekwm::ascii_ncase_equal(command, "EXIT")) {
		_stop = true;
	} else if (pekwm::ascii_ncase_equal(command, "RELOAD")) {
		reload();
	} else if (pekwm::ascii_ncase_equal(command, "THEME")) {
		handleTheme(StringView(*buf, 0, 6));
		_resources.notifyXTerms();
	} else if (pekwm::ascii_ncase_equal(command, "TIMEOFDAY")) {
		handleSetTimeOfDay(args);
	} else if (pekwm::ascii_ncase_equal(command, "XSET")) {
		handleSetXSETTING(args);
	} else if (pekwm::ascii_ncase_equal(command, "XSAVE")) {
		handleXSave();
	} else {
		// unknown command
		P_DBG("unknown command: " << command);
	}
}

void
PekwmSys::handleSetXSETTING(const std::vector<std::string> &args)
{
	if (args.size() != 2) {
		return;
	}
	_xsettings.setString(args[0], args[1]);
	_xsettings.updateServer();
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

void
PekwmSys::handleSetTimeOfDay(const std::vector<std::string> &args)
{
	if (args.size() != 1) {
		return;
	}

	if (pekwm::ascii_ncase_equal(args[1], "AUTO")) {
		_tod_override = static_cast<TimeOfDay>(-1);
	} else if (pekwm::ascii_ncase_equal(args[1], "TOGGLE")) {
		// toggle between day and night
		if (_tod == TIME_OF_DAY_DAY) {
			_tod_override = TIME_OF_DAY_NIGHT;
		} else {
			_tod_override = TIME_OF_DAY_DAY;
		}
	} else if (pekwm::ascii_ncase_equal(args[1], "NEXT")) {
		// go to next time of day including dawn and dusk
		_tod_override = static_cast<TimeOfDay>(_tod + 1);
		if (_tod_override > TIME_OF_DAY_DUSK) {
			_tod_override = TIME_OF_DAY_NIGHT;
		}
	} else {
		_tod_override = time_of_day_from_string(args[1]);
	}

	_tod = timeOfDayChanged(isTimeOfDayOverride()
				? _tod_override : _daytime.getTimeOfDay());
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
		} else {
			_tod_override =
				time_of_day_from_string(_cfg.getTimeOfDay());
		}
		update_tod = true;
	}

	if (update_tod) {
		_daytime = updateDaytime(time(NULL));
		_tod = _daytime.getTimeOfDay();
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

Daytime
PekwmSys::updateDaytime(time_t now)
{
	Daytime daytime(now, _cfg.getLatitude(), _cfg.getLongitude());
	time_t next = std::min(Calendar(now).nextDay().getTimestamp(),
			       daytime.getTimeOfDayEnd(now));
	_timeouts.replaceTime(PEKWM_SYS_DAY_CHANGED, next);
	return daytime;
}

enum TimeOfDay
PekwmSys::timeOfDayChanged(enum TimeOfDay tod)
{
	if (_tod == static_cast<TimeOfDay>(-1)) {
		P_LOG("initial time of day: " << time_of_day_to_string(tod));
	} else {
		P_LOG("time of day changed from "
		      << time_of_day_to_string(_tod) << " to "
		      << time_of_day_to_string(tod));
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

static void
usage(int ret)
{
	std::cout << "usage: " << progname << " [-dh]" << std::endl;
	std::cout << " -f --log-file       Set log file." << std::endl;
	std::cout << " -l --log-level      Set log level." << std::endl;
	exit(ret);
}

int
main(int argc, char *argv[])
{
	pledge_x11_required("");

	progname = argv[0];
	const char *display = NULL;
	std::string theme;

	static struct option opts[] = {
		{const_cast<char*>("display"), required_argument, nullptr, 'd'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("log-level"), required_argument, nullptr,
		 'l'},
		{const_cast<char*>("log-file"), required_argument, nullptr,
		 'f'},
		{const_cast<char*>("theme"), required_argument, nullptr,
		 't'},
		{nullptr, 0, nullptr, 0}
	};

	int ch;
	while ((ch = getopt_long(argc, argv, "d:h:l:f:t:", opts, nullptr))
	       != -1) {
		switch (ch) {
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

	Destruct<Os> os(mkOs());
	PekwmSys sys(*os);
	return sys.main(theme);
}
