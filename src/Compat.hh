//
// Compat.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#pragma once

#include "config.h"

#ifndef HAVE_TO_STRING
#include <string>
#include <sstream>
#endif // HAVE_TO_STRING

#include <cwchar>
#include <cstddef>

#ifndef WAIT_ANY
#define WAIT_ANY -1
#endif // WAIT_ANY

#ifndef HAVE_SWPRINTF
namespace std {
    int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
}
#endif // HAVE_SWPRINTF

#ifdef HAVE_SETENV
extern "C" {
#include <stdlib.h>
}
#else // ! HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif // HAVE_SETENV

#ifdef HAVE_UNSETENV
extern "C" {
#include <stdlib.h>
}
#else // ! HAVE_UNSETENV
int unsetenv(const char *name);
#endif // HAVE_UNSETENV

#ifndef HAVE_DAEMON
int daemon(int nochdir, int noclose);
#endif // HAVE_DAEMON

#ifndef HAVE_TIMERSUB
#define timersub(a, b, result)                                                \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
    }                                                                         \
  } while (0)
#endif // HAVE_TIMERSUB

#ifndef HAVE_PUT_TIME

namespace std
{
    const char* put_time(const struct tm *tm, const char *fmt);
}

#endif // HAVE_PUT_TIME

#ifndef HAVE_TO_STRING

namespace std
{
    template<typename T>
    std::string to_string(T const& val)
    {
        std::ostringstream oss;
        oss << val;
        return oss.str();
    }
}

#endif // HAVE_TO_STRING

#ifndef HAVE_STOI

namespace std
{
    int stoi(const std::string& str);
}

#endif // HAVE_STOI


#ifndef HAVE_STOF

namespace std
{
    float stof(const std::string& str);
    float stof(const std::wstring& str);
}

#endif // HAVE_STOF
