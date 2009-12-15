//
// PWinObjReference.cc for pekwm
// Copyright © 2009 Claes Nästen <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "PWinObjReference.hh"

/**
 * Construct new window reference.
 */
PWinObjReference::PWinObjReference(PWinObj *wo_ref)
    : _wo_ref(0)
{
    setWORef(wo_ref);
}

/**
 * Destruct refernce, remove observer.
 */
PWinObjReference::~PWinObjReference(void)
{
    setWORef(0);
}

/**
 * Set the window reference and update observer.
 */
void
PWinObjReference::setWORef(PWinObj *wo_ref)
{
    if (_wo_ref != 0) {
        _wo_ref->removeObserver(this);
    }

    _wo_ref = wo_ref;

    if (_wo_ref != 0) {
        _wo_ref->addObserver(this);
    }
}
