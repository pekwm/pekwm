//
// Observable.hh for pekwm
// Copyright © 2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _OBSERVABLE_HH_
#define _OBSERVABLE_HH_

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

#endif // _OBSERVABLE_HH_
