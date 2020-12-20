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

//! @brief Prints version
void
printVersion(void)
{
    std::cout << "pekwm: version " << VERSION << std::endl;
}

//! @brief Prints version and availible options
void
printUsage(void)
{
    printVersion();
    std::cout << " --help       show this info." << std::endl;
    std::cout << " --version    show version info" << std::endl;
    std::cout << " --info       extended info. Use for bug reports." << std::endl;
    std::cout << " --display    display to connect to" << std::endl;
    std::cout << " --config     alternative config file" << std::endl;
    std::cout << " --replace    replace running window manager" << std::endl;
}

//! @brief Prints version and build-time options
void
printInfo(void)
{
    printVersion();
    std::cout << "features: " << FEATURES << std::endl;
}

} // end namespace Info

//! @brief Main function of pekwm
int
main(int argc, char **argv)
{
    std::string config_file;
    bool replace = false;

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
        if (getenv("PEKWM_CONFIG_FILE") && strlen(getenv("PEKWM_CONFIG_FILE"))) {
            config_file = getenv("PEKWM_CONFIG_FILE");
        } else {
            config_file = std::string(getenv("HOME")) + "/.pekwm/config";
        }
    }

#ifdef DEBUG
    std::cout << "Starting pekwm. Use this information in bug reports:" << std::endl;
    Info::printInfo();
#endif // DEBUG

    WindowManager *wm = WindowManager::start(config_file, replace);

    if (wm) {
        try {
            wm->doEventLoop();

            // see if we wanted to restart
            if (WindowManager::instance()->shallRestart()) {
                std::string command = WindowManager::instance()->getRestartCommand();

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
