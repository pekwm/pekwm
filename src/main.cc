//
// main.cc for pekwm
// Copyright (C) 2002 Claes Nasten <pekdon@gmx.net>
//
// main.cc for aewm++
// Copyright (C) 2000 Frank Hale
// frankhale@yahoo.com
// http://sapphire.sourceforge.net/
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
 
#include "windowmanager.hh"
#include "util.hh"

#include <iostream>
#include <string>
#include <cstring>

using std::cout;
using std::endl;
using std::string;

const char *VERSION = "0.1.1";
const char *EXTRA_VERSION_INFO = " Mon Oct 14 19:36:18 CEST 2002";

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
	cout << " --help       shows this info" << endl;
	cout << " --version    show version info" << endl;
	cout << " --display    display to connect to" << endl;
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
		} else if (!strcmp("--help", argv[i]) || !strcmp("-h", argv[i])) {
			Info::printUsage();
			exit(0);
		}
	}

	WindowManager pekwm(command_line);

	return 0;
}
