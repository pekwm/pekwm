//
// test_Daytime.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Daytime.hh"

class TestDaytime : public TestSuite {
public:
	TestDaytime();
	~TestDaytime();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testDaytime();
	static void testGetTimeOfDay();
};

TestDaytime::TestDaytime()
	: TestSuite("Daytime")
{
}

TestDaytime::~TestDaytime()
{
}

bool
TestDaytime::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "daytime", testDaytime());
	TEST_FN(spec, "getTimeOfDay", testGetTimeOfDay());
	return status;
}

void
TestDaytime::testDaytime()
{
	Daytime dt1(1736588028, 56.0449, 12.692);
	ASSERT_TRUE("sunrise before time", 1736588028 > dt1.getSunRise());
	ASSERT_EQUAL("daytime (56.0449, 12.692)", 0,
		     1736580961 - dt1.getSunRise());
	ASSERT_TRUE("sunset after time", 1736588028 < dt1.getSunSet());
	ASSERT_EQUAL("daytime (56.0449, 12.692)", 0,
		     1736607626 - dt1.getSunSet());

	// 2025-03-23T13:53:40 (rise 05:31:22 set 18:12:49)
	Daytime dt2(1742734420, 65.5191, 18.8572);
	ASSERT_TRUE("sunrise before time", 1742734420 > dt2.getSunRise());
	ASSERT_EQUAL("daytime (64.5191, 18.8572)", 0,
		     1742704510 - dt2.getSunRise());
	ASSERT_TRUE("sunset after time", 1742734420 < dt2.getSunSet());
	ASSERT_EQUAL("daytime (64.5191, 18.8572)", 0,
		     1742749779 - dt2.getSunSet());
}

void
TestDaytime::testGetTimeOfDay()
{
	Daytime dt(1736588028, 56.0449, 12.692);
	ASSERT_EQUAL("night (before)", TIME_OF_DAY_NIGHT,
		     dt.getTimeOfDay(1736580960));
	ASSERT_EQUAL("night (after)", TIME_OF_DAY_NIGHT,
		     dt.getTimeOfDay(1736607627));
	ASSERT_EQUAL("day", TIME_OF_DAY_DAY,
		     dt.getTimeOfDay(1736580961));
}
