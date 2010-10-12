//
// Observable.cc for pekwm
// Copyright © 2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "Observable.hh"
#include "Observer.hh"

using SLIST_NAMESPACE::slist;

/**
 * Notify all observers.
 */
void
Observable::notifyObservers(Observation *observation)
{
    if (_observers.size()) {
        slist<Observer*>::iterator it(_observers.begin());
        for (; it != _observers.end(); ++it) {
            (*it)->notify(this, observation);
        }
    }
}

/**
 * Add observer.
 */
void
Observable::addObserver(Observer *observer)
{
    _observers.push_front(observer);
}

/**
 * Remove observer from list.
 */
void
Observable::removeObserver(Observer *observer)
{
    if (_observers.size()) {
        _observers.remove(observer);
    }
}
