//
// test_pekwm_sys.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "Charset.hh"
#include "test.hh"

#include "test_SysConfig.hh"
#include "test_SysResources.hh"
#include "test_XSettings.hh"

int
main(int argc, char *argv[])
{
	Charset::WithCharset charset;

	TestSysConfig testSysConfig;
	TestSysResources testSysResources;
	TestXSettings testXSettings;

	return TestSuite::main(argc, argv);
}
