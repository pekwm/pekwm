//
// Os.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
// 
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_OS_HH_
#define _PEKWM_OS_HH_

#include "config.h"

#include <string>
#include <vector>

extern "C" {
#include <sys/types.h>
}

/**
 * Interface for Os functions.
 */
class Os {
public:
	Os() { }
	virtual ~Os() { }

	virtual pid_t processExec(const std::vector<std::string> &args)
	{
		return -1;
	}
	virtual bool processSignal(pid_t pid, int signum) { return false; }
	virtual bool isProcessAlive(pid_t pid) { return false; }
};

Os *mkOs();

#endif // _PEKWM_OS_HH_
