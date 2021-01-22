//
// test_CfgParser.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "CfgParser.hh"

class TestCfgParser : public TestSuite,
                      public CfgParser {
public:
    TestCfgParser()
        : TestSuite("CfgParser"),
          CfgParser()
    {
        register_test("INCLUDE", std::bind(&TestCfgParser::testInclude, this));
    }

    void testInclude(void) {
        auto cfg =
            "INCLUDE = \"../test/data/cfg_parser_include.cfg\"";
        auto source = new CfgParserSourceString(":memory:", cfg);
        ASSERT_EQUAL("parse ok", true, parse(source));
        ASSERT_EQUAL("var in include", "value", getVar("$VAR"));
    }
};

int
main(int argc, char *argv[])
{
    TestCfgParser testCfgParser;
    TestSuite::main(argc, argv);
}

