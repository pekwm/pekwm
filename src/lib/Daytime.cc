//
// Daytime.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Daytime.hh"
#include "String.hh"

extern "C" {
#include <math.h>
#include <time.h>
}

static const double seconds_per_day = 86400.0;
static const double julian_date_epoch = 2440587.5;
static const double julian_day_20010101 = 2451545.0;
static const double equation_of_the_center_deg_earth_coeff = 1.9148;
static const double earth_maximal_axial_tilt_sun =
	23.4397 / 180.0 * M_PI;

static double
_ts_to_j(double ts)
{
	return (ts / seconds_per_day) + julian_date_epoch;
}

static time_t
_j_to_ts(double julian)
{
	return (julian - julian_date_epoch) * seconds_per_day;
}

static double
_j_to_julian_day(double julian)
{
	return ceil(julian - (julian_day_20010101 + 0.0009)
		    + 69.184 / seconds_per_day);
}

static double
_to_rad(double degrees)
{
	return degrees / 180.0 * M_PI;
}

static double
_to_deg(double radians)
{
	return radians * 180.0 / M_PI;
}

/**
 * Get string representation of TimeOfDay.
 */
const char*
time_of_day_to_string(enum TimeOfDay tod)
{
	switch (tod) {
	case TIME_OF_DAY_DAWN:
		return "dawn";
	case TIME_OF_DAY_DAY:
		return "day";
	case TIME_OF_DAY_DUSK:
		return "dusk";
	case TIME_OF_DAY_NIGHT:
	default:
		return "night";
	}
}

/**
 * Get TimeOfDay from string.
 */
enum TimeOfDay
time_of_day_from_string(const std::string &str)
{
	if (pekwm::ascii_ncase_equal(str, "DAWN")) {
		return TIME_OF_DAY_DAWN;
	} else if (pekwm::ascii_ncase_equal(str, "DAY")) {
		return TIME_OF_DAY_DAY;
	} else if (pekwm::ascii_ncase_equal(str, "DUSK")) {
		return TIME_OF_DAY_DUSK;
	} else {
		return TIME_OF_DAY_NIGHT;
	}
}

/**
 * Default constructor, inits 0 daytime object
 */
Daytime::Daytime()
	: _now(0),
	  _sun_rise(0),
	  _sun_set(0),
	  _day_length_s(0)
{
}

/**
 * Create daytime object for the provided day, location and elevation.
 */
Daytime::Daytime(time_t ts, double latitude, double longitude,
		 double elevation)
	: _now(ts),
	  _sun_rise(0),
	  _sun_set(0),
	  _day_length_s(0)
{
	double julian = _ts_to_j(ts);
	double julian_day = _j_to_julian_day(julian);

	double mean_solar_time = julian_day + 0.0009 - longitude / 360.0;

	double solar_mean_anomaly_deg =
		fmod(357.5291 + 0.98560028 * mean_solar_time, 360.0);
	double solar_mean_anomaly = _to_rad(solar_mean_anomaly_deg);

	double equation_of_the_center_deg =
		equation_of_the_center_deg_earth_coeff
		* sin(solar_mean_anomaly) + 0.02
		* sin(2 * solar_mean_anomaly) + 0.0003
		* sin(3 * solar_mean_anomaly);

	double ecliptic_longitude_deg =
		fmod(solar_mean_anomaly_deg + equation_of_the_center_deg
		     + 180.0 + 102.9372, 360.0);
	double ecliptic_longitude = _to_rad(ecliptic_longitude_deg);

	double declination_of_the_sun_sin =
		sin(ecliptic_longitude) * sin(earth_maximal_axial_tilt_sun);
	double declination_of_the_sun_cos =
		cos(asin(declination_of_the_sun_sin));

	double hour_angle_cos =
		(sin(_to_rad(-0.833 - 2.076 * sqrt(elevation) / 60.0))
		 - sin(_to_rad(latitude)) * declination_of_the_sun_sin)
		/ (cos(_to_rad(latitude)) * declination_of_the_sun_cos);
	if (hour_angle_cos <= 1.0) {
		double hour_angle_deg = _to_deg(acos(hour_angle_cos));
		double solar_transit =
			julian_day_20010101
			+ mean_solar_time
			+ 0.0053 * sin(solar_mean_anomaly)
			- 0.0069 * sin(2 * ecliptic_longitude);
		_sun_rise = _j_to_ts(solar_transit - hour_angle_deg / 360.0);
		_sun_set = _j_to_ts(solar_transit + hour_angle_deg / 360.0);
		_day_length_s = _sun_set - _sun_rise;
	}
}

Daytime::~Daytime()
{
}

Daytime&
Daytime::operator=(const Daytime &rhs)
{
	_now = rhs._now;
	_sun_rise = rhs.getSunRise();
	_sun_set = rhs.getSunSet();
	_day_length_s = rhs.getDayLengthS();
	return *this;
}

/**
 * Get time of day for the given timestamp, if ts is outside of the range of
 * the given day Daytime was created for, TIME_OF_DAY_NIGHT is returned.
 */
enum TimeOfDay
Daytime::getTimeOfDay(time_t ts)
{
	if (ts == 0) {
		ts = _now;
	}
	if (ts < _sun_rise || ts > _sun_set) {
		return TIME_OF_DAY_NIGHT;
	}
	return TIME_OF_DAY_DAY;
}

/**
 * Get time of day when the current time of day state ends.
 */
time_t
Daytime::getTimeOfDayEnd(time_t ts)
{
	if (ts == 0) {
		ts = _now;
	}
	if (ts > _sun_set) {
		// next day (without change in calculation)
		return _sun_rise + int(seconds_per_day);
	}
	if (ts > _sun_rise) {
		return _sun_set;
	}
	return _sun_rise;
}
