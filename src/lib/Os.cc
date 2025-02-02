//
// Os.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Debug.hh"
#include "Os.hh"

extern "C" {
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
}

OsEnv::OsEnv()
	: _dirty(false),
	  _c_env(nullptr)
{
}

OsEnv::~OsEnv()
{
	freeCEnv();
}

/**
 * Set override value in environment, will replace environment variable
 * with the provided value.
 */
void
OsEnv::override(const std::string& key, const std::string& value)

{
	_dirty = true;
	_env_override[key] = value;
}

/**
 * Get merged environment as a map, overrides are in effect.
 */
const std::map<std::string, std::string>&
OsEnv::getEnv()
{
	if (_env.empty()) {
		getCEnv();
	}
	return _env;
}

/**
 * Get merged environment as a nullptr terminated C array, memory will
 * be freed when the OsEnv goes out of scope or new overrides have been
 * set and a getCEnv is called.
 */
char**
OsEnv::getCEnv()
{
	if (! _dirty && _c_env) {
		return _c_env;
	}

	_env.clear();
	freeCEnv();

	// build environment from process environment
	for (char **p = environ; *p; p++) {
		setEnvVar(_env, *p);
	}

	// override environment variables
	std::map<std::string, std::string>::const_iterator
		it(_env_override.begin());
	for (; it != _env_override.end(); ++it) {
		_env[it->first] = it->second;
	}

	_c_env = toCEnv(_env);
	return _c_env;
}

char**
OsEnv::toCEnv(const std::map<std::string, std::string> &env)
{
	size_t i = 0;
	char **c_env = new char*[env.size() + 1];

	std::map<std::string, std::string>::const_iterator it(env.begin());
	for (; it != env.end(); ++it) {
		std::string val = it->first + "=" + it->second;
		c_env[i++] = strdup(val.c_str());
	}
	c_env[env.size()] = nullptr;

	return c_env;
}

void
OsEnv::freeCEnv()
{
	if (! _c_env) {
		return;
	}

	for (size_t i = 0; _c_env[i] != nullptr; i++) {
		free(_c_env[i]);
	}
	delete[] _c_env;
	_c_env = nullptr;
}

void
OsEnv::setEnvVar(std::map<std::string, std::string> &env, const char *envp)
{
	const char *start = strchr(envp, '=');
	if (start == nullptr) {
		env[envp] = "";
	} else {
		std::string key(envp, start - envp);
		std::string value(start + 1);
		env[key] = value;
	}
}

static void
_exec_args(const std::vector<std::string> &args, OsEnv *env)
{
	int i = 0;
	char **argv = new char*[args.size() + 1];
	std::vector<std::string>::const_iterator it = args.begin();
	for (; it != args.end(); ++it) {
		argv[i++] = const_cast<char*>(it->c_str());
	}
	argv[i] = nullptr;

	if (env) {
		execvpe(argv[0], argv, env->getCEnv());
	} else {
		execvp(argv[0], argv);
	}
	// error occured, exit with failure
	exit(1);
}

/**
 * Default implementation of the OsSelect interface.
 */
class OsSelectImpl : public OsSelect {
public:
	typedef std::vector<std::pair<int, int> > fd_vector;

	OsSelectImpl()
		: _max_fd(-1),
		  _pos(0)
	{
	}
	virtual ~OsSelectImpl() { }

	virtual void add(int fd, int select_mask)
	{
		_fds.push_back(std::pair<int,int>(fd, select_mask));
		if (fd > _max_fd) {
			_max_fd = fd;
		}
	}

	virtual void remove(int fd)
	{
		fd_vector::iterator it(_fds.begin());
		for (; it != _fds.end(); ++it) {
			if (it->first == fd) {
				_fds.erase(it);
				break;
			}
		}
		setMaxFd();
	}

	virtual bool isSet(int fd, enum Type type)
	{
		if (type == OS_SELECT_READ) {
			return FD_ISSET(fd, &_rfds);
		} else if (type == OS_SELECT_WRITE) {
			return FD_ISSET(fd, &_wfds);
		}
		return false;
	}

	virtual bool wait(struct timeval *tv)
	{
		_pos = 0;
		initFdSets();
		int ret = select(_max_fd + 1, &_rfds, &_wfds, nullptr, tv);
		return ret > 0;
	}

	virtual bool next(int &fd, int &mask)
	{
		while (_pos < _fds.size()) {
			fd = _fds[_pos++].first;
			mask = 0;
			if (FD_ISSET(fd, &_rfds)) {
				mask |= OS_SELECT_READ;
			}
			if (FD_ISSET(fd, &_wfds)) {
				mask |= OS_SELECT_WRITE;
			}
			if (mask) {
				return true;
			}

		}
		return false;
	}

private:
	void initFdSets()
	{
		FD_ZERO(&_rfds);
		FD_ZERO(&_wfds);
		fd_vector::iterator it(_fds.begin());
		for (; it != _fds.end(); ++it) {
			if (it->second & OS_SELECT_READ) {
				FD_SET(it->first, &_rfds);
			}
			if (it->second & OS_SELECT_WRITE) {
				FD_SET(it->first, &_wfds);
			}
		}
	}

	void setMaxFd()
	{
		int max_fd = -1;
		fd_vector::iterator it(_fds.begin());
		for (; it != _fds.end(); ++it) {
			if (it->first > max_fd) {
				max_fd = it->first;
			}
		}
		_max_fd = max_fd;
	}

	int _max_fd;
	fd_vector _fds;
	size_t _pos;
	fd_set _rfds;
	fd_set _wfds;
};

/**
 * Default implementation of the ChildProcess interface.
 */
class ChildProcessImpl : public ChildProcess {
public:
	ChildProcessImpl(const std::vector<std::string> &args, int flags,
			 OsEnv *env)
		: _pid(-1)
	{
		_pipe[0] = -1;
		_pipe[1] = -1;

		int err = pipe(_pipe);
		if (err == -1) {
			P_ERR("pipe failed: " << strerror(errno));
			return;
		}

		_pid = fork();
		if (_pid == -1) {
			// throw error
			P_ERR("fork failed: " << strerror(errno));
		} else if (_pid == 0) {
			// child process
			if (flags & CHILD_IO_STDOUT) {
				dup2(getWriteFd(), STDOUT_FILENO);
			} else {
				::close(getWriteFd());
			}
			if (flags & CHILD_IO_STDIN) {
				dup2(getReadFd(), STDIN_FILENO);
			} else {
				::close(getReadFd());
			}

			_exec_args(args, env);
		} else {
			P_TRACE("started child process " << _pid);
			// inverse, no output means no input and vice versa
			if (! (flags & CHILD_IO_STDOUT)) {
				::close(getReadFd());
			}
			if (! (flags & CHILD_IO_STDIN)) {
				::close(getWriteFd());
			}
		}
	}
	virtual ~ChildProcessImpl()
	{
		int exitcode;
		wait(exitcode);
	}

	virtual bool wait(int &exitcode)
	{
		if (_pid == -1) {
			return true;
		}

		int pid_status;
		int status = waitpid(_pid, &pid_status, 0);
		if (status == -1) {
			P_ERR("failed to wait for pid " << _pid);
		}
		_pid = -1;
		closeFd(_pipe[0]);
		closeFd(_pipe[1]);
		return _pid == -1;
	}

	virtual pid_t getPid() const { return _pid; }
	virtual int getReadFd() const { return _pipe[0]; }
	virtual int getWriteFd() const { return _pipe[1]; }

	virtual size_t write(const char *data, size_t len)
	{
		return ::write(getWriteFd(), data, len);
	}

	virtual size_t read(Buf<char> &buf, size_t len)
	{
		buf.ensure(len, false);
		return ::read(getReadFd(), *buf, len);
	}

private:
	void closeFd(int &fd) const { if (fd != -1) { ::close(fd); fd = -1; } }

	pid_t _pid;
	int _pipe[2];
};

/**
 * Default implementation of the Os interface.
 */
class OsImpl : public Os {
public:
	OsImpl() : Os() { }
	virtual ~OsImpl() { }

	/**
	 * Set OS environmnet variable.
	 */
	virtual bool getEnv(const std::string &key, std::string &val,
			    const std::string &val_default)
	{
		char *c_val = getenv(key.c_str());
		if (c_val == nullptr) {
			val = val_default;
			return false;
		}
		val = c_val;
		return true;
	}

	/**
	 * Set OS environment variable.
	 */
	virtual bool setEnv(const std::string &key, const std::string &val)
	{
		int err = setenv(key.c_str(), val.c_str(), 1 /* overwrite */);
		return ! err;
	}

	/**
	 * Fork and exec, return pid if successful, log on error.
	 */
	virtual pid_t processExec(const std::vector<std::string> &args)
	{
		assert(! args.empty());

		pid_t pid = fork();
		switch (pid) {
		case 0:
			setsid();
			_exec_args(args, nullptr);
		case -1:
			P_ERR("fork failed: " << strerror(errno));
		default:
			P_TRACE("started child " << pid);
			return pid;
		}
	}

	/**
	 * Fork and exec child process, setting up a pipe for communication
	 * over stdin/stdout.
	 */
	virtual ChildProcess *childExec(const std::vector<std::string> &args,
					int flags, OsEnv *env)
	{
		ChildProcess *process = new ChildProcessImpl(args, flags, env);
		if (process->getPid() == -1) {
			delete process;
			return nullptr;
		}
		return process;
	}

	/**
	 * Wrapper for kill.
	 */
	virtual bool processSignal(pid_t pid, int signum)
	{
		return kill(pid, signum) == 0;
	}

	/**
	 * Wrapper for waitpid return true if the provided PID is alive.
	 */
	virtual bool isProcessAlive(pid_t pid)
	{
		// invalid pid, can not be running
		if (static_cast<pid_t>(-1) == pid) {
			return false;
		}

		int status;
		int ret;
		do {
			ret = waitpid(pid, &status, WNOHANG);
		} while (ret == -1 && errno == EINTR);
		return ret == 0;
	}
};

/**
 * Create new Os interface.
 */
Os *mkOs()
{
	return new OsImpl();
}

/**
 * Create new OsSelect interface.
 */
OsSelect *mkOsSelect()
{
	return new OsSelectImpl();
}
