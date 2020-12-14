//
// Copyright (C) 2005-2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "CfgParserSource.hh"
#include "Util.hh"

#include <iostream>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include <stdio.h>
#include <unistd.h>
}

using std::cerr;
using std::endl;
using std::string;
using std::fopen;
using std::fclose;
using std::exit;

unsigned int CfgParserSourceCommand::_sigaction_counter = 0;

/**
 * Open file based configuration source.
 */
bool
CfgParserSourceFile::open(void)
{
    if (_file) {
        throw string("TRYING TO OPEN ALREADY OPEN SOURCE");
    }

    _file = fopen(_name.c_str(), "r");
    if (! _file) {
        throw string("failed to open file " + _name);
    }

    return true;
}

void
CfgParserSourceFile::close(void)
{
    if (! _file) {
        throw string("trying to close already closed source");
    }

    fclose(_file);
    _file = 0;
}

/**
 * Run command and treat output as configuration source.
 */
bool
CfgParserSourceCommand::open(void)
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

    _pid = fork();
    if (_pid == -1) { // Error
        return false;

    } else if (_pid == 0) { // Child
        dup2(fd[1], STDOUT_FILENO);

        ::close(fd[0]);
        ::close(fd[1]);

        execlp("/bin/sh", "sh", "-c", _name.c_str(), (char *) 0);

        // PRINT ERROR

        ::close (STDOUT_FILENO);

        exit (1);

    } else { // Parent

        ::close (fd[1]);

        _file = ::fdopen(fd[0], "r");
    }
    return true;
}

/**
 * Close source, wait for child process to finish.
 */
void
CfgParserSourceCommand::close(void)
{
    if (_sigaction_counter < 1) {
        return;
    }
    _sigaction_counter--;

    // Close source.
    fclose(_file);

    // Wait for process.
    int status;
    int pid_status;

    status = waitpid(_pid, &pid_status, 0);

    // If no other open CfgParserSourceCommand open, restore sigaction.
    if (_sigaction_counter == 0) {
        sigaction(SIGCHLD, &_sigaction, 0);
    }

    // Wait failed, throw error
    if (status == -1) {
        throw string("failed to wait for pid " + Util::to_string<int>(_pid));
    }
}
