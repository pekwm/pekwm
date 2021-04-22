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

    virtual bool run_test(TestSpec spec, bool status);

    void testEmptyVal(void);
    void testIncludeWithoutNewline(void);
};

TestCfgParser::TestCfgParser(void)
    : TestSuite("CfgParser"),
      CfgParser()
{
}

TestCfgParser::~TestCfgParser(void)
{
}

void
TestCfgParser::testEmptyVal(void)
{
    const char *cfg = "Section {\n    Key = \"\"\n}";
    CfgParserSourceString *source =
      new CfgParserSourceString(":memory:", cfg);

    clear();
    ASSERT_EQUAL("parse ok", true, parse(source));
    CfgParser::Entry *section = getEntryRoot()->findSection("SECTION");
    ASSERT_EQUAL("section", true, section != nullptr);
    CfgParser::Entry *entry = section->findEntry("KEY");
    ASSERT_EQUAL("entry", true, entry != nullptr);
    ASSERT_EQUAL("value", "", entry->getValue());
}

void
TestCfgParser::testIncludeWithoutNewline(void)
{
    const char *cfg =
      "INCLUDE = \"../test/data/cfg_parser_include.cfg\"";
    CfgParserSourceString *source =
      new CfgParserSourceString(":memory:", cfg);

    clear();
    ASSERT_EQUAL("parse ok", true, parse(source));
    std::string var = getVar("$VAR");
    ASSERT_EQUAL("var in include", "value", var);
}

bool
TestCfgParser::run_test(TestSpec spec, bool status)
{
    TEST_FN(spec, "empty val", testEmptyVal());
    TEST_FN(spec, "INCLUDE without newline", testIncludeWithoutNewline());
    return status;
}
