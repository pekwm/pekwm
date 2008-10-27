//
// Compat.hh for pekwm
// Copyright © 2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _COMPAT_HH_
#define _COMPAT_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cwchar>
#include <cstddef>

#ifndef HAVE_SWPRINTF
int swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...);
#endif // HAVE_SWPRINTF

#ifndef HAVE_SETENV
int setenv(const char *name, const char *value, int overwrite);
#endif // HAVE_SETENV

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
