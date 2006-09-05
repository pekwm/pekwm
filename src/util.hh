//
// util.hh for pekwm
// Copyright (C) 2002 Claes Nästen
// pekdon@gmx.net
// http://pekdon.babblica.net/
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef _MISC_HH_
#define _MISC_HH_

#include <string>
#include <vector>

namespace Util {
	void forkExec(const std::string &command);
	bool isExecutable(const std::string &file);

	void expandFileName(std::string &file);
	unsigned int splitString(std::string str, std::vector<std::string> &vals,
													 const char *sep, int max_tokens = -1);
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

#endif // _MISC_HH_
