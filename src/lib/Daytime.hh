//
// Daytime.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_DAYTIME_HH_
#define _PEKWM_DAYTIME_HH_

#include <string>
extern "C" {
#include <time.h>
}

enum TimeOfDay {
	TIME_OF_DAY_NIGHT,
	TIME_OF_DAY_DAWN,
	TIME_OF_DAY_DAY,
	TIME_OF_DAY_DUSK
};

const char *time_of_day_to_string(enum TimeOfDay tod);
enum TimeOfDay time_of_day_from_string(const std::string &str);

/**
 * Class used to calculate sun rise and set times based on the given
 * coordinates and elevation.
 *
 * Based on the sun rise equation on Wikipedia.
 */
class Daytime {
public:
	Daytime();
	Daytime(time_t now, double latitude, double longitude,
		double elevation=0.0);
	~Daytime();

	Daytime &operator=(const Daytime &rhs);

	bool isValid() const { return _valid; }
	time_t getDawn() const { return _dawn; }
	time_t getSunRise() const { return _sun_rise; }
	time_t getSunSet() const { return _sun_set; }
	time_t getNight() const { return _night; }
	int getDayLengthS() const { return _day_length_s; }

	enum TimeOfDay getTimeOfDay(time_t ts = 0);
	time_t getTimeOfDayEnd(time_t ts = 0);

private:
	bool calculate(double julian_day, double latitude, double longitude,
		       double elevation, time_t &sun_rise, time_t &sun_set);

	bool _valid;
	time_t _now;
	time_t _dawn;
	time_t _sun_rise;
	time_t _sun_set;
	time_t _night;
	int _day_length_s;
};

#endif // _PEKWM_DAYTIME_HH_
