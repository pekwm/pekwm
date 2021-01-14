//
// Observable.cc for pekwm
// Copyright (C) 2009-2020 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <algorithm>

#include "Observable.hh"
#include "Observer.hh"

/**
 * Notify all observers.
 */
void
Observable::notifyObservers(Observation *observation)
{
    auto it(_observers.begin());
    auto end(_observers.end());
    for (; it != end; ++it) {
        (*it)->notify(this, observation);
    }
}

/**
 * Add observer.
 */
void
Observable::addObserver(Observer *observer)
{
    _observers.push_back(observer);
}

/**
 * Remove observer from list.
 */
void
Observable::removeObserver(Observer *observer)
{
    _observers.erase(std::remove(_observers.begin(), _observers.end(), observer), _observers.end());
}
