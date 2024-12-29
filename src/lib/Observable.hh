//
// Observable.hh for pekwm
// Copyright (C) 2009-2022 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_OBSERVABLE_HH_
#define _PEKWM_OBSERVABLE_HH_

#include <cstdlib>
#include <map>
#include <vector>

/**
 * Message sent to observer.
 */
class Observation {
public:
	virtual ~Observation(void);
};

/**
 * Observable object.
 */
class Observable {
public:
	virtual ~Observable(void);
};

/**
 * Object observing Observables.
 */
class Observer {
public:
	virtual ~Observer(void);
	virtual void notify(Observable*, Observation*) = 0;
};

/**
 * Observer with priority.
 */
class ObserverPriority {
public:
	ObserverPriority(Observer *observer, int priority);
	~ObserverPriority();

	Observer *getObserver() const { return _observer; }
	int getPriority() const { return _priority; }

private:
	Observer *_observer;
	int _priority;
};

bool operator<(const ObserverPriority &lhs, const ObserverPriority &rhs);
bool operator<(const ObserverPriority &lhs, int priority);
bool operator<(int priority, const ObserverPriority &rhs);
bool operator==(const ObserverPriority &lhs, Observer *observer);

/**
 * Mapping from Observable to Observers, sorted in priority order.
 */
class ObserverMapping {
public:
	typedef std::vector<ObserverPriority> observer_vector;
	typedef std::map<Observable*, observer_vector> observable_map;
	typedef observable_map::iterator observable_map_it;

	ObserverMapping(void);
	~ObserverMapping(void);

	size_t size(void) const { return _observable_map.size(); }

	void notifyObservers(Observable *observable, Observation *observation);
	void addObserver(Observable *observable, Observer *observer,
			 int priority);
	void removeObserver(Observable *observable, Observer *observer);

	void removeObservable(Observable *observable);

private:
	/** Map from Observable to list of observers. */
	observable_map _observable_map;
};

namespace pekwm
{
	ObserverMapping* observerMapping(void);
}

#endif // _PEKWM_OBSERVABLE_HH_
