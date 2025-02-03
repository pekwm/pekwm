//
// pekwm_sys.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Calendar.hh"
#include "Compat.hh"
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

extern "C" {
#include <getopt.h>
}

enum PekwmSysAction {
	PEKWM_SYS_DAY_CHANGED
};

static const char *progname = nullptr;

class PekwmSys {
public:
	PekwmSys(Os *os);
	~PekwmSys();

	int main();

private:
	void handleXEvent(XEvent &ev);
	void handleStdin();
	void handleSetXSETTING(const std::vector<std::string> &args);
	void handleSetTimeOfDay(const std::vector<std::string> &args);

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

	Timeouts _timeouts;
	OsSelect *_select;
	SysConfig _cfg;
	SysResources _resources;
	XSettings _xsettings;
	Daytime _daytime;
	TimeOfDay _tod;
	TimeOfDay _tod_override;
};

PekwmSys::PekwmSys(Os *os)
	: _select(mkOsSelect()),
	  _cfg(os),
	  _resources(_cfg),
	  _tod_override(static_cast<TimeOfDay>(-1))
{
}

PekwmSys::~PekwmSys()
{
	delete _select;
}

int
PekwmSys::main()
{
	if (! _cfg.parseConfig()) {
		std::cerr << "failed to parse configuration" << std::endl;
		return 1;
	}

	// get location in order to have correct theme and X resources setup.
	updateLocation();

	// init time of day
	_daytime = updateDaytime(time(NULL));
	timeOfDayChanged(isTimeOfDayOverride()
			 ? _tod_override : _daytime.getTimeOfDay());

	// run after first time of day change to get properties initial values
	// set.
	if (_cfg.isXSettingsEnabled()) {
		_xsettings.setOwner();
	}

	_select->add(STDIN_FILENO, OsSelect::OS_SELECT_READ);
	_select->add(X11::getFd(), OsSelect::OS_SELECT_READ);

	bool stop = false;
	do {
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
	} while (! stop);

	return 0;
}

void
PekwmSys::handleXEvent(XEvent &ev)
{
	switch (ev.type) {
	default:
		P_TRACE("got X11 event " << X11::getEventTypeString(ev.type));
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

	if (pekwm::ascii_ncase_equal(command, "TIMEOFDAY")) {
		handleSetTimeOfDay(args);
	} else if (pekwm::ascii_ncase_equal(command, "XSET")) {
		handleSetXSETTING(args);
	} else {
		// unknown command
		P_DBG("unknown command: " << command);
	}
}

void
PekwmSys::handleSetXSETTING(const std::vector<std::string> &args)
{
	if (args.size() != 3) {
		return;
	}
	_xsettings.setString(args[1], args[2]);
	_xsettings.updateServer();
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
	P_LOG("time of day changed to " << time_of_day_to_string(tod));
	_resources.update(_daytime, tod);

	// automatic XSETTINGS theme switch
	if (! _cfg.getNetTheme().empty()) {
		_xsettings.setString("Net/ThemeName",
				     _cfg.getNetTheme() + themeSuffix(tod));
		_xsettings.updateServer();
	}

	// daytime switch commands
	const SysConfig::string_vector &commands = _cfg.getDaytimeCommands();
	SysConfig::string_vector::const_iterator it(commands.begin());
	for (; it != commands.end(); ++it) {
		Util::forkExec(*it);
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

	static struct option opts[] = {
		{const_cast<char*>("display"), required_argument, nullptr, 'd'},
		{const_cast<char*>("help"), no_argument, nullptr, 'h'},
		{const_cast<char*>("log-level"), required_argument, nullptr,
		 'l'},
		{const_cast<char*>("log-file"), required_argument, nullptr,
		 'f'},
		{nullptr, 0, nullptr, 0}
	};

	int ch;
	while ((ch = getopt_long(argc, argv, "d:h:l:f:", opts, nullptr))
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
		default:
			usage(1);
			break;
		}
	}

	if (! X11::init(display, std::cerr)) {
		return 1;
	}

	Destruct<Os> os(mkOs());
	PekwmSys sys(*os);
	return sys.main();
}
