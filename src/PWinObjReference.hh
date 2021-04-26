//
// PWinObjReference.hh for pekwm
// Copyright (C) 2009-2021 Claes Nästen <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_PWINOBJREFERENCE_HH_
#define _PEKWM_PWINOBJREFERENCE_HH_

#include "config.h"

#include "PWinObj.hh"
#include "Observable.hh"

/**
 * Observer with a reference to a PWinObj.
 */
class PWinObjReference : public Observer {
public:
    PWinObjReference(PWinObj *wo_ref=nullptr)
        : _wo_ref(nullptr)
    {
        setWORef(wo_ref);
    }
    virtual ~PWinObjReference(void)
    {
        setWORef(nullptr);
    }

    /** Notify about reference update, unset the reference. */
    virtual void notify(Observable *observable, Observation *observation) {
        if (observation == &PWinObj::pwin_obj_deleted
            && observable == _wo_ref) {
            _wo_ref = nullptr;
        }
    }

    /** Returns the PWinObj reference. */
    PWinObj *getWORef(void) const { return _wo_ref; }

    /** Sets the PWinObj reference and add this as an observer. */
    virtual void setWORef(PWinObj *wo_ref) {
        if (_wo_ref != wo_ref) {
            if (_wo_ref != nullptr) {
                pekwm::observerMapping()->removeObserver(_wo_ref, this);
            }
            _wo_ref = wo_ref;
            if (_wo_ref != nullptr) {
                pekwm::observerMapping()->addObserver(_wo_ref, this);
            }
        }
    }

private:
    /** Window object reference. */
    PWinObj *_wo_ref;
};

#endif // _PEKWM_PWINOBJREFERENCE_HH_
