//
// Os.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
// 
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_OS_HH_
#define _PEKWM_OS_HH_

#include "config.h"
#include "Compat.hh"
#include "Mem.hh"

#include <map>
#include <string>
#include <vector>

extern "C" {
#include <sys/types.h>
#include <unistd.h>
}

#ifndef PEKWM_HAVE_ENVIRON
extern char **environ;
#endif // PEKWM_HAVE_ENVIRON

/**
 * OS environment wrapper, supports getting environments and providing
 * environment arrays with overriden values.
 */
class OsEnv {
public:
	OsEnv();
	~OsEnv();

	void override(const std::string& key, const std::string& value);

	const std::map<std::string, std::string>& getEnv();
	char** getCEnv();

private:
	OsEnv(const OsEnv&);
	OsEnv& operator=(const OsEnv&);

	char** toCEnv(const std::map<std::string, std::string> &env);
	void freeCEnv();

	void setEnvVar(std::map<std::string, std::string> &env,
		       const char *envp);

	bool _dirty;
	std::map<std::string, std::string> _env;
	std::map<std::string, std::string> _env_override;
	char **_c_env;
};

/**
 * File-descriptor select abstraction.
 */
class OsSelect {
public:
	enum Type {
		OS_SELECT_READ = 1 << 0,
		OS_SELECT_WRITE = 1 << 1,
		OS_SELECT_ALL = OS_SELECT_READ | OS_SELECT_WRITE
	};

	virtual ~OsSelect() { };

	virtual void add(int fd, int select_mask) = 0;
	virtual void remove(int fd) = 0;

	virtual bool next(int &fd, int &select_mask) = 0;
	virtual bool isSet(int fd, enum Type type) = 0;

	virtual bool wait(struct timeval *tv = nullptr) = 0;
};

/**
 * Child process with a pipe setup for communication with the calling process
 * on stdin/stdout.
 */
class ChildProcess {
public:
	enum ChildIo {
		CHILD_IO_STDIN = (1 << 0),
		CHILD_IO_STDOUT = (1 << 1),
		CHILD_IO_STDERR = (1 << 2),
		CHILD_IO_ALL = CHILD_IO_STDIN
			| CHILD_IO_STDOUT
			| CHILD_IO_STDERR
	};

	virtual ~ChildProcess() { }

	virtual bool wait(int &exitcode) = 0;

	virtual pid_t getPid() const = 0;
	virtual int getReadFd() const = 0;
	virtual int getWriteFd() const = 0;

	size_t write(const std::string &data)
	{
		return write(data.data(), data.size());
	}
	virtual size_t write(const char *data, size_t len) = 0;
	virtual size_t read(Buf<char> &buf, size_t len) = 0;
};

/**
 * Interface for Os functions.
 */
class Os {
public:
	virtual ~Os() { }

	virtual bool getEnv(const std::string &key, std::string &val,
			    const std::string &val_default = "") = 0;
	virtual bool setEnv(const std::string &key, const std::string &val) = 0;

	virtual bool pathExists(const std::string &path) = 0;

	virtual pid_t processExec(const std::vector<std::string> &args,
				  OsEnv *env = nullptr) = 0;
	virtual ChildProcess *childExec(const std::vector<std::string> &args,
					int flags, OsEnv *env = nullptr) = 0;
	virtual bool processSignal(pid_t pid, int signum) = 0;
	virtual bool isProcessAlive(pid_t pid) = 0;
};

Os *mkOs();
OsSelect *mkOsSelect();

#endif // _PEKWM_OS_HH_
