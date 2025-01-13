//
// Copyright (C) 2005-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "CfgParserSource.hh"
#include "Os.hh"
#include "Util.hh"

#include <iostream>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include <stdio.h>
#include <unistd.h>
}

unsigned int CfgParserSourceCommand::_sigaction_counter = 0;

CfgParserSource::CfgParserSource(const std::string &source)
	: _name(source),
	  _type(SOURCE_VIRTUAL),
	  _line(0),
	  _is_dynamic(false)
{
}

CfgParserSource::~CfgParserSource (void)
{
}

/**
 * Open file based configuration source.
 */
bool
CfgParserSourceFile::open(void)
{
	if (_file) {
		throw std::string("TRYING TO OPEN ALREADY OPEN SOURCE");
	}

	_file = fopen(_name.c_str(), "r");
	if (! _file) {
		throw std::string("failed to open file " + _name);
	}

	return true;
}

void
CfgParserSourceFile::close(void)
{
	if (! _file) {
		throw std::string("trying to close already closed source");
	}

	fclose(_file);
	_file = 0;
}


CfgParserSourceString::CfgParserSourceString(const std::string &source,
					     const std::string &data)
	: CfgParserSource(source),
	  _data(data)
{
	_pos = _data.begin();
}

CfgParserSourceString::~CfgParserSourceString(void)
{
}

bool
CfgParserSourceString::open(void)
{
	_pos = _data.begin();
	return true;
}

void
CfgParserSourceString::close(void)
{
	_pos = _data.end();
}

int
CfgParserSourceString::get_char(void)
{
	if (_pos == _data.end()) {
		return EOF;
	}
	return CfgParserSource::do_get_char(*_pos++);
}

void
CfgParserSourceString::unget_char(int c)
{
	if (_pos != _data.begin()) {
		CfgParserSource::unget_char(*_pos--);
	}
}

/**
 * Run command and treat output as configuration source.
 */
bool
CfgParserSourceCommand::open(void)
{
	// Remove signal handler while parsing as otherwise reading from the
	// pipe will break sometimes.
	if (_sigaction_counter++ == 0) {
		struct sigaction action;

		action.sa_handler = SIG_DFL;
		action.sa_mask = sigset_t();
		action.sa_flags = 0;

		sigaction(SIGCHLD, &action, &_sigaction);
	}

	// Run with command path in the PATH
	OsEnv env;
	std::string path(Util::getEnv("PATH"));
	path = _command_path + ":" + path;
	env.override("PATH", path);

	std::vector<std::string> args;
	args.push_back(PEKWM_SH);
	args.push_back("-c");
	args.push_back(_name);
	_process = _os->childExec(args, ChildProcess::CHILD_IO_STDOUT, &env);
	if (! _process) {
		return false;
	}
	_file = ::fdopen(_process->getReadFd(), "r");
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

	fclose(_file);
	pid_t pid = _process->getPid();
	int exitcode;
	bool status = _process->wait(exitcode);
	delete _process;
	_process = nullptr;

	// If no other open CfgParserSourceCommand open, restore sigaction.
	if (_sigaction_counter == 0) {
		sigaction(SIGCHLD, &_sigaction, 0);
	}

	// Wait failed, throw error
	if (! status) {
		std::string msg =
			"failed to wait for pid " + std::to_string(pid);
		throw(msg);
	}
}
