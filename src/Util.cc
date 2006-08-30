//
// Util.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// misc.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "Util.hh"
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

//! @fn    void forkExec(const string &command)
//! @brief
void
forkExec(string command) // we fork, never know if it's destroyed
{
	if(!command.length())
		return;

	pid_t pid = fork();
	switch (pid) {
	case 0:
		setsid();
		execlp("/bin/sh", "sh", "-c", command.c_str(), (char *) NULL);
		cerr << "exec failed, cleaning up child" << endl;
		exit(1);
	case -1:
		cerr << "can't fork" << endl;
	}
}

//! @fn    bool isExecutable(const string &file)
//! @brief Determines if the file is executable for the current user.
bool
isExecutable(const string &file)
{
	if (!file.size())
		return false;

	struct stat stat_buf;
	if (!stat(file.c_str(), &stat_buf)) {
		if (stat_buf.st_uid == getuid()) { // user readable and executable
			if ((stat_buf.st_mode&S_IRUSR) && (stat_buf.st_mode&S_IXUSR))
				return true;
		}
		if (getgid() == stat_buf.st_gid) { // group readable and executable
			if ((stat_buf.st_mode&S_IRGRP) && (stat_buf.st_mode&S_IXGRP))
				return true;
		}
		if ((stat_buf.st_mode&S_IROTH) && (stat_buf.st_mode&S_IXOTH)) {
			return true; // other readable and executable
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

//! @fn    unsigned int splitString(const string& str, vector<string> &vals, const char *sep, unsigned int max_tokens)
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
splitString(const string& str, vector<string>& vals,
						const char *sep, unsigned int max_tokens)
{
	if (!str.length())
		return 0;

	// eat leading whitespaces new lines and tabs
	string::size_type start = str.find_first_not_of(" \n\t");
	if (start == string::npos)
		return 0; // nothing useful on the line

	string::size_type stop;
	unsigned int tokens = 0;

	for (unsigned int i = 0; (max_tokens == 0) || (i < max_tokens); ++i) {
		if (max_tokens && ((i + 1) == max_tokens)) {
			stop = str.size();
		} else {
			stop = str.find_first_of(sep, start);
		}

		if (stop != string::npos) {
			if (stop > start) { // make sure we have a token
				vals.push_back(str.substr(start, stop - start));
				++tokens;
			}
		} else if (start < str.size()) {
			vals.push_back(str.substr(start, str.size() - start));
			++tokens;
			break;
		} else {
			// nothing more to take care of
			break;
		}

		start = str.find_first_not_of(sep, stop);
	}

	return tokens;
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
