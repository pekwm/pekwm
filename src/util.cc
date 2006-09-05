//
// util.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// misc.cc for aewm++
// Copyright (C) 2000 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
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

#include "util.hh"
#include <iostream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef NEED_SETENV
#include <stdio.h>
#include <string.h>
#endif // NEED_SETENV

using std::cerr;
using std::endl;
using std::string;
using std::vector;

namespace Util {

void
forkExec(const string &command)
{
	if(!command.length())
		return;

	pid_t pid = fork();
	switch (pid) {
	case 0:
		setsid();
		execlp("/bin/sh", "sh", "-c", command.c_str(), NULL);
		cerr << "exec failed, cleaning up child" << endl;
		exit(1);
	case -1:
		cerr << "can't fork" << endl;
	}
}

bool
isExecutable(const string &file)
{
	if (!file.size())
		return false;

	struct stat stat_buf;
	if (!stat(file.c_str(), &stat_buf)) {
		// found the file, lets see if it's executable and readable
		if (stat_buf.st_uid == getuid()) {
			if ((stat_buf.st_mode&S_IRUSR) && (stat_buf.st_mode&S_IXUSR))
				return true;
		}

		if (getgid() == stat_buf.st_gid) {
			if ((stat_buf.st_mode&S_IRGRP) && (stat_buf.st_mode&S_IXGRP))
				return true;
		}

		if ((stat_buf.st_mode&S_IROTH) && (stat_buf.st_mode&S_IXOTH)) {
			return true;
		}
	}

	return false;
}

//! @fn void expandFileName(string &file)
//! @brief Replaces the ~ with the complete homedir path.
void
expandFileName(string &file)
{
	if (!file.size())
		return;

	if (file[0] == '~')
		file.replace(0, 1, getenv("HOME"));
}

//! @fn    unsigned int splitString(string str, vector<string> &vals, const char *sep, int max_tokens)
//! @brief Split the string str based on separator sep and put into vals
//!
//! This splits the string str into to max_tokens parts and puts in the vector
//! vals. If max_tokens is less than 0 then it'll split it into as many tokens
//! as possible, max_tokens defaults to -1.
//! splitString returns the number of tokens it put into vals.
//!
//! @param str String to split
//! @param vals Vector to put split values into
//! @param sep Separators to use when splitting string
//! @param max_tokens Maximum number of elements to put into vals (optional)
//! @return Number of tokens inserted into vals
unsigned int
splitString(string str, vector<string> &vals, const char *sep, int max_tokens)
{
	if (!str.length() || !max_tokens)
		return 0;

	bool max = true;
	unsigned int contains = 0;

	// eat leading whitespaces new lines and tabs
	string::size_type start = str.find_first_not_of(" \n\t");
	if (start == string::npos)
		return contains; // nothing useful on the line

	string::size_type stop;

	if (max_tokens < 0) {
		max_tokens = 1;
		max = false;
	}

	for (int i = 0; i < max_tokens;) {
		if (max && ((i + 1) == max_tokens)) {
			stop = str.size();
		} else {
			stop = str.find_first_of(sep, start);
		}

		if (stop != string::npos) {
			vals.push_back(str.substr(start, stop - start));
			++contains;
		} else if (start < str.size()) {
			vals.push_back(str.substr(start, str.size() - start));
			++contains;
			break;
		} else {
			// nothing more to take care of
			break;
		}

		start = str.find_first_not_of(sep, stop);

		if (max) {
			++i;
		}
	}

	return contains;
}

}; // end namespace Misc


// Author: Wietse Venema, Eindhoven University of Technology, The Netherlands.
// Modified for pekwm by Claes Nästen
//
// setenv - update or insert environment (name,value) pair
#ifdef NEED_SETENV
int setenv(char *name, char *value, int clobber)
{
	if (!name || !value)
		return 1; // we don't want to null variables do we?

	if (!clobber && getenv(name))
		return 0;

	char *cp = new char[strlen(name) + strlen(value) + 2];
	if (!cp)
		return 1;

	snprintf(cp, strlen(cp), "%s=%s", name, value);
	return (putenv(cp));
}
#endif // NEED_SETENV
