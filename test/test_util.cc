//
// test_util.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "test.hh"
#include "Debug.hh"

#include "test_Calendar.hh"
#include "test_CfgParser.hh"
#include "test_Charset.hh"
#include "test_Daytime.hh"
#include "test_Geometry.hh"
#include "test_Json.hh"
#include "test_Location.hh"
#include "test_Mem.hh"
#include "test_Os.hh"
#include "test_RegexString.hh"
#include "test_String.hh"
#include "test_Timeouts.hh"
#include "test_Tokenizer.hh"
#include "test_Util.hh"

int
main(int argc, char *argv[])
{
	Charset::WithCharset charset;

	TestCalendar testCalendar;
	TestCfgParser testCfgParser;
	TestCharset testCharset;
	TestDaytime testDaytime;
	TestGeometry testGeometry;
	TestGeometryOverlap testGeometryOverlap;
	TestJson testJson;
	TestLocation testLocation;
	TestMem testMem;
	TestRegexString testRegexString;
	TestString testString;
	TestTimeouts testTimeouts;
	TestTokenizer testTokenizer;
	TestUtf8Iterator testUtf8Iterator;

	// Util
	TestGenerator testGenerator;
	TestUtil testUtil;
	TestOsEnv testOsEnv;

	return TestSuite::main(argc, argv);
}
