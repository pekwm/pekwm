//
// Daytime.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Calendar.hh"
#include "Daytime.hh"
#include "Debug.hh"
#include "String.hh"
#include "Types.hh"

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
	return static_cast<time_t>(
		(julian - julian_date_epoch) * seconds_per_day);
}

static double
_j_to_julian_day(double julian)
{
	return round(julian - (julian_day_20010101 + 0.0009)
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
bool
time_of_day_from_string(const std::string &str, enum TimeOfDay &tod)
{
	if (pekwm::ascii_ncase_equal(str, "DAWN")) {
		tod = TIME_OF_DAY_DAWN;
	} else if (pekwm::ascii_ncase_equal(str, "DAY")) {
		tod = TIME_OF_DAY_DAY;
	} else if (pekwm::ascii_ncase_equal(str, "DUSK")) {
		tod = TIME_OF_DAY_DUSK;
	} else if (pekwm::ascii_ncase_equal(str, "NIGHT")) {
		tod = TIME_OF_DAY_NIGHT;
	} else {
		return false;
	}
	return true;
}

/**
 * Default constructor, inits 0 daytime object
 */
Daytime::Daytime()
	: _valid(false),
	  _now(0),
	  _dawn(0),
	  _sun_rise(0),
	  _sun_set(0),
	  _night(0),
	  _day_length_s(0)
{
}

/**
 * Create daytime object for the provided day, location and elevation.
 */
Daytime::Daytime(time_t ts, double latitude, double longitude,
		 double elevation)
	: _valid(true),
	  _now(ts),
	  _dawn(0),
	  _sun_rise(0),
	  _sun_set(0),
	  _night(0),
	  _day_length_s(0)
{
	if (isnan(latitude) || latitude < -90.0 || latitude > 90.0
	    || isnan(longitude) || longitude < -180.0 || longitude > 180.0) {
		_valid = false;
		return;
	}

	double julian = _ts_to_j(ts);
	double julian_day = _j_to_julian_day(julian);
	double elevation_deg = -1.0 * (2.076 * sqrt(elevation) / 60.0);
	calculate(julian_day, latitude, longitude, elevation_deg,
		  _sun_rise, _sun_set);

	// NOTE: this is just for working around failed Julian day conversions
	//       issues, rounding errors.
	Calendar cal_ts(ts);
	Calendar cal_sun_rise(_sun_rise);
	Calendar cal_sun_set(_sun_set);
	if (cal_ts.getYDay() != cal_sun_rise.getYDay()) {
		if (cal_ts.getYear() > cal_sun_rise.getYear()) {
			julian_day += cal_sun_rise.getYearDays();
		} else if (cal_ts.getYear() < cal_sun_rise.getYear()) {
			julian_day -= cal_ts.getYearDays();
		}
		julian_day += static_cast<double>(
			cal_ts.getYDay() - cal_sun_rise.getYDay());
		calculate(julian_day, latitude, longitude, elevation_deg,
			  _sun_rise, _sun_set);
	}
	_day_length_s = _sun_set - _sun_rise;

	if (! calculate(julian_day, latitude, longitude, elevation - 6.0,
			_dawn, _night)) {
		// incorrectly setting dawn/night to the same as rise/set,
		// no night time.
		_dawn = _sun_rise;
		_night = _sun_set;
	}

	P_TRACE("DayTime ts: " << ts << " dawn: " << _dawn
		<< " sun_rise: " << _sun_rise << " sun_set: " << _sun_set
		<< " night: " << _night);
}

bool
Daytime::calculate(double julian_day, double latitude, double longitude,
		   double elevation_deg, time_t &sun_rise, time_t &sun_set)
{
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
		(sin(_to_rad(-0.833 + elevation_deg))
		 - sin(_to_rad(latitude)) * declination_of_the_sun_sin)
		/ (cos(_to_rad(latitude)) * declination_of_the_sun_cos);
	double hour_angle_deg = _to_deg(acos(hour_angle_cos));
	if (! isnan(hour_angle_deg)) {
		double solar_transit =
			julian_day_20010101
			+ mean_solar_time
			+ 0.0053 * sin(solar_mean_anomaly)
			- 0.0069 * sin(2 * ecliptic_longitude);
		sun_rise = _j_to_ts(solar_transit - hour_angle_deg / 360.0);
		sun_set = _j_to_ts(solar_transit + hour_angle_deg / 360.0);
		return true;
	}
	return false;
}

Daytime::~Daytime()
{
}

Daytime&
Daytime::operator=(const Daytime &rhs)
{
	_now = rhs._now;
	_dawn = rhs.getDawn();
	_sun_rise = rhs.getSunRise();
	_sun_set = rhs.getSunSet();
	_night = rhs.getNight();
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
	if (ts < _dawn) {
		return TIME_OF_DAY_NIGHT;
	} else if (ts < _sun_rise) {
		return TIME_OF_DAY_DAWN;
	} else if (ts < _sun_set) {
		return TIME_OF_DAY_DAY;
	} else if (ts < _night) {
		return TIME_OF_DAY_DUSK;
	} else {
		return TIME_OF_DAY_NIGHT;
	}
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
