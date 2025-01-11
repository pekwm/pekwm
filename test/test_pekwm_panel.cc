//
// test_pekwm_panel.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"

#include "test_ExternalCommandData.hh"

#include "pekwm.hh"
#include "Debug.hh"
#include "X11.hh"

static int
main_tests(int argc, char *argv[])
{
	// Setup environment required for the tests
	Debug::setLogFile("/dev/null");
	X11::addHead(Head(0, 0, 800, 600));

	TestExternalCommandData externalCommandData;

	return TestSuite::main(argc, argv);
}

int
main(int argc, char *argv[])
{
	pekwm::initNoDisplay();
	int res = main_tests(argc, argv);
	pekwm::cleanupNoDisplay();

	return res;
}
