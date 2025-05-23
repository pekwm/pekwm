//
// test_Calendar.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Calendar.hh"

class TestCalendar : public TestSuite {
public:
	TestCalendar();
	~TestCalendar();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testToString();
	static void testNextDay();
	static void testIsLeapYear();
};

TestCalendar::TestCalendar(void)
	: TestSuite("Calendar")
{
}

TestCalendar::~TestCalendar(void)
{
}

bool
TestCalendar::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "toString", testToString());
	TEST_FN(spec, "nextDay", testNextDay());
	TEST_FN(spec, "isLeapYear", testIsLeapYear());
	return status;
}

void
TestCalendar::testNextDay()
{
	Calendar c1(1737011287);
	ASSERT_EQUAL("nextDay", 16, c1.getMDay());
	Calendar c2 = c1.nextDay();
	ASSERT_EQUAL("nextDay", "2025-01-17T00:00:00Z", c2.toString());
	ASSERT_EQUAL("nextDay", 17, c2.getMDay());
}

void
TestCalendar::testToString()
{
	Calendar c1(1737011287);
	ASSERT_EQUAL("toString", "2025-01-16T07:08:07Z", c1.toString());
}

void
TestCalendar::testIsLeapYear()
{
	Calendar c1992(701046000);
	ASSERT_TRUE("1992", c1992.isLeapYear());
	Calendar c1993(732582000);
	ASSERT_FALSE("1993", c1993.isLeapYear());
	Calendar c2000(953506800);
	ASSERT_TRUE("2000", c2000.isLeapYear());
	Calendar c2001(985042800);
	ASSERT_FALSE("2001", c2001.isLeapYear());
}
