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

/**
 * Default implementation of the Os interface.
 */
class OsImpl : public Os {
public:
	OsImpl() : Os() { }
	virtual ~OsImpl() { }

	/**
	 * Fork and exec, return pid if successful, log on error.
	 */
	virtual pid_t processExec(const std::vector<std::string> &args)
	{
		assert(! args.empty());

		pid_t pid = fork();
		switch (pid) {
		case 0: {
			int i = 0;
			char **argv = new char*[args.size() + 1];
			std::vector<std::string>::const_iterator it =
				args.begin();
			for (; it != args.end(); ++it) {
				argv[i++] = const_cast<char*>(it->c_str());
			}
			argv[i] = nullptr;

			setsid();
			execvp(argv[0], argv);
			exit(1);
		}
		case -1:
			P_ERR("fork failed: " << strerror(errno));
		default:
			P_TRACE("started child " << pid);
			return pid;
		}
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
