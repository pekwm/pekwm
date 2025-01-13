#ifndef _PEKWM_TIMEOUTS_HH_
#define _PEKWM_TIMEOUTS_HH_

#include "Compat.hh"

#include <vector>

extern "C" {
#include <sys/select.h>
#include <string.h>
#include <time.h>
}

void timeout_ms_to_timespec(struct timespec &ts, int timeout_ms,
			    bool gettime=true);
void timespec_diff_to_timeval(struct timespec &ts, struct timespec &ts_end,
			      struct timeval &tv);

/**
 * Action to perform when timeout has passed.
 */
class TimeoutAction {
public:
	TimeoutAction()
		: _key(-1)
	{
		memset(&_ts, '\xff', sizeof(_ts));
	}

	TimeoutAction(int key, int timeout_ms)
		:  _key(key)
	{
		timeout_ms_to_timespec(_ts, timeout_ms);
	}

	TimeoutAction(int key, struct timespec ts)
		:  _ts(ts),
		   _key(key)
	{
	}


	virtual ~TimeoutAction() { }

	bool operator<(const TimeoutAction &rhs)
	{
		if (_ts.tv_sec == rhs.getTimeEnd().tv_sec) {
			return _ts.tv_nsec < rhs.getTimeEnd().tv_nsec;
		} else if (_ts.tv_sec < rhs.getTimeEnd().tv_sec) {
			return true;
		} else {
			return false;
		}
	}

	const struct timespec &getTimeEnd() const { return _ts; }
	int getKey() const { return _key; }

private:
	struct timespec _ts;
	int _key;
};

/**
 * Timeout manager, tracks TimeoutAction instances in order of timeout and
 * provides timeval for select timeouts.
 */
class Timeouts {
public:
	typedef std::vector<TimeoutAction> ta_vector;

	void add(const TimeoutAction &action);
	void replaceTime(int key, time_t end_time);
	void replace(const TimeoutAction &action);

	bool getNextTimeout(struct timeval **tv, TimeoutAction &action);

private:
	struct timeval _tv;
	ta_vector _actions;
};

#endif // _PEKWM_TIMEOUTS_HH_
