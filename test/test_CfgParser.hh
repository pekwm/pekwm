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
    TestCfgParser(void);
    virtual ~TestCfgParser(void);

    void testEmptyVal(void) {
        auto cfg = "Section {\n    Key = \"\"\n}";
        auto source = new CfgParserSourceString(":memory:", cfg);

        clear();
        ASSERT_EQUAL("parse ok", true, parse(source));
        auto section = getEntryRoot()->findSection("SECTION");
        ASSERT_EQUAL("section", true, section != nullptr);
        auto entry = section->findEntry("KEY");
        ASSERT_EQUAL("entry", true, entry != nullptr);
        ASSERT_EQUAL("value", "", entry->getValue());
    }

    void testIncludeWithoutNewline(void) {
        auto cfg =
            "INCLUDE = \"../test/data/cfg_parser_include.cfg\"";
        auto source = new CfgParserSourceString(":memory:", cfg);

        clear();
        ASSERT_EQUAL("parse ok", true, parse(source));
        auto var = getVar("$VAR");
        ASSERT_EQUAL("var in include", "value", var);
    }
};

TestCfgParser::TestCfgParser(void)
    : TestSuite("CfgParser"),
      CfgParser()
{
    register_test("empty val",
                  std::bind(&TestCfgParser::testEmptyVal, this));
    register_test("INCLUDE without newline",
                  std::bind(&TestCfgParser::testIncludeWithoutNewline,
                            this));
}

TestCfgParser::~TestCfgParser(void)
{
}
