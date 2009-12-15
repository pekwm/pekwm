//
// Observable.hh for pekwm
// Copyright © 2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _OBSERVABLE_HH_
#define _OBSERVABLE_HH_

#include <list>

class Observer;

class Observable {
public:
    Observable(void) : _observers(0) { }
    virtual ~Observable(void) { delete _observers; }

    void notifyObservers(void);

    void addObserver(Observer *observer); 
    void removeObserver(Observer *observer); 

private:
    std::list<Observer*> *_observers; /**< List of observers. */
};

#endif // _OBSERVABLE_HH_
