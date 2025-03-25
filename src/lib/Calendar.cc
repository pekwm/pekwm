//
// Calendar.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Calendar.hh"
#include "Mem.hh"

extern "C" {
#include <sys/types.h>
#include <time.h>
}

static const time_t SECONDS_PER_DAY = 86400;

/**
 * Create a new calendar object representing the current time.
 */
Calendar::Calendar()
{
	time_t now = time(NULL);
	gmtime_r(&now, &_tm);
}

/**
 * Create a new calendar object representing the given UTC time.
 */
Calendar::Calendar(time_t ts, int ts_off)
{
	if (ts_off) {
		ts += ts_off * SECONDS_PER_DAY;
	}
	gmtime_r(&ts, &_tm);
}

/**
 * Create a new calendar object representing the given UTC time.
 */
Calendar::Calendar(const struct tm &tm)
	: _tm(tm)
{
}

Calendar::~Calendar()
{
}

/**
 * Get UTC timestamp (seconds since epoch).
 */
time_t
Calendar::getTimestamp() const
{
	struct tm tm = _tm;
	return timegm(&tm);
}

/**
 * Create string representation of Celander object
 */
std::string
Calendar::toString(const char *format) const
{
	if (! format) {
		format = "%Y-%m-%dT%H:%M:%SZ";
	} else if (format[0] == '\0') {
		return std::string("");
	}

	Buf<char> buf(64);
	size_t len;
	do {
		len = strftime(*buf, buf.size(), format, &_tm);
		if (len == 0) {
			buf.grow(false);
		}
	} while (len == 0);

	return std::string(*buf);
}

/**
 * Create a new calendar object, set to 00:00:00 of the day following the day
 * set in this Calendar object.
 */
Calendar
Calendar::nextDay() const
{
	struct tm tm;
	time_t ts_next = getTimestamp() + SECONDS_PER_DAY;
	gmtime_r(&ts_next, &tm);
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	return Calendar(tm);
}
