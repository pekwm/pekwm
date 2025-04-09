//
// pekwm_wm.cc for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Charset.hh"
#include "Compat.hh"
#include "Debug.hh"
#include "Exception.hh"
#include "WindowManager.hh"
#include "Util.hh"
#include "pekwm_env.hh"

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
	std::cout << " --skip-start do not run the start file" << std::endl;
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
	Util::setEnv("PEKWM_ETC_PATH", SYSCONFDIR);
	Util::setEnv("PEKWM_SCRIPT_PATH", DATADIR "/pekwm/scripts");
	Util::setEnv("PEKWM_THEME_PATH", DATADIR "/pekwm/themes");
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
	// Initial pass of limited access (on OpenBSD), includes prot_exec
	// and networking to support initializing the X11 connection.
	//
	// It will be further limited after the connection has been made.
	pledge_x11_required("fattr chown");

	Charset::init();
	initEnv(false);

	// get the args and test for different options
	bool skip_start = false;
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
		} else if (strcmp("--log-level", argv[i]) == 0
			   && ((i + 1) < argc)) {
			Debug::setLevel(Debug::getLevel(argv[++i]));
		} else if (strcmp("--log-file", argv[i]) == 0
			   && ((i + 1) < argc)) {
			if (! Debug::setLogFile(argv[++i])) {
				std::cerr << "Failed to open log file "
					  << argv[i] << std::endl;
			}
		} else if (strcmp("--replace", argv[i]) == 0) {
			replace = true;
		} else if (strcmp("--skip-start", argv[i]) == 0) {
			skip_start = true;
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
			Util::setEnv("DISPLAY", argv[++i]);
		} else if (standalone && strcmp("--config", argv[i]) == 0
			   && (i + 1) < argc) {
			Util::setEnv("PEKWM_CONFIG_FILE", argv[++i]);
		} else {
			printUsage();
			stop(write_fd, "stop", nullptr);
		}
	}

	// Get configuration file if none was specified as a parameter,
	// default to reading environment, if not set get ~/.pekwm/config
	std::string config_file = Util::getEnv("PEKWM_CONFIG_FILE");
	if (config_file.size() == 0) {
		std::string cfg_dir = Util::getConfigDir();
		if (cfg_dir.size() == 0) {
			std::cerr << "failed to get configuration file path, "
				  << "none of $HOME and $PEKWM_CONFIG_PATH "
				  << "is set." << std::endl;
			stop(write_fd, "error", nullptr);
		}
		config_file = cfg_dir + "/config";
	}
	std::string config_path = Util::getDir(config_file);
	if (config_path.size() != 0) {
		Util::setEnv("PEKWM_CONFIG_PATH", config_path);
	}

	USER_INFO("Starting pekwm. Use this information in bug reports: "
		  << FEATURES << std::endl
		  << "using configuration at " << config_file);

	WindowManager *wm =
		WindowManager::start(config_file, replace, skip_start,
				     synchronous, standalone);

	// Further limit access (on OpenBSD) after the X11 connection has
	// been setup.
	pledge_x("stdio rpath wpath cpath proc exec", NULL);

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
