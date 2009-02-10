//
// PDecor.hh for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

/**
 * Exception thrown when loading of a file/data fails.
 */
class LoadException
{
public:
    LoadException(const char *resource) : _resource(resource) { }
    virtual ~LoadException(void) { }

    /** Get resource string. */
    const char *getResource(void) const { return _resource; }

private:
    const char *_resource; /**< Resource that failed to load. */
};

#endif // _EXCEPTION_H_
