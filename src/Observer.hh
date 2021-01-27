//
// Observer.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

class Observable;
class Observation;

class Observer {
public:
    Observer(void) { }
    virtual ~Observer(void) { }

    virtual void notify(Observable *observable, Observation *observation) = 0;
};
