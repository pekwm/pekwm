//
// Observable.cc for pekwm
// Copyright © 2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <algorithm>

#include "Observable.hh"
#include "Observer.hh"

using std::vector;

/**
 * Notify all observers.
 */
void
Observable::notifyObservers(Observation *observation)
{
    vector<Observer*>::const_iterator it(_observers.begin());
    vector<Observer*>::const_iterator end(_observers.end());
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
