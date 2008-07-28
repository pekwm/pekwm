//
// Copyright © 2005-2008 Claes Nasten <me{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "CfgParserSource.hh"
#include "Util.hh"

#include <iostream>
#include <cstdlib>

extern "C" {
#include <unistd.h>
}

using std::cerr;
using std::endl;
using std::string;

unsigned int CfgParserSourceCommand::_sigaction_counter = 0;

//! @brief
//! @return
bool
CfgParserSourceFile::open (void)
throw (std::string&)
{
    if (_op_file) {
        throw string ("TRYING TO OPEN ALLREADY OPEN SOURCE");
    }

    _op_file = fopen (_or_name.c_str (), "r");
    if (!_op_file)  {
        throw string("FAILED TO OPEN " + _or_name);
    }

    return true;
}

//! @brief
void
CfgParserSourceFile::close (void)
throw (std::string&)
{
    if (!_op_file) {
        throw string("TRYING TO CLOSE ALLREADY CLOSED SOURCE");
    }

    fclose (_op_file);
    _op_file = NULL;
}

//! @brief
//! @param
//! @return
bool
CfgParserSourceCommand::open (void)
throw (std::string&)
{
    int fd[2];
    int status;

    status = pipe(fd);
    if (status == -1) {
        return false;
    }

    // Remove signal handler while parsing as otherwise reading from the
    // pipe will break sometimes.
    if (_sigaction_counter++ == 0) {
        struct sigaction action;

        action.sa_handler = SIG_DFL;
        action.sa_mask = sigset_t();
        action.sa_flags = 0;

        sigaction(SIGCHLD, &action, &_sigaction);
    }

    _o_pid = fork ();
    if (_o_pid == -1) { // Error
        return false;

    } else if (_o_pid == 0) { // Child
        dup2 (fd[1], STDOUT_FILENO);

        ::close (fd[0]);
        ::close (fd[1]);

        execlp ("/bin/sh", "sh", "-c", _or_name.c_str(), (char *) NULL);

        // PRINT ERROR

        ::close (STDOUT_FILENO);

        exit (1);

    } else { // Parent

        ::close (fd[1]);

        _op_file = fdopen (fd[0], "r");
    }
    return true;
}

//! @brief
void
CfgParserSourceCommand::close (void)
throw (std::string&)
{
    if (_sigaction_counter < 1) {
        return;
    }
    _sigaction_counter--;

    // Close source.
    fclose (_op_file);

    // Wait for process.
    int status;
    int pid_status;

    status = waitpid(_o_pid, &pid_status, 0);

    // If no other open CfgParserSourceCommand open, restore sigaction.
    if (_sigaction_counter == 0) {
        sigaction(SIGCHLD, &_sigaction, NULL);
    }

    // Wait failed, throw error
    if (status == -1) {
        throw string("FAILED TO WAIT FOR PID " + Util::to_string<int> (_o_pid));
    }
}
