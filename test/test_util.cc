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

#include "test_CfgParser.hh"
#include "test_Charset.hh"
#include "test_Geometry.hh"
#include "test_Json.hh"
#include "test_RegexString.hh"
#include "test_String.hh"
#include "test_Tokenizer.hh"
#include "test_Util.hh"

int
main(int argc, char *argv[])
{
	Charset::WithCharset charset;

	TestCfgParser testCfgParser;
	TestCharset testCharset;
	TestGeometry testGeometry;
	TestGeometryOverlap testGeometryOverlap;
	TestJson testJson;
	TestRegexString testRegexString;
	TestString testString;
	TestTokenizer testTokenizer;
	TestUtf8Iterator testUtf8Iterator;

	// Util
	TestGenerator testGenerator;
	TestUtil testUtil;
	TestOsEnv testOsEnv;

	return TestSuite::main(argc, argv);
}
