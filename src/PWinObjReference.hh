//
// PWinObjReference.hh for pekwm
// Copyright © 2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PWIN_OBJ_REFERENCE_HH_
#define _PWIN_OBJ_REFERENCE_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PWinObj.hh"
#include "Observer.hh"

class PWinObjReference : public Observer {
public:
    PWinObjReference(PWinObj *wo_ref=0);
    virtual ~PWinObjReference(void);

    /** Notify about reference update, unset the reference. */
    virtual void notify(Observable *observable) { _wo_ref = 0; }

    /** Returns the PWinObj reference. */
    PWinObj *getWORef(void) { return _wo_ref; }
    /** Sets the PWinObj reference. */
    void setWORef(PWinObj *wo_ref);

private:
    PWinObj *_wo_ref; /**< Window object reference. */
};

#endif // _PWIN_OBJ_REFERENCE_HH_
