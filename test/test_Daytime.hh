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
	Daytime dt(1736588028, 56.0449, 12.692);
	ASSERT_EQUAL("daytime", 1736580961, dt.getSunRise());
	ASSERT_EQUAL("daytime", 1736607626, dt.getSunSet());
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
