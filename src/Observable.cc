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
    auto oit = _observable_map.find(observable);
    if (oit != _observable_map.end()) {
        for (auto it : oit->second) {
            it->notify(observable, observation);
        }
    }
}

/**
 * Add observer.
 */
void
ObserverMapping::addObserver(Observable *observable,
                             Observer *observer)
{
    auto it = _observable_map.find(observable);
    if (it == _observable_map.end()) {
        _observable_map[observable] = {observer};
    } else {
        it->second.push_back(observer);
    }
}

/**
 * Remove observer from list.
 */
void
ObserverMapping::removeObserver(Observable *observable,
                                Observer *observer)
{
    auto it = _observable_map.find(observable);
    if (it == _observable_map.end()) {
        ERR("stale observable " << observable);
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
    auto it = _observable_map.find(observable);
    if (it != _observable_map.end()) {
        _observable_map.erase(it);
    }
}
