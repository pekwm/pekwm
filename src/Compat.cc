//
// Compat.cc for pekwm
// Copyright (C) 2009-2021 Claes Nästén <pekdon@gmail.com>
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
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#ifndef PEKWM_HAVE_PUT_TIME
#include <time.h>
#endif // PEKWM_HAVE_PUT_TIME
#include <unistd.h>
}

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
			std::cerr << "failed to setsid, aborting: " << strerror(errno)
				  << std::endl;
			exit(1);
		}

		if (! nochdir) {
			if (chdir("/") == -1) {
				std::cerr << "failed to change directory to /" << std::endl;
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
	assert(clk_id == CLOCK_MONOTONIC);

	struct timeval tv = {0};
	int ret = gettimeofday(&tv, nullptr);
	if (! ret) {
		tp->tv_sec = tv.tv_sec;
		tp->tv_nsec = tv.tv_usec / 100;
	}
	return ret;
}

#endif // ! PEKWM_HAVE_CLOCK_GETTIME

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
	std::string to_string(long val)
	{
		char buf[32];
		snprintf(buf, sizeof(buf), "%ld", val);
		return buf;
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


#ifndef PEKWM_HAVE_STOF

namespace std
{
	float
	stof(const std::string& str)
	{
		char *endptr;
		double val = strtod(str.c_str(), &endptr);
		if (*endptr != 0) {
			std::string msg("not a valid float: ");
			msg += str;
			throw std::invalid_argument(msg);
		}
		return static_cast<float>(val);
	}
}

#endif
