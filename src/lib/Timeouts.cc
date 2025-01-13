//
// Timeouts.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Timeouts.hh"

/**
 * Populate timespec with the current time + timeout_ms milliseconds added to
 * the timeout.
 */
void
timeout_ms_to_timespec(struct timespec &ts, int timeout_ms, bool gettime)
{
	if (gettime) {
		clock_gettime(CLOCK_REALTIME, &ts);
	}
	ts.tv_sec += timeout_ms / 1000;
	ts.tv_nsec += 1000000LL * (timeout_ms % 1000);
	if (ts.tv_nsec > 1000000000LL) {
		ts.tv_sec += 1;
		ts.tv_nsec -= 1000000000LL;
	}
}

/**
 * Calculate diff between ts_end and ts and store it into timeval.
 */
void
timespec_diff_to_timeval(struct timespec &ts, struct timespec &ts_end,
			 struct timeval &tv)
{
	tv.tv_sec = ts_end.tv_sec - ts.tv_sec;
	if (ts_end.tv_nsec < ts.tv_nsec) {
		tv.tv_sec--;
		tv.tv_usec = 1000000 + ((ts_end.tv_nsec - ts.tv_nsec) / 1000);
	} else {
		tv.tv_usec = (ts_end.tv_nsec - ts.tv_nsec) / 1000;
	}
}

/**
 * Add TimeoutAction to list of future actions.
 */
void
Timeouts::add(const TimeoutAction &action)
{
	ta_vector::iterator it(_actions.begin());
	for (; it != _actions.end(); ++it) {
		if (*it < action) {
			break;
		}
	}
	_actions.insert(it, action);
}

/**
 * Add action to list, trigger at the provided time.
 */
void
Timeouts::replaceTime(int key, time_t end_time)
{
	struct timespec ts;
	ts.tv_sec = end_time;
	ts.tv_nsec = 0;
	replace(TimeoutAction(key, ts));
}

/**
 * Add TimeoutAction, replacing first occurance of any action with the same
 * id as the given action.
 */
void
Timeouts::replace(const TimeoutAction &action)
{
	ta_vector::iterator it(_actions.begin());
	for (; it != _actions.end(); ++it) {
		if (it->getKey() == action.getKey()) {
			_actions.erase(it);
			break;
		}
	}
	add(action);
}

/**
 * Fill in timeval/action to run next. Returns true if action was set, else
 * false.
 */
bool
Timeouts::getNextTimeout(struct timeval **tv, TimeoutAction &action)
{
	*tv = nullptr;
	if (_actions.empty()) {
		return false;
	}

	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	struct timespec ts_end = _actions.front().getTimeEnd();
	if (ts.tv_sec > ts_end.tv_sec
	    || (ts.tv_sec == ts_end.tv_sec
		&& ts.tv_nsec > ts_end.tv_nsec)) {
		action = _actions.front();
		_actions.erase(_actions.begin());
		return true;
	}

	timespec_diff_to_timeval(ts, ts_end, _tv);
	*tv = &_tv;
	return false;
}
