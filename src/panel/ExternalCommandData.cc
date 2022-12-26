//
// ExternalCommandData.cc for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "ExternalCommandData.hh"

extern "C" {
#include <assert.h>
#include <errno.h>
#include <unistd.h>
}

// ExternalCommandData::CommandProcess

ExternalCommandData::CommandProcess::CommandProcess(const std::string& command,
						    uint interval_s)
	: _command(command),
	  _interval_s(interval_s),
	  _pid(-1),
	  _fd(-1)
{
	// set next interval to last second to ensure immediate
	// execution
	int ret = clock_gettime(CLOCK_MONOTONIC, &_next_interval);
	assert(ret == 0);
	_next_interval.tv_sec--;
}

ExternalCommandData::CommandProcess::~CommandProcess(void)
{
	reset();
}

bool
ExternalCommandData::CommandProcess::start(void)
{
	int fd[2];
	int ret = pipe(fd);
	if (ret == -1) {
		P_ERR("pipe failed due to: "
		      << strerror(errno));
		return false;
	}

	_pid = fork();
	if (_pid == -1) {
		close(fd[0]);
		close(fd[1]);
		P_ERR("fork failed due to: "
		      << strerror(errno));
		return false;
	} else if (_pid == 0) {
		// child, dup write end of file descriptor to
		// stdout
		dup2(fd[1], STDOUT_FILENO);

		close(fd[0]);
		close(fd[1]);

		char *argv[4];
		argv[0] = strdup("/bin/sh");
		argv[1] = strdup("-c");
		argv[2] = strdup(_command.c_str());
		argv[3] = NULL;
		execvp(argv[0], argv);

		P_ERR("failed to execute: " << _command);

		close(STDOUT_FILENO);
		exit(1);
	}

	// parent, close write end just going to read
	_fd = fd[0];
	close(fd[1]);
	Util::setNonBlock(_fd);
	if (Debug::isLevel(Debug::LEVEL_TRACE)) {
		std::ostringstream msg;
		msg << "pid " << _pid << " started with fd "
		    << _fd << " for command " << _command;
		P_TRACE(msg.str());
	}
	return true;
}

void
ExternalCommandData::CommandProcess::reset(void)
{
	_pid = -1;
	if (_fd != -1) {
		close(_fd);
	}
	_fd = -1;
	_buf = "";

	int ret = clock_gettime(CLOCK_MONOTONIC,
				&_next_interval);
	assert(ret == 0);
	_next_interval.tv_sec += _interval_s;
}


// ExternalCommandData

ExternalCommandData::ExternalCommandData(const PanelConfig& cfg,
					 VarData& var_data)
	: _cfg(cfg),
	  _var_data(var_data)
{
	PanelConfig::command_config_it it = _cfg.commandsBegin();
	for (; it != _cfg.commandsEnd(); ++it) {
		_command_processes.push_back(
				CommandProcess(it->getCommand(),
					       it->getIntervalS()));
	}
}

ExternalCommandData::~ExternalCommandData(void)
{
}

void
ExternalCommandData::refresh(fdFun addFd, void *opaque)
{
	struct timespec now;
	int ret = clock_gettime(CLOCK_MONOTONIC, &now);
	assert(ret == 0);

	std::vector<CommandProcess>::iterator it =
		_command_processes.begin();
	for (; it != _command_processes.end(); ++it) {
		if (it->getPid() == -1
		    && it->checkInterval(&now)
		    && it->start()) {
			addFd(it->getFd(), opaque);
		}
	}
}

bool
ExternalCommandData::input(int fd)
{
	char buf[1024];
	ssize_t nread = read(fd, buf, sizeof(buf));
	if (nread < 1) {
		if (nread == -1) {
			P_TRACE("failed to read from " << fd << ": "
				<< strerror(errno));
		}
		return false;
	}

	std::vector<CommandProcess>::iterator it =
		_command_processes.begin();
	for (; it != _command_processes.end(); ++it) {
		if (it->getFd() == fd) {
			append(it->getBuf(), buf, nread);
			break;
		}
	}

	return true;
}


void
ExternalCommandData::done(pid_t pid, fdFun removeFd, void *opaque)
{
	std::vector<CommandProcess>::iterator it =
		_command_processes.begin();
	for (; it != _command_processes.end(); ++it) {
		if (it->getPid() == pid) {
			while (input(it->getFd())) {
				// read data left in pipe if any
			}
			parseOutput(it->getBuf());
			removeFd(it->getFd(), opaque);

			// clean up state, resetting timer and pid/fd
			it->reset();
			break;
		}
	}
}

void
ExternalCommandData::append(std::string &buf, char *data, size_t size)
{
	buf.append(data, data + size);
	size_t pos = buf.find('\n');
	while (pos != std::string::npos) {
		std::string line = buf.substr(0, pos);
		parseLine(line);
		buf.erase(0, pos + 1);
		pos = buf.find('\n');
	}
}

void
ExternalCommandData::parseOutput(const std::string& buf)
{
	std::vector<std::string> lines;
	Util::splitString(buf, lines, "\n");
	std::vector<std::string>::iterator it = lines.begin();
	for (; it != lines.end(); ++it) {
		parseLine(*it);
	}
}

void
ExternalCommandData::parseLine(const std::string& line)
{
	std::vector<std::string> field_value;
	if (Util::splitString(line, field_value, " \t", 2) == 2) {
		_var_data.set(field_value[0], field_value[1]);
	}
}
