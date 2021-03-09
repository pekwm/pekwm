//
// Observable.hh for pekwm
// Copyright (C) 2009-2021 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include <vector>

/**
 * Message sent to observer.
 */
class Observation {
public:
    virtual ~Observation(void) { };
};

class Observable;

class Observer {
public:
    Observer(void) { }
    virtual ~Observer(void) { }

    virtual void notify(Observable *observable, Observation *observation) { }
};

class Observable {
public:
    Observable(void) { }
    virtual ~Observable(void) { }

    /**
     * Notify all observers.
     */
    void notifyObservers(Observation *observation) {
        for (auto it : _observers) {
            it->notify(this, observation);
        }
    }

    /**
     * Add observer.
     */
    void addObserver(Observer *observer) {
        _observers.push_back(observer);
    }

    /**
     * Remove observer from list.
     */
    void removeObserver(Observer *observer) {
        _observers.erase(std::remove(_observers.begin(), _observers.end(),
                                     observer), _observers.end());
    }

private:
    std::vector<Observer*> _observers; /**< List of observers. */
};
