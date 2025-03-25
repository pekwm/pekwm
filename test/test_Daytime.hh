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

	// 2024-12-31T00:00:00 (rise 09:04:09 set 12:31:27)
	for (time_t t = 1735603200; t < 1735516800; t += 10800) {
		Daytime dt3(t, 65.5191, 18.8572);
		ASSERT_EQUAL("daytime (64.5191, 18.8572)", 0,
			     1735635849 - dt3.getSunRise());
		ASSERT_EQUAL("daytime (64.5191, 18.8572)", 0,
			     1735648407 - dt3.getSunSet());
	}

	// 1993-01-01
	Daytime dt4(725846400, 65.5191, 18.8572);
	ASSERT_EQUAL("daytime 1993-01-01 (65.5191, 18.8572)", 0,
		     725878939 - dt4.getSunRise());
	ASSERT_EQUAL("daytime 1993-01-01 (65.5191, 18.8572)", 0,
		     725891789 - dt4.getSunSet());
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
