//
// Compat.hh for pekwm
// Copyright (C) 2009-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _COMPAT_HH_
#define _COMPAT_HH_

#include "config.h"

#include <cwchar>
#include <cstddef>

#ifndef HAVE_SWPRINTF
namespace std {
    int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
}
#endif // HAVE_SWPRINTF
using std::swprintf;

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

#endif // _COMPAT_HH_
