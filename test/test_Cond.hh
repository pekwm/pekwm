//
// test_Cond.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Cond.hh"

class TestCond : public TestSuite {
public:
	TestCond();
	~TestCond();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testEmpty();
	static void testEqStr();
	static void testEqNum();
	static void testNotEqStr();
	static void testNotEqNum();
	static void testGt();
	static void testLt();
};

TestCond::TestCond()
	: TestSuite("Cond")
{
}

TestCond::~TestCond()
{
}

bool
TestCond::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "empty", testEmpty());
	TEST_FN(spec, "EqStr", testEqStr());
	TEST_FN(spec, "EqNum", testEqNum());
	TEST_FN(spec, "NotEqStr", testNotEqStr());
	TEST_FN(spec, "NotEqNum", testNotEqNum());
	TEST_FN(spec, "Gt", testGt());
	TEST_FN(spec, "Lt", testLt());
	return status;
}

void
TestCond::testEmpty()
{
	ASSERT_TRUE("empty", Cond::eval(""));
}

void
TestCond::testEqStr()
{
	ASSERT_TRUE("\"hello world\" = \"hello world\"",
		    Cond::eval("\"hello world\" = \"hello world\""));
	ASSERT_FALSE("\"hello world\" = \"hello\"",
		    Cond::eval("\"hello world\" = \"hello\""));
}

void
TestCond::testEqNum()
{
	ASSERT_TRUE("100 = 100", Cond::eval("100 = 100"));
	ASSERT_FALSE("200 = 2", Cond::eval("200 = 2"));
}

void
TestCond::testNotEqStr()
{
	ASSERT_TRUE("\"hello world\" != \"hello\"",
		    Cond::eval("\"hello world\" != \"hello\""));
	ASSERT_FALSE("\"hello world\" != \"hello world\"",
		     Cond::eval("\"hello world\" != \"hello world\""));
}

void
TestCond::testNotEqNum()
{
	ASSERT_FALSE("100 != 100", Cond::eval("100 != 100"));
	ASSERT_TRUE("200 != 2", Cond::eval("200 != 2"));
}

void
TestCond::testGt()
{
	ASSERT_TRUE("3 > 2", Cond::eval("3 > 2"));
	ASSERT_TRUE("3.1 > 3", Cond::eval("3.1 > 3"));
	ASSERT_FALSE("112.1 > 112.1", Cond::eval("112.1 > 112.1"));
	// non numeric data, always false
	ASSERT_FALSE("\"hello\" > 1", Cond::eval("\"hello\" > 1"));
}

void
TestCond::testLt()
{
	ASSERT_TRUE("2 < 3", Cond::eval("2 < 3"));
	ASSERT_FALSE("3.1 < 3", Cond::eval("3.1 < 3"));
	ASSERT_FALSE("112.1 < 112.1", Cond::eval("112.1 < 112.1"));
	// non numeric data, always false
	ASSERT_FALSE("\"hello\" < 1", Cond::eval("\"hello\" < 1"));
}
