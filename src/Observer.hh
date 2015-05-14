//
// Observer.hh for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _OBSERVER_HH_
#define _OBSERVER_HH_

#include "config.h"

class Observable;
class Observation;

class Observer {
public:
    Observer(void) { }
    virtual ~Observer(void) { }

    virtual void notify(Observable *observable, Observation *observation) { }
};

#endif // _OBSERVER_HH_
