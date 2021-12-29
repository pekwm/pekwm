//
// test_Util.cc for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Util.hh"

class TestStringUtil : public TestSuite {
public:
	TestStringUtil()
		: TestSuite("StringUtil")
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testAsciiTolower(void);
	static void testAsciiNCaseCmp(void);
	static void testAsciiNCaseEqual(void);
	static void testShellSplit(void);
	static void assertShellSplit(std::string msg, std::string str,
				     std::vector<std::string> &expected);
};

bool
TestStringUtil::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "ascii_tolower", testAsciiTolower());
	TEST_FN(spec, "ascii_ncase_cmp", testAsciiNCaseCmp());
	TEST_FN(spec, "ascii_ncase_equal", testAsciiNCaseEqual());
	TEST_FN(spec, "shell_split", testShellSplit());
	return status;
}

void
TestStringUtil::testAsciiTolower(void)
{
	ASSERT_EQUAL("a", static_cast<int>('a'),
		     StringUtil::ascii_tolower('a'));
	ASSERT_EQUAL("z", static_cast<int>('z'),
		     StringUtil::ascii_tolower('z'));
	ASSERT_EQUAL("A", static_cast<int>('a'),
		     StringUtil::ascii_tolower('A'));
	ASSERT_EQUAL("Z", static_cast<int>('z'),
		     StringUtil::ascii_tolower('Z'));
	ASSERT_EQUAL("0", static_cast<int>('0'),
		     StringUtil::ascii_tolower('0'));
	ASSERT_EQUAL("9", static_cast<int>('9'),
		     StringUtil::ascii_tolower('9'));
}

void
TestStringUtil::testAsciiNCaseCmp(void)
{
	ASSERT_EQUAL("lower", -1,
		     StringUtil::ascii_ncase_cmp("abc", "bcd"));
	ASSERT_EQUAL("UPPER", 2,
		     StringUtil::ascii_ncase_cmp("CDE", "abc"));
	ASSERT_EQUAL("short lhs", -99,
		     StringUtil::ascii_ncase_cmp("ab", "abc"));
	ASSERT_EQUAL("short rhs", 99,
		     StringUtil::ascii_ncase_cmp("abc", "ab"));
}

void
TestStringUtil::testAsciiNCaseEqual(void)
{
	ASSERT_EQUAL("lower", true,
		     StringUtil::ascii_ncase_equal("hello", "hello"));
	ASSERT_EQUAL("UPPER", true,
		     StringUtil::ascii_ncase_equal("HELLO", "hello"));
	ASSERT_EQUAL("MiXeD", true,
		     StringUtil::ascii_ncase_equal("MiXeD", "mixed"));
	ASSERT_EQUAL("short lhs", false,
		     StringUtil::ascii_ncase_equal("test", "tes"));
	ASSERT_EQUAL("short rhs", false,
		     StringUtil::ascii_ncase_equal("tes", "test"));
	ASSERT_EQUAL("empty", true,
		     StringUtil::ascii_ncase_equal("", ""));
}

void
TestStringUtil::testShellSplit(void)
{
	std::vector<std::string> basic_e;
	basic_e.push_back("one");
	basic_e.push_back("two");
	basic_e.push_back("three");
	assertShellSplit("basic", "one two three", basic_e);

	std::vector<std::string> basic_skip_space;
	basic_skip_space.push_back("1");
	basic_skip_space.push_back("2");
	basic_skip_space.push_back("3");
	assertShellSplit("skip space", "1   2\t\t\t3 \t", basic_skip_space);

	std::vector<std::string> basic_squote;
	basic_squote.push_back("o n e");
	basic_squote.push_back("");
	basic_squote.push_back("three");
	assertShellSplit("single quote", "'o n e' '' three", basic_squote);

	std::vector<std::string> basic_dquote;
	basic_dquote.push_back("one");
	basic_dquote.push_back("t w o '' ");
	basic_dquote.push_back("three");
	assertShellSplit("double quote", "one \"t w o '' \" three",
			 basic_dquote);
}

void
TestStringUtil::assertShellSplit(std::string msg, std::string str,
                             std::vector<std::string> &expected)
{
	std::vector<std::string> res = StringUtil::shell_split(str);
	ASSERT_EQUAL(msg + " size", expected.size(), res.size());
	for (uint i = 0; i < expected.size(); i++) {
		std::string is =  std::to_string(i);
		ASSERT_EQUAL(msg + " res[" + is + "]",
			     expected[i], res[i]);
	}
}

class TestUtil : public TestSuite {
public:
	TestUtil()
		: TestSuite("Util")
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testSplitString(void);
	static void assertSplitString(std::string msg,
				      uint e_ret, std::vector<std::string> e_toks,
				      const std::string str, const char *sep,
				      uint max = 0,
				      bool include_empty = false,
				      char escape = '\\');
};

bool
TestUtil::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "splitString", testSplitString());
	return status;
}

void
TestUtil::testSplitString(void)
{
	assertSplitString("empty string", 0, std::vector<std::string>(),
			  "", ",");

	std::vector<std::string> no_limit;
	no_limit.push_back("1");
	no_limit.push_back("2");
	no_limit.push_back("3");
	assertSplitString("no limit", 3, no_limit, "1,2,3", ",");
}

void
TestUtil::assertSplitString(std::string msg,
                            uint e_ret, std::vector<std::string> e_toks,
                            const std::string str, const char *sep,
                            uint max, bool include_empty, char escape)
{
	std::vector<std::string> toks;
	uint ret = Util::splitString(str, toks, sep, max,
				     include_empty, escape);
	ASSERT_EQUAL(msg + " ret", e_ret, ret);
	ASSERT_EQUAL(msg + " tokens", e_toks.size(), toks.size());
	for (uint i = 0; i < e_toks.size(); i++) {
		std::string is = std::to_string(i);
		ASSERT_EQUAL(msg + " tok " + is, e_toks[i], toks[i]);
	}
}
