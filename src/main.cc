//
// main.cc for pekwm
// Copyright © 2003-2009 Claes Nästén <me@pekdon.net>
//
// main.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

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

extern "C" {
#include <unistd.h> // execlp
#include <locale.h>
}

using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::locale;

namespace Info {

//! @brief Prints version
void
printVersion(void)
{
    cout << "pekwm: version " << VERSION << EXTRA_VERSION_INFO << endl;
}

//! @brief Prints version and availible options
void
printUsage(void)
{
    printVersion();
    cout << " --help       show this info." << endl;
    cout << " --version    show version info" << endl;
    cout << " --info       extended info. Use for bug reports." << endl;
    cout << " --display    display to connect to" << endl;
    cout << " --config     alternative config file" << endl;
    cout << " --replace    replace running window manager" << endl;
}

//! @brief Prints version and build-time options
void
printInfo(void)
{
    printVersion();
    cout << "features: " << FEATURES << endl;
}

} // end namespace Info

//! @brief Main function of pekwm
int
main(int argc, char **argv)
{
    string config_file;
    bool replace = false;

    // Set LC_CTYPE before initializing iconv, used locale::global
    // here did not behave properly under FreeBSD 9.1
    char *locale = setlocale(LC_CTYPE, "");
    if (locale == NULL) {
        ERR("The environment variables specify an unknown locale - "
            "falling back to default \"C\". This is not a bug in PekWM.");
        setlocale(LC_CTYPE, "C");
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
        } else if (strcmp("--help", argv[i]) || ! strcmp("-h", argv[i]) == 0) {
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
            config_file = string(getenv("HOME")) + string("/.pekwm/config");
        }
    }

#ifdef DEBUG
    cout << "Starting pekwm. Use this information in bug reports:" << endl;
    Info::printInfo();
#endif // DEBUG

    WindowManager *wm = WindowManager::start(config_file, replace);

    if (wm) {
        try {
            wm->doEventLoop();

            // see if we wanted to restart
            if (WindowManager::instance()->shallRestart()) {
                string command = WindowManager::instance()->getRestartCommand();

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
            cerr << "exception occurred: " << ex.what() << endl;
        } catch (string& ex) {
            cerr << "unexpected error occured: " << ex << endl;
        }
        WindowManager::destroy();
    }

    // Cleanup
    Util::iconv_deinit();

    return 0;
}
