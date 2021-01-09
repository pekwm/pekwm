//
// main.cc for pekwm
// Copyright (C) 2003-2020 Claes Nästén <pekdon@gmail.com>
//
// main.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include "Debug.hh"
#include "PWinObj.hh"
#include "PDecor.hh"
#include "Client.hh"
#include "Compat.hh"
#include "Frame.hh"
#include "WindowManager.hh"
#include "Util.hh"

#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <locale>

extern "C" {
#include <unistd.h> // execlp
#include <locale.h>
}

namespace Info {

    /**
     * Prints version
     */
    void
    printVersion(void)
    {
        std::cout << "pekwm: version " << VERSION << std::endl;
    }

    /**
     * Prints version and availible options
     */
    void
    printUsage(void)
    {
        printVersion();
        std::cout
            << " --help     show this info." << std::endl
            << " --version  show version info" << std::endl
            << " --info     extended info. Use for bug reports." << std::endl
            << " --display  display to connect to" << std::endl
            << " --config   alternative config file" << std::endl
            << " --replace  replace running window manager" << std::endl;
    }

    /**
     * Prints version and build-time options
     */
    void
    printInfo(void)
    {
        printVersion();
        std::cout << "features: " << FEATURES << std::endl;
    }

} // end namespace Info

/**
 * Main function of pekwm
 */
int
main(int argc, char **argv)
{
    try {
        std::locale::global(std::locale(""));
    } catch (const std::runtime_error &e) {
        ERR("The environment variables specify an unknown C++ locale - "
            "falling back to C's setlocale().");
        setlocale(LC_ALL, "");
    }

    Util::iconv_init();

    setenv("PEKWM_ETC_PATH", SYSCONFDIR, 1);
    setenv("PEKWM_SCRIPT_PATH", DATADIR "/pekwm/scripts", 1);
    setenv("PEKWM_THEME_PATH", DATADIR "/pekwm/themes", 1);

    // get the args and test for different options
    std::string config_file;
    bool replace = false;
    for (int i = 1; i < argc; ++i)	{
        if ((strcmp("--display", argv[i]) == 0) && ((i + 1) < argc)) {
            setenv("DISPLAY", argv[++i], 1);
        } else if ((strcmp("--config", argv[i]) == 0) && ((i + 1) < argc)) {
            config_file = argv[++i];
        } else if (strcmp("--replace", argv[i]) == 0) {
            replace = true;
        } else if (strcmp("--version", argv[i]) == 0) {
            Info::printVersion();
            exit(0);
        } else if (strcmp("--info", argv[i]) == 0) {
            Info::printInfo();
            exit(0);
        } else {
            Info::printUsage();
            exit(0);
        }
    }

    // Get configuration file if none was specified as a parameter,
    // default to reading environment, if not set get ~/.pekwm/config
    if (config_file.size() == 0) {
        config_file = Util::getEnv("PEKWM_CONFIG_FILE");
        if (config_file.size() == 0) {
            auto home = Util::getEnv("HOME");
            if (home.size() == 0) {
                std::cerr << "failed to get configuration file path, "
                          << "$HOME not set." << std::endl;
                exit(1);
            }
            config_file = home + "/.pekwm/config";
        }
    }

#ifdef DEBUG
    std::cout << "Starting pekwm. Use this information in bug reports:"
              << std::endl;
    Info::printInfo();
    std::cout << "using configuration at " << config_file << std::endl;
#endif // DEBUG

    auto wm = WindowManager::start(config_file, replace);

    if (wm) {
        try {
#ifdef DEBUG
            std::cout << "Enter event loop." << std::endl;
#endif // DEBUG
            wm->doEventLoop();

            // see if we wanted to restart
            if (WindowManager::instance()->shallRestart()) {
                auto command = WindowManager::instance()->getRestartCommand();

                // cleanup before restarting
                WindowManager::destroy();
                Util::iconv_deinit();

                if (command.empty()) {
                    execvp(argv[0], argv);
                } else {
                    command = "exec " + command;
                    execl("/bin/sh", "sh" , "-c", command.c_str(), (char*) 0);
                }
            }
        } catch (std::exception& ex) {
            std::cerr << "exception occurred: " << ex.what() << std::endl;
        } catch (std::string& ex) {
            std::cerr << "unexpected error occurred: " << ex << std::endl;
        }
        WindowManager::destroy();
    }

    // Cleanup
    Util::iconv_deinit();

    return 0;
}
