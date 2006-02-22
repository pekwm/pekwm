//! @file
//! @author Claes Nasten <pekdon{@}pekdon{.}net
//! @date 2005-06-15
//! @brief Configuration parser source handler.

//
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "CfgParserSource.hh"
#include "Util.hh"

#include <iostream>

extern "C" {
#include <unistd.h>
}

using std::cerr;
using std::endl;
using std::string;

unsigned int CfgParserSourceCommand::m_sigaction_counter = 0;

//! @brief
//! @return
bool
CfgParserSourceFile::open (void)
  throw (std::string&)
{
  if (m_op_file) {
    throw string ("TRYING TO OPEN ALLREADY OPEN SOURCE");
  }

  m_op_file = fopen (m_or_name.c_str (), "r");
  if (!m_op_file)  {
    throw string("FAILED TO OPEN " + m_or_name);
  }

  return true;
}

//! @brief
void
CfgParserSourceFile::close (void)
  throw (std::string&)
{
  if (!m_op_file) {
    throw string("TRYING TO CLOSE ALLREADY CLOSED SOURCE");
  }

  fclose (m_op_file);
  m_op_file = NULL;
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
  if (m_sigaction_counter++ == 0) {
    struct sigaction action;

    action.sa_handler = SIG_DFL;
    action.sa_mask = sigset_t();
    action.sa_flags = 0;

    sigaction(SIGCHLD, &action, &m_sigaction);
  }

  m_o_pid = fork ();
  if (m_o_pid == -1) { // Error
    return false;

  } else if (m_o_pid == 0) { // Child
    dup2 (fd[1], STDOUT_FILENO);

    ::close (fd[0]);
    ::close (fd[1]);

    execlp ("/bin/sh", "sh", "-c", m_or_name.c_str(), (char *) NULL);

    // PRINT ERROR

    ::close (STDOUT_FILENO);

    exit (1);

  } else { // Parent

    ::close (fd[1]);

    m_op_file = fdopen (fd[0], "r");
  }
  return true;
}

//! @brief
void
CfgParserSourceCommand::close (void)
  throw (std::string&)
{
  if (m_sigaction_counter < 1) {
    return;
  }
  m_sigaction_counter--;

  // Close source.
  fclose (m_op_file);

  // Wait for process.
  int status;
  int pid_status;

  status = waitpid(m_o_pid, &pid_status, 0);

  // If no other open CfgParserSourceCommand open, restore sigaction.
  if (m_sigaction_counter == 0) {
    sigaction(SIGCHLD, &m_sigaction, NULL);
  }

  // Wait failed, throw error
  if (status == -1) {
    throw string("FAILED TO WAIT FOR PID " + Util::to_string<int> (m_o_pid));
  }
}
