//
// Calendar.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CALENDAR_HH_
#define _PEKWM_CALENDAR_HH_

#include <string>

#include "Compat.hh"

extern "C" {
#include <sys/types.h>
#include <time.h>
}

class Calendar {
public:
	Calendar();
	Calendar(time_t ts);
	Calendar(const struct tm &tm);
	~Calendar();

	int getYear() const { return _tm.tm_year; }
	int getMonth() const { return _tm.tm_mon + 1; }
	int getMDay() const { return _tm.tm_mday; }
	int getHour() const { return _tm.tm_hour; }
	int getMin() const { return _tm.tm_min; }
	int getSec() const { return _tm.tm_sec; }

	time_t getTimestamp() const;
	std::string toString(const char *format = nullptr) const;

	Calendar nextDay() const;

private:
	struct tm _tm;
};

#endif // _PEKWM_CALENDAR_HH_
