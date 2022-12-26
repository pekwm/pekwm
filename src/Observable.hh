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

class ObserverMapping {
public:
	typedef std::map<Observable*, std::vector<Observer*> > observable_map;
	typedef observable_map::iterator observable_map_it;

	ObserverMapping(void);
	~ObserverMapping(void);

	size_t size(void) const { return _observable_map.size(); }

	void notifyObservers(Observable *observable, Observation *observation);
	void addObserver(Observable *observable, Observer *observer);
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
