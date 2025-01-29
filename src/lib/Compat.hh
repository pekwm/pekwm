//
// Compat.hh for pekwm
// Copyright (C) 2009-2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_COMPAT_HH_
#define _PEKWM_COMPAT_HH_

#include "config.h"

#if __cplusplus <= 199711L

#ifndef nullptr
#define nullptr 0
#endif // !nullptr

#include <stdexcept>
#include <string>
#include <sstream>

#endif

#include <cstddef>
extern "C" {
#include <sys/wait.h>
#include <time.h>
}

#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif // WAIT_ANY

#ifndef PEKWM_HAVE_EXECVPE
int execvpe(const char *file, char *const argv[], char *const envp[]);
#endif // PEKWM_HAVE_EXECVPE

#ifdef PEKWM_HAVE_SETENV
extern "C" {
#include <stdlib.h>
}
#else // ! PEKWM_HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif // PEKWM_HAVE_SETENV

#ifdef PEKWM_HAVE_UNSETENV
extern "C" {
#include <stdlib.h>
}
#else // ! PEKWM_HAVE_UNSETENV
int unsetenv(const char *name);
#endif // PEKWM_HAVE_UNSETENV

#ifndef PEKWM_HAVE_DAEMON
int daemon(int nochdir, int noclose);
#endif // PEKWM_HAVE_DAEMON

#if !defined(PEKWM_HAVE_CLOCK_GETTIME) || !defined(PEKWM_HAVE_TIMEGM)
extern "C" {
#include <sys/time.h>
#include <time.h>
}
#endif

#ifndef PEKWM_HAVE_CLOCK_GETTIME
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 0
#endif // !CLOCK_MONOTONIC
typedef int clockid_t;

int clock_gettime(clockid_t clk_id, struct timespec *tp);
#endif // PEKWM_HAVE_CLOCK_GETTIME

#ifndef PEKWM_HAVE_TIMEGM
time_t timegm(struct tm *tm);
#endif // PEKWM_HAVE_TIMEGM

#ifndef PEKWM_HAVE_TIMERSUB
#define timersub(a, b, result)						\
	do {								\
		(result)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
		(result)->tv_usec = (a)->tv_usec - (b)->tv_usec;	\
		if ((result)->tv_usec < 0) {				\
			--(result)->tv_sec;				\
			(result)->tv_usec += 1000000;			\
		}							\
	} while (0)
#endif // PEKWM_HAVE_TIMERSUB

#ifndef PEKWM_HAVE_PUT_TIME
namespace std
{
	const char* put_time(const struct ::tm *tm, const char *fmt);
}
#endif

#ifndef PEKWM_HAVE_TO_STRING
#include <string>

namespace std
{
	std::string to_string(long val);
}
#endif

#ifndef PEKWM_HAVE_STOD
namespace std
{
	double stod(const std::string& str);
}
#endif

#ifndef PEKWM_HAVE_STOI
namespace std
{
	int stoi(const std::string& str);
}
#endif

int stoi_safe(const std::string& str, int def);

#ifndef PEKWM_HAVE_STOF
namespace std
{
	float stof(const std::string& str);
}
#endif

#ifndef PEKWM_HAVE_LIMITS

#ifdef PEKWM_HAVE_SYS_LIMITS_H
#include <sys/limits.h>
#else // ! PEKWM_HAVE_SYS_LIMITS_H
#include <limits.h>
#endif // PEKWM_HAVE_SYS_LIMITS_H

namespace std
{
	template<typename T>
	struct numeric_limits {
		static T min(void);
		static T max(void);
	};

	struct numeric_limits<int> {
		static int min(void) { return INT_MIN; }
		static int max(void) { return INT_MAX; }
	};

	struct numeric_limits<unsigned int> {
		static unsigned int min(void) { return 0; }
		static unsigned int max(void) { return UINT_MAX; }
	};
}

#endif

#ifdef PEKWM_HAVE_PLEDGE
#include <unistd.h>
#else // ! PEKWM_HAVE_PLEDGE
int pledge(const char *promises, const char *execpromises);
#endif // PEKWM_HAVE_PLEDGE
void pledge_x(const char *promises, const char *execpromises);
void pledge_x11_required(const std::string& extra);

#endif // _PEKWM_COMPAT_HH_
