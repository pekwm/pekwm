//
// Debug.cc for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
// Copyright (C) 2012 Andreas Schlick <ioerror@lavabit.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "pekwm.hh"
#include "Debug.hh"
#include "Util.hh"

#include <cstdlib>
#include <ctime>

static Util::StringTo<Debug::Level> debug_level_map[] = {
	{"ERROR", Debug::LEVEL_ERR},
	{"WARNING", Debug::LEVEL_WARN},
	{"INFO", Debug::LEVEL_INFO},
	{"DEBUG", Debug::LEVEL_DEBUG},
	{"TRACE", Debug::LEVEL_TRACE},
	{nullptr, Debug::LEVEL_WARN}
};

static Debug::Level _level = Debug::LEVEL_WARN;
static bool _use_cerr = true;
std::ofstream _log("/dev/null");


/**
 * Output current timestamp to log stream.
 */
static void
addTimestamp(std::ostream &log)
{
	std::time_t t = std::time(nullptr);
	std::tm tm;
	localtime_r(&t, &tm);
	log << std::put_time(&tm, "%Y-%m-%d %H:%M:%S ");
}

namespace Debug
{

	/**
	 * Get log level from string.
	 */
	Level
	getLevel(const std::string& str)
	{
		return Util::StringToGet<Debug::Level>(debug_level_map, str);
	}

	/**
	 * Return true if current level includes level.
	 */
	bool
	isLevel(Level level)
	{
		return Debug::getLevel() >= level;
	}

	/**
	 * Get current log level.
	 */
	Level
	getLevel(void)
	{
		return _level;
	}

	/**
	 * Set log level.
	 */
	void
	setLevel(Level level)
	{
		_level = level;
	}

	std::ostream&
	getStream(const char* prefix)
	{
		if (_use_cerr) {
			addTimestamp(std::cerr);
			std::cerr << prefix;
			return std::cerr;
		} else {
			addTimestamp(_log);
			_log << prefix;
			return _log;
		}
	}

	std::ostream&
	getStream(const char* fun, int line, const char* prefix)
	{
		if (_use_cerr) {
			addTimestamp(std::cerr);
			std::cerr << fun << '@' << line << ":\n    " << prefix;
			return std::cerr;
		} else {
			addTimestamp(_log);
			_log << fun << '@' << line << ":\n    " << prefix;
			return _log;
		}
	}

	/**
	 * Set log file.
	 */
	bool
	setLogFile(const std::string& path)
	{
		_log.close();
		_log.open(path.c_str());
		_use_cerr = ! _log.good();
		return _log.good();
	}

	/**
	 * Debug Commands:
	 *
	 * logfile <filename> - set log file, use - for stderr.
	 * level [err|warn|info|debug|trace] - sets log level.
	 */
	void
	doAction(const std::string &cmd)
	{
		std::vector<std::string> args;
		if (Util::splitString(cmd, args, " \t") != 2) {
			return;
		}

		Util::to_lower(args[0]);
		if (args[0] == "logfile") {
			if (args[1] == "-") {
				_log.close();
				_use_cerr = true;
			} else {
				setLogFile(args[1]);
			}
		} else if (args[0] == "level") {
			_level = getLevel(args[1]);
		}
	}
}
