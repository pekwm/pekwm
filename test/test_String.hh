//
// test_String.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "String.hh"

class TestString : public TestSuite {
public:
	TestString(void)
		: TestSuite("String")
	{
	}
	virtual ~TestString(void)
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testStrStartsWith();
	static void testStrEndsWith();
	static void testStrHash();
	static void testAsciiTolower();
	static void testAsciiNCaseCmp();
	static void testAsciiNCaseEqual();
};

bool
TestString::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "str_starts_with", testStrStartsWith());
	TEST_FN(spec, "str_ends_with", testStrEndsWith());
	TEST_FN(spec, "str_hash", testStrHash());
	TEST_FN(spec, "ascii_tolower", testAsciiTolower());
	TEST_FN(spec, "ascii_ncase_cmp", testAsciiNCaseCmp());
	TEST_FN(spec, "ascii_ncase_equal", testAsciiNCaseEqual());
	return status;
}

void
TestString::testStrStartsWith()
{
	ASSERT_EQUAL("empty", false, pekwm::str_starts_with("", '\n'));
	ASSERT_EQUAL("not match", false, pekwm::str_starts_with("text", '\n'));
	ASSERT_EQUAL("match", true, pekwm::str_starts_with("\ntext", '\n'));

	ASSERT_EQUAL("empty", false, pekwm::str_starts_with("", "\\n"));
	ASSERT_EQUAL("not match", false,
		     pekwm::str_starts_with("text", "\\n"));
	ASSERT_EQUAL("match", true,
		     pekwm::str_starts_with("\\ntext", "\\n"));
}

void
TestString::testStrEndsWith()
{
	ASSERT_EQUAL("empty", false, pekwm::str_ends_with("", '\n'));
	ASSERT_EQUAL("not match", false, pekwm::str_ends_with("text", '\n'));
	ASSERT_EQUAL("match", true, pekwm::str_ends_with("text\n", '\n'));

	ASSERT_EQUAL("empty", false, pekwm::str_ends_with("", "\\n"));
	ASSERT_EQUAL("not match", false,
		     pekwm::str_ends_with("text", "\\n"));
	ASSERT_EQUAL("match", true,
		     pekwm::str_ends_with("text\\n", "\\n"));
}

void
TestString::testStrHash()
{
	uint cpp_hash = pekwm::str_hash(std::string("C++ string"));
	ASSERT_TRUE("std::string hash", cpp_hash != 0);
	uint c_hash = pekwm::str_hash("C string");
	ASSERT_TRUE("C string hash", c_hash != 0);
	ASSERT_TRUE("hash not equal", cpp_hash != c_hash);
}

void
TestString::testAsciiTolower()
{
	ASSERT_EQUAL("a", static_cast<int>('a'),
		     pekwm::ascii_tolower('a'));
	ASSERT_EQUAL("z", static_cast<int>('z'),
		     pekwm::ascii_tolower('z'));
	ASSERT_EQUAL("A", static_cast<int>('a'),
		     pekwm::ascii_tolower('A'));
	ASSERT_EQUAL("Z", static_cast<int>('z'),
		     pekwm::ascii_tolower('Z'));
	ASSERT_EQUAL("0", static_cast<int>('0'),
		     pekwm::ascii_tolower('0'));
	ASSERT_EQUAL("9", static_cast<int>('9'),
		     pekwm::ascii_tolower('9'));
}

void
TestString::testAsciiNCaseCmp()
{
	ASSERT_EQUAL("lower", -1,
		     pekwm::ascii_ncase_cmp("abc", "bcd"));
	ASSERT_EQUAL("UPPER", 2,
		     pekwm::ascii_ncase_cmp("CDE", "abc"));
	ASSERT_EQUAL("short lhs", -99,
		     pekwm::ascii_ncase_cmp("ab", "abc"));
	ASSERT_EQUAL("short rhs", 99,
		     pekwm::ascii_ncase_cmp("abc", "ab"));
}

void
TestString::testAsciiNCaseEqual()
{
	ASSERT_EQUAL("lower", true,
		     pekwm::ascii_ncase_equal("hello", "hello"));
	ASSERT_EQUAL("UPPER", true,
		     pekwm::ascii_ncase_equal("HELLO", "hello"));
	ASSERT_EQUAL("MiXeD", true,
		     pekwm::ascii_ncase_equal("MiXeD", "mixed"));
	ASSERT_EQUAL("short lhs", false,
		     pekwm::ascii_ncase_equal("test", "tes"));
	ASSERT_EQUAL("short rhs", false,
		     pekwm::ascii_ncase_equal("tes", "test"));
	ASSERT_EQUAL("empty", true,
		     pekwm::ascii_ncase_equal("", ""));
}
