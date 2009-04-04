//
// Session.hh for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _SESSION_HH_
#define _SESSION_HH_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <string>

extern "C" {
#include <X11/SM/SMlib.h>
}

/**
 * Session manager session class.
 */
class Session
{
public:
    Session(int argc, char **argv);
    ~Session(void);

    /**< Return shutdown_flag pointer. */
    bool *getShutdownFlag(void) { return _shutdown_flag; }
    /**< Set shutdown_flag pointer. */
    void setShutdownFlag(bool *flag) { _shutdown_flag = flag; }

    bool save(void);
    bool load(void);

    void setProgram(void);
    void setUser(void);
    void setRestartStyle(bool restart);
    void setPid(void);
    void setPriority(void);
    void setCloneCommand(void);
    void setRestartCommand(void);

private:

    void setProperty(SmPropValue *vals, int num_vals,
                     const char *name, const char *type);

    bool startup(void);
    void shutdown(bool do_restart);

private:
    int _argc; /**< Argument count for application. */
    char **_argv; /**< Argument vector for application. */
    std::string _path; /**< Path to session file. */

    SmcConn _conn; /**< Connection to the session manager. */
    char *_session_id; /**< ID of current session. */
    bool *_shutdown_flag; /**< Pointer to boolean flag setting shutdown. */

    static const int ERROR_BUF_SIZE; /**< Session manager buffer size */
};

#endif // _SESSION_HH_
