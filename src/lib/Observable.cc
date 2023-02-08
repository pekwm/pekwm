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
		std::vector<Observer*>::iterator it = oit->second.begin();
		for (; it != oit->second.end(); ++it) {
			(*it)->notify(observable, observation);
		}
	}
}

/**
 * Add observer.
 */
void
ObserverMapping::addObserver(Observable *observable, Observer *observer)
{
	observable_map_it it = _observable_map.find(observable);
	if (it == _observable_map.end()) {
		_observable_map[observable] = std::vector<Observer*>();
		_observable_map[observable].push_back(observer);
	} else {
		it->second.push_back(observer);
	}
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
