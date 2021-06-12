//
// PDecor.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_EXCEPTION_HH_
#define _PEKWM_EXCEPTION_HH_

#include "config.h"

/**
 * Base class for all pekwm exceptions.
 */
class PekwmException
{
};

/**
 * Exception thrown to stop execution.
 */
class StopException : public PekwmException
{
public:
	StopException(const char *msg)
		: _msg(msg)
	{
	}

	const char* getMsg(void) const { return _msg; }

private:
	const char *_msg;
};

/**
 * Exception thrown when loading of a file/data fails.
 */
class LoadException : public PekwmException
{
public:
	LoadException(const std::string &resource)
		: _resource(resource)
	{
	}

	/** Get resource string. */
	const std::string& getResource(void) const { return _resource; }

private:
	const std::string _resource; /**< Resource that failed to load. */
};

/**
 * Invalid value provided.
 */
class ValueException : public PekwmException
{
};

#endif // _PEKWM_EXCEPTION_HH_
