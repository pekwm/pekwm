//
// Util.hh for pekwm
// Copyright (C) 2002-2003 Claes Nästen <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _UTIL_HH_
#define _UTIL_HH_

#include <string>
#include <vector>

namespace Util {
	void forkExec(std::string command); // we fork, never know if it's destroyed
	bool isExecutable(const std::string &file);

	void expandFileName(std::string &file);
	unsigned int splitString(const std::string& str,
													 std::vector<std::string> &vals,
													 const char *sep,
													 unsigned int max_tokens = 0);
	inline void trimLeadingBlanks(std::string &trim) {
		std::string::size_type first = trim.find_first_not_of(" \n\t");
		if ((first != std::string::npos) &&
				(first != (std::string::size_type) trim[0]))
			trim = trim.substr(first, trim.size() - first);
	}

	inline bool isTrue(const std::string &value) {
		if (! strncasecmp(value.c_str(), "true", strlen("true")))
			return true;
		return false;
	}
};

#ifdef NEED_SETENV
int setenv(char *name, char *value, int clobber);
#endif // NEED_SETENV

#endif // _UTIL_HH_
