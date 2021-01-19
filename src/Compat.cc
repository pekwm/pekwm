//
// Compat.cc for pekwm
// Copyright (C) 2009-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Compat.hh"
#include "Util.hh"

#include <iostream>
#include <string>
#include <cstring>

extern "C" {
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <wchar.h>
}

#ifndef HAVE_SWPRINTF
/**< Message displayed when %ls formatting is attempted. */
static const wchar_t *SWPRINTF_LS_NOT_SUPPORTED =
    L"%ls format string not supported";

/**
 * Compat swprintf, print formatted wide string.
 *
 * @param wcs Result string, maxlen long.
 * @param maxlen Maximum number of characters to print.
 * @param format Formatting string.
 * @param ... Formatting arguments.
 * @return Number of characters written or -1 on error.
 */
namespace std {
    int
    swprintf(wchar_t *wcs, size_t maxlen, const wchar_t *format, ...)
    {
        size_t len;
        string mb_format(Util::to_mb_str(format));

        // Look for wide string formatting, not yet implemented.
        if (mb_format.find("%ls") != string::npos) {
            len = std::min(wcslen(SWPRINTF_LS_NOT_SUPPORTED), maxlen - 1);
            wmemcpy(wcs, SWPRINTF_LS_NOT_SUPPORTED, len);
        } else {
            char *res = new char[maxlen];

            va_list ap;
            va_start(ap, format);
            vsnprintf(res, maxlen, mb_format.c_str(), ap);
            va_end(ap);

            wstring w_res(Util::to_wide_str(res));
            len = std::min(maxlen - 1, w_res.size());
            wmemcpy(wcs, w_res.c_str(), len);

            delete [] res;
        }

        // Null terminate and return result.
        wcs[len] = L'\0';

        return wcslen(wcs);
    }
}
#endif // HAVE_SWPRINTF

#ifndef HAVE_SETENV
/**
 * Compat setenv, insert variable to environment.
 */
int
setenv(const char *name, const char *value, int overwrite)
{
    // Invalid parameters
    if (! name || ! value) {
        return -1;
    }
    // Do not overwrite
    if (! overwrite && getenv(name)) {
        return 0;
    }

    size_t len = strlen(name) + strlen(value) + 2;
    char *str = new char[len];
    if (! str) {
        errno = ENOMEM;
        return -1;
    }

    snprintf(str, len, "%s=%s", name, value);

    return (putenv(str));
}
#endif // ! HAVE_SETENV

#ifndef HAVE_UNSETENV
/**
 * Compat unsetenv, removes variable from the environment.
 */
int
unsetenv(const char *name) {
    const char *value = getenv(name);
    if (value && strlen(value)) {
        return setenv(name, "", 1);
    } else {
        errno = EINVAL;
        return -1;
    }
}
#endif // ! HAVE_UNSETENV

#ifndef HAVE_DAEMON
/**
 * Compat daemon.
 */
int
daemon(int nochdir, int noclose)
{
    pid_t pid = fork();
    if (pid == -1) {
        return -1;
    } else if (pid == 0) {
        pid_t session = setsid();
        if (session == -1) {
            std::cerr << "failed to setsid, aborting: " << strerror(errno)
                      << std::endl;
            exit(1);
        }

        if (! nochdir) {
            if (chdir("/") == -1) {
                std::cerr << "failed to change directory to /" << std::endl;
            }
        }
        if (! noclose) {
            close(0);
            close(1);
            close(2);
        }

    } else {
        _exit(2);
    }

    return 0;
}
#endif // ! HAVE_DAEMON
