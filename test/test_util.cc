//
// test_util.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"

#include "Debug.hh"

#include "test_CfgParser.hh"
#include "test_Charset.hh"
#include "test_RegexString.hh"
#include "test_Util.hh"

int
main(int argc, char *argv[])
{
    Charset::WithCharset charset;

    // CfgParser
    TestCfgParser testCfgParser;

    // Charset
    TestCharset testCharset;

    // RegexString
    TestRegexString testRegexString;

    // Util
    TestString testString;
    TestUtil testUtil;

    return TestSuite::main(argc, argv);
}
