//
// main.cc for pekwm
// Copyright (C) 2002-2003 Claes Nasten <pekdon@gmx.net>
//
// main.cc for aewm++
// Copyright (C) 2000 Frank Hale <frankhale@yahoo.com>
// http://sapphire.sourceforge.net/
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"
#include "WindowManager.hh"
#include "Util.hh"

#include <iostream>
#include <string>
#include <cstring>

using std::cout;
using std::endl;
using std::string;

namespace Info {

void
printVersion(void)
{
	cout << "pekwm: version " << VERSION << EXTRA_VERSION_INFO << endl;
}

void
printUsage(void)
{
	printVersion();
	cout << " --help       show this info." << endl;
	cout << " --version    show version info" << endl;
	cout << " --info       extended info. Use for bug reports." << endl;
	cout << " --display    display to connect to" << endl;
}

void printInfo(void)
{
	printVersion();
	cout << "Built with these options:" << endl;
	cout << CONFIG_OPTS << endl << endl;
}

}; // end namespace Info

int
main(int argc, char *argv[])
{
	string command_line;

	for (int i = 0; i < argc; ++i)
		command_line = command_line + argv[i] + " ";

	// Get the args and test for different options
	for (int i = 1; i < argc; ++i)	{
		if (!strcmp("--display", argv[i]) && ((i + 1) < argc)) {
			setenv("DISPLAY", argv[++i], 1);
		} else if (!strcmp("--version", argv[i])) {
			Info::printVersion();
			exit(0);
		} else if (!strcmp("--info", argv[i])) {
			Info::printInfo();
			exit(0);
		} else if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
			Info::printUsage();
			exit(0);
		}
	}

#ifdef DEBUG
	cout << "Starting pekwm. Use this information in bug reports:" << endl;
	Info::printInfo();
#endif

	WindowManager pekwm(command_line);

	return 0;
}
