//
// test_CfgParser.hh for pekwm
// Copyright (C) 2021-2023 Claes Nästén <pekdon@gmail.com>
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

	// command
	void testCommandOk();
	void testCommandMissing();

	// variable parsing
	void testParseVarEol();
	void testParseEscapeAtEndVar();
	void testParseUnderscoreVar();
	void testParseAtVar();
	void testParseAmpVar();
	void testParseCurlyVar();
	void testParseCurlyNotClosedVar();
};

TestCfgParser::TestCfgParser(void)
	: TestSuite("CfgParser"),
	  CfgParser("../test/data")
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
	CfgParserSource *source =
		new CfgParserSourceString(":memory:", cfg);

	clear();
	ASSERT_EQUAL("parse ok", true, parse(source));
	std::string var = getVar("VAR");
	ASSERT_EQUAL("var in include", "value", var);
}

void
TestCfgParser::testCommandOk()
{
	const char *cfg = "COMMAND = \"cfg_parser_command.sh\"";
	CfgParserSource *source =
		new CfgParserSourceString(":memory:", cfg);

	clear();
	ASSERT_EQUAL("parse ok", true, parse(source));
	CfgParser::Entry *entry = getEntryRoot()->findEntry("KEY");
	ASSERT_EQUAL("entry", true, entry != nullptr);
	ASSERT_EQUAL("value", "value", entry->getValue());

}

void
TestCfgParser::testCommandMissing()
{
	const char *cfg = "COMMAND = \"cfg_parser_missing.sh\"";
	CfgParserSource *source =
		new CfgParserSourceString(":memory:", cfg);

	clear();
	// parse succeeds even if COMMAND fails to execute (legacy behavior)
	ASSERT_EQUAL("parse ok", true, parse(source));
}

void
TestCfgParser::testParseVarEol()
{
	std::string line("$name"), var;
	std::string::size_type begin = 0, end = 0;
	bool res = parseVarName(line, begin, end, var);
	ASSERT_EQUAL("should succeeed", true, res);
	ASSERT_EQUAL("begin", 0, begin);
	ASSERT_EQUAL("end", 5, end);
	ASSERT_EQUAL("var", "name", var);
}

void
TestCfgParser::testParseEscapeAtEndVar()
{
	std::string line("$nam\\"), var;
	std::string::size_type begin = 0, end = 0;
	bool res = parseVarName(line, begin, end, var);
	ASSERT_EQUAL("should fail", false, res);
	ASSERT_EQUAL("begin", 0, begin);
	ASSERT_EQUAL("end", 0, end);
	ASSERT_EQUAL("var", "", var);
}

void
TestCfgParser::testParseUnderscoreVar()
{
	std::string line("my $_ENV var"), var;
	std::string::size_type begin = 0, end = 0;
	bool res = parseVarName(line, begin, end, var);
	ASSERT_EQUAL("should succeeed", true, res);
	ASSERT_EQUAL("begin", 3, begin);
	ASSERT_EQUAL("end", 8, end);
	ASSERT_EQUAL("var", "_ENV", var);
}

void
TestCfgParser::testParseAtVar()
{
	std::string line("$@atom"), var;
	std::string::size_type begin = 0, end = 0;
	bool res = parseVarName(line, begin, end, var);
	ASSERT_EQUAL("should succeeed", true, res);
	ASSERT_EQUAL("begin", 0, begin);
	ASSERT_EQUAL("end", 6, end);
	ASSERT_EQUAL("var", "@atom", var);
}

void
TestCfgParser::testParseAmpVar()
{
	std::string line("$&resource"), var;
	std::string::size_type begin = 0, end = 0;
	bool res = parseVarName(line, begin, end, var);
	ASSERT_EQUAL("should succeeed", true, res);
	ASSERT_EQUAL("begin", 0, begin);
	ASSERT_EQUAL("end", 10, end);
	ASSERT_EQUAL("var", "&resource", var);
}

void
TestCfgParser::testParseCurlyVar()
{
	std::string line("my ${variable with space} here"), var;
	std::string::size_type begin = 0, end = 0;
	bool res = parseVarName(line, begin, end, var);
	ASSERT_EQUAL("should succeeed", true, res);
	ASSERT_EQUAL("begin", 3, begin);
	ASSERT_EQUAL("end", 24, end);
	ASSERT_EQUAL("var", "variable with space", var);
}

void
TestCfgParser::testParseCurlyNotClosedVar()
{
	std::string line("my ${variable"), var;
	std::string::size_type begin = 0, end = 0;
	bool res = parseVarName(line, begin, end, var);
	ASSERT_EQUAL("should fail", false, res);
	ASSERT_EQUAL("begin", 3, begin);
	ASSERT_EQUAL("end", 0, end);
	ASSERT_EQUAL("var", "", var);
}

bool
TestCfgParser::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "empty val", testEmptyVal());
	TEST_FN(spec, "INCLUDE without newline", testIncludeWithoutNewline());

	// command
	TEST_FN(spec, "COMMAND found", testCommandOk());
	TEST_FN(spec, "COMMAND missing", testCommandMissing());

	// variable parsing
	TEST_FN(spec, "$ var, to end of line", testParseVarEol());
	TEST_FN(spec, "$ var, escape at end", testParseEscapeAtEndVar());
	TEST_FN(spec, "$_ variable", testParseUnderscoreVar());
	TEST_FN(spec, "$@ variable", testParseAtVar());
	TEST_FN(spec, "$& variable", testParseAmpVar());
	TEST_FN(spec, "${} variable", testParseCurlyVar());
	TEST_FN(spec, "${ variable", testParseCurlyNotClosedVar());
	return status;
}
