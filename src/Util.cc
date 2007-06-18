//
// Util.cc for pekwm
// Copyright (C) 2002-2007 Claes Nästén <me{@}pekdon{.}net>
//
// misc.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <algorithm>
#include <iostream>
#include <sstream>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef HAVE_SETENV
#include <cstdio>
#include <cstring>
#include <errno.h>
#endif // HAVE_SETENV

#include "Util.hh"

using std::cerr;
using std::endl;
using std::ostringstream;
using std::string;
using std::transform;
using std::vector;
using std::wstring;

namespace Util {

//! @brief Fork and execute command with /bin/sh and execlp
void
forkExec(std::string command)
{
    if (command.length() == 0) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Util::forkExec() *** command length == 0" << endl;
#endif // DEBUG
        return;
    }

    pid_t pid = fork();
    switch (pid) {
    case 0:
        setsid();
        execlp("/bin/sh", "sh", "-c", command.c_str(), (char *) NULL);
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Util::forkExec(" << command << ") execlp failed." << endl;
        exit(1);
    case -1:
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Util::forkExec(" << command << ") fork failed." << endl;
    }
}

//! @brief Determines if the file exists
bool
isFile(const std::string &file)
{
    if (file.size() == 0) {
        return false;
    }

    struct stat stat_buf;
    if (stat(file.c_str(), &stat_buf) == 0) {
        return (S_ISREG(stat_buf.st_mode));
    }

    return false;
}

//! @brief Determines if the file is executable for the current user.
bool
isExecutable(const std::string &file)
{
    if (file.size() == 0) {
#ifdef DEBUG
        cerr << __FILE__ << "@" << __LINE__ << ": "
             << "Util::isExecutable() *** file length == 0" << endl;
#endif // DEBUG
        return false;
    }

    struct stat stat_buf;
    if (!stat(file.c_str(), &stat_buf)) {
        if (stat_buf.st_uid == getuid()) { // user readable and executable
            if ((stat_buf.st_mode&S_IRUSR) && (stat_buf.st_mode&S_IXUSR)) {
                return true;
            }
        }
        if (getgid() == stat_buf.st_gid) { // group readable and executable
            if ((stat_buf.st_mode&S_IRGRP) && (stat_buf.st_mode&S_IXGRP)) {
                return true;
            }
        }
        if ((stat_buf.st_mode&S_IROTH) && (stat_buf.st_mode&S_IXOTH)) {
            return true; // other readable and executable
        }
    }

    return false;
}

//! @brief Returns .extension of file
std::string
getFileExt(const std::string &file)
{
    string::size_type pos = file.find_last_of('.');
    if ((pos != string::npos) && (pos < file.size())) {
        return file.substr(pos + 1, file.size() - pos - 1);
    } else {
        return string("");
    }
}

//! @brief Returns dir part of file
std::string
getDir(const std::string &file)
{
    string::size_type pos = file.find_last_of('/');
    if ((pos != string::npos) && (pos < file.size())) {
        return file.substr(0, pos);
    } else {
        return string("");
    }
}

//! @brief Replaces the ~ with the complete homedir path.
void
expandFileName(std::string &file)
{
    if (file.size() > 0) {
        if (file[0] == '~') {
            file.replace(0, 1, getenv("HOME"));
        }
    }
}

//! @brief Split the string str based on separator sep and put into vals
//!
//! This splits the string str into to max_tokens parts and puts in the vector
//! vals. If max is 0 then it'll split it into as many tokens
//! as possible, max defaults to 0.
//! splitString returns the number of tokens it put into vals.
//!
//! @param str String to split
//! @param vals Vector to put split values into
//! @param sep Separators to use when splitting string
//! @param max Maximum number of elements to put into vals (optional)
//! @return Number of tokens inserted into vals
uint
splitString(const std::string &str, std::vector<std::string> &toks,
            const char *sep, uint max)
{
    if (str.size() < 1) {
        return 0;
    }

    uint n = toks.size();
    string::size_type s, e;

    s = str.find_first_not_of(" \t\n");
    for (uint i = 0; ((i < max) || (max == 0)) && (s != string::npos); ++i) {
        e = str.find_first_of(sep, s);

        if ((e != string::npos) && (i < (max - 1))) {
            toks.push_back(str.substr(s, e - s));
        } else if (s < str.size()) {
            toks.push_back(str.substr(s, str.size() - s));
            break;
        } else {
            break;
        }

        s = str.find_first_not_of(sep, e);
    }

    return (toks.size() - n);
}

//! @brief Converts string to uppercase
//! @param str Reference to string to convert
void
to_upper(std::string &str)
{
    transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::toupper);
}

//! @brief Converts string to lowercase
//! @param str Reference to string to convert
void
to_lower(std::string &str)
{
    transform(str.begin(), str.end(), str.begin(), (int(*)(int)) std::tolower);
}

//! @brief Converts string to multi byte version
//! @param str String to convert.
//! @return Returns multibyte version of string.
std::string
to_mb_str(const std::wstring &str)
{
  size_t ret, num = str.size() * 6 + 1;
  char *buf = new char[num];
  memset(buf, '\0', num);

  ret = wcstombs(buf, str.c_str(), num);
  if (ret == static_cast<size_t>(-1)) {
    cerr << " *** WARNING: failed to convert wide string to multibyte string" << endl;
  }
  string ret_str(buf);

  delete [] buf;

  return ret_str;
}

//! @brief Converts string to multi byte version
//! @param str String to convert.
//! @return Returns multibyte version of string.
std::wstring
to_wide_str(const std::string &str)
{
  size_t ret, num = str.size() + 1;
  wchar_t *buf = new wchar_t[num];
  wmemset(buf, L'\0', num);

  ret = mbstowcs(buf, str.c_str(), num);
  if (ret == static_cast<size_t>(-1)) {
    cerr << " *** WARNING: failed to multibyte string to wide string" << endl;
  }
  wstring ret_str(buf);

  delete [] buf;

  return ret_str;

}

//! @brief Converts string to UTF-8
//! @param str String to convert.
//! @return Returns UTF-8 representation of string.
std::string
to_utf8_str(const std::wstring &str)
{
  // FIXME: Use iconv in to_mb_str
  return to_mb_str(str);
}

//! @brief Converts to wide string from UTF-8
//! @param str String to convert.
//! @return Returns wide representation of string.
std::wstring
from_utf8_str(const std::string &str)
{
  // FIXME: Use iconv in from_utf8_str
  return to_wide_str(str);
}

} // end namespace Util.

//! @brief Inserts or resets the enviornment variable name
#ifndef HAVE_SETENV
int
setenv(const char *name, const char *value, int overwrite)
{
    // invalid parameters
    if ((name == NULL) || (value == NULL)) {
        return -1;
    }

    // don't owerwrite
    if ((overwrite == 0) && (getenv(name) != NULL)) {
        return 0;
    }

    char *str = new char[strlen(name) + strlen(value) + 2];
    if (str == NULL) {
        errno = ENOMEM;
        return -1;
    }

    snprintf(str, strlen(str), "%s=%s", name, value);

    return (putenv(str));
}
#endif // HAVE_SETENV
