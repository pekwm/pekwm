//
// test_PekwmSys.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "test_Mock.hh"

#define UNITTEST
#include "sys/pekwm_sys.cc"

class TestPekwmSys : public TestSuite {
public:
	TestPekwmSys();
	virtual ~TestPekwmSys();

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testUpdateDaytime();
};

TestPekwmSys::TestPekwmSys()
	: TestSuite("PekwmSys")
{
}

TestPekwmSys::~TestPekwmSys()
{
}

bool
TestPekwmSys::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "updateDaytime", testUpdateDaytime());
	return status;
}

void
TestPekwmSys::testUpdateDaytime()
{
	OsMock os;
	PekwmSys sys("", false, &os);

	time_t now = time(NULL);
	time_t next_day = Calendar(now).nextDay().getTimestamp();
	time_t next_day_timeout = next_day - now - 1;
	struct timeval *tv;
	TimeoutAction action;

	ASSERT_EQUAL("default to day", TIME_OF_DAY_DAY, sys.updateDaytime(now));
	sys._timeouts.getNextTimeout(&tv, action);
	ASSERT_EQUAL("timeout next day", next_day_timeout, tv->tv_sec);

	sys._cfg.setLatitude(0.0);
	sys._cfg.setLongitude(0.0);
	sys._tod_override = TIME_OF_DAY_DUSK;
	ASSERT_EQUAL("use override", TIME_OF_DAY_DUSK, sys.updateDaytime(now));
	ASSERT_EQUAL("timeout next day", next_day_timeout, tv->tv_sec);

	sys._tod_override = static_cast<TimeOfDay>(-1);
	// based on current time, just know timeout is <= next day timeout
	ASSERT_EQUAL("timeout next day", next_day_timeout, tv->tv_sec);
}
