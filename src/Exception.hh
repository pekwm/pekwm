//
// PDecor.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

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
