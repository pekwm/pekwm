//
// pekwm.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"
#include "Compat.hh"
#include "CfgUtil.hh"
#include "pekwm_env.hh"

#include <iostream>
#include <vector>

extern "C" {
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
}

static int child_signal = 0;

static void
sigHandler(int signal)
{
	switch (signal) {
	case SIGHUP:
	case SIGINT:
	case SIGTERM:
		child_signal = signal;
		break;
	}
}

static int
waitPid(pid_t pid)
{
	int ret;
	while (waitpid(pid, &ret, 0) == -1 && errno == EINTR) {
		if (child_signal) {
			kill(pid, child_signal);
			child_signal = 0;
		}
	}

	if (WIFEXITED(ret)) {
		return WEXITSTATUS(ret);
	}
	return ret;
}

static bool
promptForRestart(const std::string& pekwm_dialog)
{
	pid_t pid = fork();
	if (pid == 0) {
		char *argv[] = {strdup(pekwm_dialog.c_str()),
			strdup("-o"), strdup("Restart"),
			strdup("-o"), strdup("Exit"),
			strdup("-t"), strdup("pekwm crashed!"),
			strdup("-r"),
			strdup("pekwm quit unexpectedly, restart?"),
			NULL};
		execvp(argv[0], argv);

		exit(1);
	} else if (pid == -1) {
		return false;
	} else {
		return waitPid(pid) == 0;
	}
}

static int
handleOkResult(char *path, char **argv, int read_fd)
{
	char buf[1024] = {0};
	if (read(read_fd, buf, sizeof(buf) - 1) == -1) {
		std::cerr << "failed to read pekwm_wm result due to: "
			  << strerror(errno) << std::endl;
	}
	close(read_fd);

	if (strncmp("stop", buf, 4) == 0) {
		return 0;
	}
	if (strncmp("error", buf, 5) == 0) {
		return 1;
	}
	if (strncmp("restart ", buf, 8) == 0) {
		std::string command = std::string(buf + 8);

		if (command.empty()) {
			execvp(path, argv);
		} else {
			command = "exec " + command;
			execl(PEKWM_SH, PEKWM_SH , "-c", command.c_str(),
			      NULL);
		}

		std::cerr << "failed to run restart command: "
			  << command << std::endl;
	}

	return 1;
}


static int
handleUnexpectedResult(char *path, char **argv, int read_fd)
{
	close(read_fd);

	// run pekwm_dialog and wait for answer on to restart or not
	std::string pekwm_dialog = std::string(path) + "_dialog";
	if (promptForRestart(pekwm_dialog)) {
		execvp(path, argv);
		std::cerr << "failed to restart pekwm" << std::endl;
		exit(1);
	}

	std::cerr << "not restarting after crash" << std::endl;

	return 1;
}

/**
 * Main function of pekwm
 */
int
main(int argc, char **argv)
{
	initEnv(true);

	if (argc > 1 && strcmp("--standalone", argv[1]) == 0) {
		std::cerr << "--standalone only supported using pekwm_wm"
			  << std::endl;
		return 1;
	}

	// Get the pekwm_wm command by appending _wm to the path to ensure
	// the correct pekwm_wm is used when running from a non-installed
	// location.
	std::string pekwm_wm = std::string(argv[0]) + "_wm";

	// Setup environment, DISPLAY is set to make RestartOther work as
	// expected re-using the display from the --display argument.
	//
	// PEKWM_CONFIG_FILE is set as an environment to make pekwm_dialog
	// catch the correct configuration file.
	std::vector<std::string> wm_argv;
	wm_argv.push_back(pekwm_wm);
	for (int i = 1; i < argc; i++) {
		if ((strcmp("--display", argv[i]) == 0) && ((i + 1) < argc)) {
			setenv("DISPLAY", argv[++i], 1);
		} else if ((strcmp("--config", argv[i]) == 0)
			   && ((i + 1) < argc)) {
			setenv("PEKWM_CONFIG_FILE", argv[++i], 1);
		} else {
			wm_argv.push_back(argv[i]);
		}
	}

	// Run the window manager inside a child process to avoid stopping
	// the X11 server on a crash, instead just restart
	int fd[2];
	if (pipe(fd) == -1) {
		std::cerr << "Failed to create pipe for communicating with "
			  << " pekwm_wm process" << std::endl;
		return 1;
	}

	int ret = 1;
	pid_t pid = fork();
	if (pid == 0) {
		// close reading end on child
		close(fd[0]);

		wm_argv.insert(wm_argv.begin() + 1, "--fd");
		wm_argv.insert(wm_argv.begin() + 2, std::to_string(fd[1]));

		int c_wm_argc = 0;
		char **c_wm_argv = new char*[wm_argv.size() + 1];
		std::vector<std::string>::iterator it = wm_argv.begin();
		for (; it != wm_argv.end(); ++it) {
			c_wm_argv[c_wm_argc++] = strdup(it->c_str());
		}
		c_wm_argv[c_wm_argc++] = NULL;

		execvp(c_wm_argv[0], c_wm_argv);

		std::cerr << "Failed to execute: " << pekwm_wm << std::endl;
		exit(1);
	} else if (pid == -1) {
		std::cerr << "Failed to fork: " << strerror(errno) << std::endl;
	} else {
		// close writing end on parent
		close(fd[1]);

		struct sigaction act;
		act.sa_handler = sigHandler;
		act.sa_mask = sigset_t();
		act.sa_flags = SA_NOCLDSTOP | SA_NODEFER;
		sigaction(SIGHUP, &act, 0);
		sigaction(SIGTERM, &act, 0);
		sigaction(SIGINT, &act, 0);

		ret = waitPid(pid);
		if (ret == 0) {
			ret = handleOkResult(argv[0], argv, fd[0]);
		} else {
			ret = handleUnexpectedResult(argv[0], argv, fd[0]);
		}
	}

	return ret;
}
