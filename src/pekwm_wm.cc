//
// pekwm_wm.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Charset.hh"
#include "Compat.hh"
#include "Debug.hh"
#include "WindowManager.hh"
#include "Util.hh"

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>

extern "C" {
#include <errno.h>
#include <unistd.h> // execlp
}

/**
 * Prints version
 */
static void
printVersion(void)
{
	std::cout << "pekwm: version " << VERSION << std::endl;
}

/**
 * Prints version and availible options
 */
static void
printUsage(void)
{
	printVersion();
	std::cout << " --config     alternative config file" << std::endl;
	std::cout << " --display    display to connect to" << std::endl;
	std::cout << " --help       show this info." << std::endl;
	std::cout << " --info       extended info. Use for bug reports."
		  << std::endl;
	std::cout << " --log-file   set log file." << std::endl;
	std::cout << " --log-level  set log level." << std::endl;
	std::cout << " --replace    replace running window manager"
		  << std::endl;
	std::cout << " --sync       run Xlib in synchronous mode" << std::endl;
	std::cout << " --standalone run pekwm_wm in standalone mode"
		  << std::endl;
	std::cout << " --version    show version info" << std::endl;
}

/**
 * Prints version and build-time options
 */
static void
printInfo(void)
{
	printVersion();
	std::cout << "features: " << FEATURES << std::endl;
}

static void
setPekwmEnv(void)
{
	setenv("PEKWM_ETC_PATH", SYSCONFDIR, 1);
	setenv("PEKWM_SCRIPT_PATH", DATADIR "/pekwm/scripts", 1);
	setenv("PEKWM_THEME_PATH", DATADIR "/pekwm/themes", 1);
}

static void
stop(int write_fd, const std::string &msg, WindowManager *wm)
{
	if (wm) {
		delete wm;
		pekwm::cleanup();
	}

	Charset::destruct();

	if (write_fd != -1
	    && write(write_fd, msg.c_str(), msg.size() + 1) == -1) {
		P_ERR("failed to write pekwm_wm result msg " << msg
		      << " due to: " << strerror(errno));
	}
	exit(0);
}

/**
 * Main function of pekwm
 */
int
main(int argc, char **argv)
{
	Charset::init();

	// get the args and test for different options
	bool synchronous = false;
	bool replace = false;

	// force --standalone option to be the first option as it
	// enables/disables the --fd/--display and --config options
	int i, write_fd;
	bool standalone;
	if (argc > 1 && strcmp("--standalone", argv[1]) == 0) {
		i = 2;
		standalone = true;
		write_fd = -1;
		setPekwmEnv();
	} else {
		i = 1;
		standalone = false;
		write_fd = fileno(stdout);
	}

	for (; i < argc; ++i) {
		if (strcmp("--info", argv[i]) == 0) {
			printInfo();
			stop(write_fd, "stop", nullptr);
		} else if (strcmp("--log-level", argv[i]) == 0 && ((i + 1) < argc)) {
			Debug::setLevel(Debug::getLevel(argv[++i]));
		} else if (strcmp("--log-file", argv[i]) == 0 && ((i + 1) < argc)) {
			if (! Debug::setLogFile(argv[++i])) {
				std::cerr << "Failed to open log file " << argv[i] << std::endl;
			}
		} else if (strcmp("--replace", argv[i]) == 0) {
			replace = true;
		} else if (strcmp("--sync", argv[i]) == 0) {
			synchronous = true;
		} else if (strcmp("--version", argv[i]) == 0) {
			printVersion();
			stop(write_fd, "stop", nullptr);
		} else if (! standalone && strcmp("--fd", argv[i]) == 0
			   && ((i + 1) < argc)) {
			write_fd = std::stoi(argv[++i]);
		} else if (standalone && (strcmp("--display", argv[i]) == 0)
			   && ((i + 1) < argc)) {
			setenv("DISPLAY", argv[++i], 1);
		} else if (standalone && strcmp("--config", argv[i]) == 0
			   && (i + 1) < argc) {
			setenv("PEKWM_CONFIG_FILE", argv[++i], 1);
		} else {
			printUsage();
			stop(write_fd, "stop", nullptr);
		}
	}

	// Get configuration file if none was specified as a parameter,
	// default to reading environment, if not set get ~/.pekwm/config
	std::string config_file = Util::getEnv("PEKWM_CONFIG_FILE");
	if (config_file.size() == 0) {
		std::string home = Util::getEnv("HOME");
		if (home.size() == 0) {
			std::cerr << "failed to get configuration file path, "
				  << "$HOME not set." << std::endl;
			stop(write_fd, "error", nullptr);
		}
		config_file = home + "/.pekwm/config";
	}
	size_t sep = config_file.rfind('/');
	if (sep != std::string::npos) {
		setenv("PEKWM_CONFIG_PATH", config_file.substr(0, sep).c_str(), 1);
	}

	USER_INFO("Starting pekwm. Use this information in bug reports: "
		  << FEATURES << std::endl
		  << "using configuration at " << config_file);

	WindowManager *wm = WindowManager::start(config_file, replace, synchronous);
	if (wm) {
		try {
			P_TRACE("Enter event loop.");

			wm->doEventLoop();

			if (wm->shallRestart()) {
				std::string command = wm->getRestartCommand();
				stop(write_fd, "restart " + command, wm);
			}

			stop(write_fd, "stop", wm);
		} catch (StopException& ex) {
			stop(write_fd, ex.getMsg(), wm);
		} catch (std::string& ex) {
			P_ERR("unexpected error occurred: " << ex);
			stop(write_fd, "error", wm);
		}
	} else {
		stop(write_fd, "stop", wm);
	}

	return 0;
}
