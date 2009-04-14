//
// Session.hh for pekwm
// Copyright © 2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

//
// Code based much on session code from openbox which in turn was
// much based on the metacity code.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifdef HAVE_SESSION

#include "Session.hh"
#include "Util.hh"

#include <cstdlib>
#include <list>
#include <iostream>

extern "C" {
#include <unistd.h>
#include <X11/SM/SMlib.h>
}

using std::cerr;
using std::endl;
using std::list;

const int Session::ERROR_BUF_SIZE = 1024;

extern "C" {

/**
 * Callback doing the actual saving of the current session,
 * this is a two phase setup due to ???
 */
static void
session_save_phase2(SmcConn conn, SmPointer data)
{
    Session *session = reinterpret_cast<Session*>(data);
    if (session->save()) {
        session->setRestartCommand();
        SmcSaveYourselfDone(conn, true);
    } else {
        SmcSaveYourselfDone(conn, false);
    }
}

/**
 * Callback saving the current session.
 */
static void
session_save(SmcConn conn, SmPointer data, int save_type,
	     Bool shutdown, int interact_style, Bool fast)
{
    // Check save type, if SmSaveGlobal store the state and return.
    if (save_type == SmSaveGlobal) {
        SmcSaveYourselfDone(conn, true);
        return;
    }

    if (! SmcRequestSaveYourselfPhase2(conn, session_save_phase2, data)) {
        cerr << " *** WARNING: session save failed" << endl;
        SmcSaveYourselfDone(conn, false);
    }
}

/**
 * Callback called after save of session is complete.
 */
static void
session_save_complete(SmcConn conn, SmPointer data)
{
    // Do nothing
}

/**
 * Callback called if shutdown is canceled.
 */
static void
session_shutdown_cancelled(SmcConn conn, SmPointer data)
{
    // Do nothing
}

/**
 * Callback
 */
static void
session_die(SmcConn conn, SmPointer data)
{
    Session *session = reinterpret_cast<Session*>(data);
    if (session->getShutdownFlag()) {
        *session->getShutdownFlag() = true;
    } else {
        // Session exit before window manager has been setup.
        exit(0);
    }
}

}

/**
 * Session constructor.
 */
Session::Session(int argc, char **argv)
    : _argc(argc), _argv(argv),
      _conn(0), _session_id(0), _shutdown_flag(0)
{
    if (startup()) {
        setProgram();
        setUser();
        setRestartStyle(true);
        setPid();
        setPriority();
        setCloneCommand();
    }
}

/**
 * Session destructor.
 */
Session::~Session(void)
{
    shutdown(false);

    delete _session_id;
}


/**
 * Connect to current session.
 */
bool
Session::startup(void)
{
    SmcCallbacks cb;

    // Setup session callbacks
    cb.save_yourself.callback = session_save;
    cb.save_yourself.client_data = this;
    cb.die.callback = session_die;
    cb.die.client_data = this;
    cb.save_complete.callback = session_save_complete;
    cb.save_complete.client_data = this;
    cb.shutdown_cancelled.callback = session_shutdown_cancelled;
    cb.shutdown_cancelled.client_data = this;

    // Connect to the server
    long mask = SmcSaveYourselfProcMask|SmcDieProcMask
        |SmcSaveCompleteProcMask|SmcShutdownCancelledProcMask;

    char buf[ERROR_BUF_SIZE + 1];
    _conn = SmcOpenConnection(0, 0, 1, 0, mask, &cb, 0, &_session_id,
                              ERROR_BUF_SIZE, buf);
    if (_conn == 0) {
        cerr << " *** WARNING: failed to connect to session manager " << endl; 
    }
    return _conn != 0;
}

/**
 * Shutdown session manager connection.
 */
void
Session::shutdown(bool do_restart)
{
    // No connection to shutdown.
    if (! _conn) {
        return;
    }

    // Do a real shutdown, make sure the session manager does not restart.
    if (! do_restart) {
        setRestartStyle(false);
    }

    SmcCloseConnection(_conn, 0, 0);
}

/**
 * Save session to disk.
 */
bool
Session::save(void)
{
    cerr << " *** WARNING: session save is not yet implemented." << endl;

    return true;
}

/**
 * Load session from disk.
 */
bool
Session::load(void)
{
    cerr << " *** WARNING: session load is not yet implemented." << endl;

    return true;
}

/**
 * Set session program.
 */
void
Session::setProgram(void)
{
    SmPropValue vals;
    vals.value = _argv[0];
    vals.length = strlen(static_cast<const char*>(vals.value)) + 1;

    setProperty(&vals, 1, SmProgram, SmARRAY8);
}

/**
 * Set session user.
 */
void
Session::setUser(void)
{
    SmPropValue vals;
    vals.value = strdup(Util::getUserName().c_str());
    vals.length = strlen(static_cast<const char*>(vals.value)) + 1;

    setProperty(&vals, 1, SmUserID, SmARRAY8);
}

/**
 * Set session restart type.
 */
void
Session::setRestartStyle(bool restart)
{
    char restart_hint = restart ? SmRestartImmediately : SmRestartIfRunning;

    SmPropValue vals;
    vals.value = &restart_hint;
    vals.length = 1;

    setProperty(&vals, 1, SmRestartStyleHint, SmCARD8);
}

/**
 * Set session pid.
 */
void
Session::setPid(void)
{
    SmPropValue vals;
    vals.value = strdup(Util::to_string<pid_t>(getpid()).c_str());
    vals.length = strlen(static_cast<const char*>(vals.value)) + 1;

    setProperty(&vals, 1, SmProcessID, SmARRAY8);

    free(vals.value);
}

/**
 * Set session priority
 */
void
Session::setPriority(void)
{
    // Priroty 20 is low enough to run before other applications
    char priority = 20;

    SmPropValue vals;
    vals.value = &priority;
    vals.length = 1;

    setProperty(&vals, 1, "_GSM_Priority", SmCARD8);
}

/**
 * Set session clone command
 */
void
Session::setCloneCommand(void)
{
    SmPropValue *vals = new SmPropValue[_argc];

    // Setup argument vector for cloning
    for (int i = 0; i < _argc; ++i) {
        vals[i].value = _argv[i];
        vals[i].length = strlen(_argv[i]) + 1;
    }

    setProperty(vals, _argc, SmCloneCommand, SmLISTofARRAY8);
    
    delete [] vals;
}

/**
 * Set session restart command.
 */
void
Session::setRestartCommand(void)
{
    // Build new argument vector.
    list<const char*> arguments(_argv, _argv + _argc);
    arguments.push_back("--sm-client-id");
    arguments.push_back(_session_id);
    arguments.push_back("--sm-save-file");
    arguments.push_back(_path.c_str());

    // Copy argument vector into vals and set
    SmPropValue *vals = new SmPropValue[arguments.size()];

    list<const char*>::iterator it(arguments.begin());
    for (unsigned int i = 0; it != arguments.end(); ++it, ++i) {
        vals[i].value = strdup(*it);
        vals[i].length = strlen(static_cast<const char*>(vals[i].value)) + 1;
    }

    setProperty(vals, _argc + 4, SmRestartCommand, SmLISTofARRAY8);

    // Cleanup
    for (unsigned int i = 0; i < arguments.size(); ++i) {
        free(vals[i].value);
    }
    delete [] vals;
}

/**
 * Wrapper for SmcSetProperties, does all the work but setting up
 * vals.
 *
 * @param vals SmPropValue array.
 * @param num_valus Number of values.
 * @param name Name of the property.
 * @param type Type of property.
 */
void
Session::setProperty(SmPropValue *vals, int num_vals,
                     const char *name, const char *type)
{
    SmProp prop;
    prop.name = strdup(name);
    prop.type = strdup(type);
    prop.num_vals = num_vals;
    prop.vals = vals;

    SmProp *list = &prop;
    SmcSetProperties(_conn, 1, &list);
    free(prop.name);
    free(prop.type);
}

#endif // HAVE_SESSION
