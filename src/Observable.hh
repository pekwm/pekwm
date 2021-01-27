//
// Observable.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include <vector>

class Observer;

/**
 * Message sent to observer.
 */
class Observation {
public:
    virtual ~Observation(void) { };
};

class Observable {
public:
    Observable(void) { }
    virtual ~Observable(void) { }

    void notifyObservers(Observation *observation);

    void addObserver(Observer *observer);
    void removeObserver(Observer *observer);

private:
    std::vector<Observer*> _observers; /**< List of observers. */
};
