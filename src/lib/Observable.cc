//
// Observable.cc for pekwm
// Copyright (C) 2021 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "Observable.hh"
#include "Util.hh"


Observation::~Observation(void)
{
}

Observable::~Observable(void)
{
	pekwm::observerMapping()->removeObservable(this);
}

Observer::~Observer(void)
{
}

ObserverPriority::ObserverPriority(Observer *observer, int priority)
	: _observer(observer),
	  _priority(priority)
{
}

ObserverPriority::~ObserverPriority()
{
}

bool operator<(const ObserverPriority &lhs, const ObserverPriority &rhs)
{
	return lhs.getPriority() < rhs.getPriority();
}

bool operator<(const ObserverPriority &lhs, int priority)
{
	return lhs.getPriority() < priority;
}

bool operator<(int priority, const ObserverPriority &rhs)
{
	return priority < rhs.getPriority();
}

bool operator==(const ObserverPriority &lhs, Observer *observer)
{
	return lhs.getObserver() == observer;
}

ObserverMapping::ObserverMapping(void)
{
}

ObserverMapping::~ObserverMapping(void)
{
}

/**
 * Notify all observers.
 */
void
ObserverMapping::notifyObservers(Observable *observable,
				 Observation *observation)
{
	observable_map_it oit = _observable_map.find(observable);
	if (oit != _observable_map.end()) {
		observer_vector::iterator it(oit->second.begin());
		for (; it != oit->second.end(); ++it) {
			it->getObserver()->notify(observable, observation);
		}
	}
}

/**
 * Add observer in priority order.
 */
void
ObserverMapping::addObserver(Observable *observable, Observer *observer,
			     int priority)
{
	observable_map_it it = _observable_map.find(observable);
	observer_vector *observers;
	if (it == _observable_map.end()) {
		_observable_map[observable] = std::vector<ObserverPriority>();
		observers = &_observable_map[observable];
	} else {
		observers = &it->second;
	}
	observer_vector::iterator oit =
		std::lower_bound(observers->begin(), observers->end(),
				 priority);
	observers->insert(oit, ObserverPriority(observer, priority));
}

/**
 * Remove observer from list.
 */
void
ObserverMapping::removeObserver(Observable *observable, Observer *observer)
{
	observable_map_it it = _observable_map.find(observable);
	if (it == _observable_map.end()) {
		P_ERR("stale observable " << observable);
		return;
	}

	it->second.erase(std::remove(it->second.begin(),
				     it->second.end(), observer),
			 it->second.end());
	if (it->second.empty()) {
		_observable_map.erase(it);
	}
}

/**
 * Remove observable from map.
 */
void
ObserverMapping::removeObservable(Observable *observable)
{
	observable_map_it it = _observable_map.find(observable);
	if (it != _observable_map.end()) {
		_observable_map.erase(it);
	}
}
