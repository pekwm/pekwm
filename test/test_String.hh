//
// test_String.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
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

	static void testAsciiTolower(void);
	static void testAsciiNCaseCmp(void);
	static void testAsciiNCaseEqual(void);
};

bool
TestString::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "ascii_tolower", testAsciiTolower());
	TEST_FN(spec, "ascii_ncase_cmp", testAsciiNCaseCmp());
	TEST_FN(spec, "ascii_ncase_equal", testAsciiNCaseEqual());
	return status;
}

void
TestString::testAsciiTolower(void)
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
TestString::testAsciiNCaseCmp(void)
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
TestString::testAsciiNCaseEqual(void)
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
