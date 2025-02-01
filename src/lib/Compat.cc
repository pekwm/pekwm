//
// Compat.cc for pekwm
// Copyright (C) 2009-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Compat.hh"
#include "Charset.hh"
#include "Util.hh"

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>

extern "C" {
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#ifndef PEKWM_HAVE_PUT_TIME
#include <time.h>
#endif // PEKWM_HAVE_PUT_TIME
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
}

#ifndef PEKWM_HAVE_EXECVPE
int execvpe(const char *file, char *const argv[], char *const envp[])
{
	const char *c_path = getenv("PATH");
	if (! c_path) {
		c_path = "/bin:/usr/bin";
	}
	const char *begin = c_path;
	const char *end = strchr(begin, ':');
	do {
		if (end == NULL) {
			end = begin + strlen(begin);
		}
		std::string cmd_path(begin, end - begin);
		struct stat sb;
		if (! stat(cmd_path.c_str(), &sb) && S_ISREG(sb.st_mode)) {
			execve(cmd_path.c_str(), argv, envp);
			exit(1);
		}
		begin = end + 1;
	} while (*begin);

	exit(127);
}
#endif // PEKWM_HAVE_EXECVPE

#ifndef PEKWM_HAVE_SETENV
/**
 * Compat setenv, insert variable to environment.
 */
int
setenv(const char *name, const char *value, int overwrite)
{
	// Invalid parameters
	if (! name || ! value) {
		return -1;
	}
	// Do not overwrite
	if (! overwrite && getenv(name)) {
		return 0;
	}

	size_t len = strlen(name) + strlen(value) + 2;
	char *str = new char[len];
	if (! str) {
		errno = ENOMEM;
		return -1;
	}

	snprintf(str, len, "%s=%s", name, value);

	return (putenv(str));
}
#endif // ! PEKWM_HAVE_SETENV

#ifndef PEKWM_HAVE_UNSETENV
/**
 * Compat unsetenv, removes variable from the environment.
 */
int
unsetenv(const char *name) {
	const char *value = getenv(name);
	if (value && strlen(value)) {
		return setenv(name, "", 1);
	} else {
		errno = EINVAL;
		return -1;
	}
}
#endif // ! PEKWM_HAVE_UNSETENV

#ifndef PEKWM_HAVE_DAEMON
/**
 * Compat daemon.
 */
int
daemon(int nochdir, int noclose)
{
	pid_t pid = fork();
	if (pid == -1) {
		return -1;
	} else if (pid == 0) {
		pid_t session = setsid();
		if (session == -1) {
			std::cerr << "failed to setsid, aborting: "
				  << strerror(errno) << std::endl;
			exit(1);
		}

		if (! nochdir) {
			if (chdir("/") == -1) {
				std::cerr << "failed to change directory to /"
					  << std::endl;
			}
		}
		if (! noclose) {
			close(0);
			close(1);
			close(2);
		}

	} else {
		_exit(2);
	}

	return 0;
}
#endif // ! PEKWM_HAVE_DAEMON

#ifndef PEKWM_HAVE_CLOCK_GETTIME

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
	assert(clk_id == CLOCK_MONOTONIC || clk_id == CLOCK_REALTIME);

	struct timeval tv = {0};
	int ret = gettimeofday(&tv, nullptr);
	if (! ret) {
		tp->tv_sec = tv.tv_sec;
		tp->tv_nsec = tv.tv_usec / 100;
	}
	return ret;
}

#endif // ! PEKWM_HAVE_CLOCK_GETTIME

#ifndef PEKWM_HAVE_TIMEGM

static int _is_leap_year(int year)
{
	return (year % 400 == 0)
		|| (year % 4 == 0 && year % 100 != 0);
}

time_t timegm(struct tm* tm)
{
	int i, year;
	time_t val;

	val = tm->tm_sec
		+ tm->tm_min * 60
		+ tm->tm_hour * 3600
		+ tm->tm_yday * 86400;

	year = 1900 + tm->tm_year;
	for (i = 1970; i < year; i++) {
		if (_is_leap_year(i)) {
			val += 366 * 86400;
		} else {
			val += 365 * 86400;
		}
	}

	return val;
}

#endif // PEKWM_HAVE_TIMEGM

#ifndef PEKWM_HAVE_PUT_TIME

namespace std
{
	const char*
	put_time(const struct ::tm *tm, const char *fmt)
	{
		static char buf[128] = {0};
		::strftime(buf, sizeof(buf), fmt, tm);
		return buf;
	}
}

#endif

#ifndef PEKWM_HAVE_TO_STRING

namespace std
{
	template<typename T>
	std::string to_string(T val)
	{
		std::stringstream buf;
		buf << val;
		return buf.str();
	}

	std::string to_string(double val)
	{
		return to_string<double>(val);
	}

	std::string to_string(float val)
	{
		return to_string<float>(val);
	}

	std::string to_string(long long val)
	{
		return to_string<long long>(val);
	}

	std::string to_string(unsigned long long val)
	{
		return to_string<unsigned long long>(val);
	}

	std::string to_string(long val)
	{
		return to_string<long>(val);
	}

	std::string to_string(unsigned long val)
	{
		return to_string<unsigned long>(val);
	}

	std::string to_string(int val)
	{
		return to_string<int>(val);
	}

	std::string to_string(unsigned int val)
	{
		return to_string<unsigned int>(val);
	}
}
#endif

#ifndef PEKWM_HAVE_STOI

namespace std
{
	int
	stoi(const std::string& str)
	{
		char *endptr;
		long val = strtol(str.c_str(), &endptr, 10);
		if (*endptr != 0) {
			std::string msg("not a valid integer: ");
			msg += str;
			throw std::invalid_argument(msg);
		}
		return val;
	}
}

#endif

/**
 * Variant of stoi where default value is returned on parse error.
 */
int
stoi_safe(const std::string& str, int def)
{
	try {
		return std::stoi(str);
	} catch (std::invalid_argument&) {
		return def;
	}
}

#if !defined(PEKWM_HAVE_STOD) || !defined(PEKWM_HAVE_STOF)

template<typename T>
static T
_stod(const std::string& str, const std::string &name)
{
	char *endptr;
	double val = strtod(str.c_str(), &endptr);
	if (*endptr != 0) {
		std::string msg("not a valid ");
		msg += name + ": " + str;
		throw std::invalid_argument(msg);
	}
	return static_cast<T>(val);
}

#endif

#ifndef PEKWM_HAVE_STOD

namespace std
{
	double
	stod(const std::string &str)
	{
		return _stod<double>(str, "double");
	}
}

#endif

#ifndef PEKWM_HAVE_STOF

namespace std
{
	float
	stof(const std::string& str)
	{
		return _stod<float>(str, "float");
	}
}

#endif

#ifndef PEKWM_HAVE_PLEDGE
int
pledge(const char*, const char*)
{
	return 0;
}
#endif // PEKWM_HAVE_PLEDGE

void
pledge_x(const char* promises, const char *execpromises)
{
	if (pledge(promises, execpromises) == -1) {
		std::cerr << "ERROR: pledge(" << promises << ", "
			  << execpromises << ") failed: "
			  << errno << std::endl;
		exit(1);
	}
}

/**
 * Pledge, limiting access to what Xlib requires to setup connection.
 */
void
pledge_x11_required(const std::string& extra)
{
	std::string promises("stdio rpath wpath cpath inet unix dns "
			     "proc exec prot_exec");
	if (! extra.empty()) {
		promises += " ";
		promises += extra;
	}

	pledge_x(promises.c_str(), NULL);
}
