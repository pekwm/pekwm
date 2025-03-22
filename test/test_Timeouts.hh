//
// test_Timeouts.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Timeouts.hh"

extern "C" {
#include <unistd.h>
};

class TestTimeouts : public TestSuite {
public:
	TestTimeouts(void);
	virtual ~TestTimeouts(void);

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testTimeoutMsToTimespec();
	static void testTimespecDiffToTimeval();
	static void testGetNextTimeout();
};

TestTimeouts::TestTimeouts(void)
	: TestSuite("Timeouts")
{
}

TestTimeouts::~TestTimeouts(void)
{
}

bool
TestTimeouts::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "timeout_ms_to_timespec", testTimeoutMsToTimespec());
	TEST_FN(spec, "timespec_diff_to_timeval", testTimespecDiffToTimeval());
	TEST_FN(spec, "getNextTimeout", testTimeoutMsToTimespec());
	return status;
}

void
TestTimeouts::testTimeoutMsToTimespec()
{
	struct timespec ts;
	ts.tv_sec = 10;
	ts.tv_nsec = 0;
	timeout_ms_to_timespec(ts, 2500, false);
	ASSERT_EQUAL("no wrap", 12, ts.tv_sec);
	ASSERT_EQUAL("no wrap", 500000000, ts.tv_nsec);

	ts.tv_sec = 10;
	ts.tv_nsec = 615000000;
	timeout_ms_to_timespec(ts, 2500, false);
	ASSERT_EQUAL("wrap", 13, ts.tv_sec);
	ASSERT_EQUAL("wrap", 115000000, ts.tv_nsec);
}

void
TestTimeouts::testTimespecDiffToTimeval()
{
	struct timespec ts;
	struct timespec ts_end;
	struct timeval tv;

	ts.tv_sec = 10;
	ts.tv_nsec = 0;
	ts_end.tv_sec = 11;
	ts_end.tv_nsec = 120000;
	timespec_diff_to_timeval(ts, ts_end, tv);
	ASSERT_EQUAL("no wrap", 1, tv.tv_sec);
	ASSERT_EQUAL("no wrap", 120, tv.tv_usec);

	ts.tv_sec = 10;
	ts.tv_nsec = 900000000LL;
	ts_end.tv_nsec = 240000;
	timespec_diff_to_timeval(ts, ts_end, tv);
	ASSERT_EQUAL("wrap", 0, tv.tv_sec);
	ASSERT_EQUAL("wrap", 100240, tv.tv_usec);
}

void
TestTimeouts::testGetNextTimeout()
{
	Timeouts timeouts;
	struct timeval *tv;
	TimeoutAction action;
	ASSERT_EQUAL("no actions", false,
		     timeouts.getNextTimeout(&tv, action));
	ASSERT_EQUAL("no action", static_cast<struct timeval*>(nullptr), tv);
	ASSERT_EQUAL("no action", -1, action.getKey());

	timeouts.add(TimeoutAction(1, 5));
	timeouts.add(TimeoutAction(2, 1));
	timeouts.add(TimeoutAction(3, 5000));

	usleep(1100);
	ASSERT_EQUAL("action", true,
		     timeouts.getNextTimeout(&tv, action));
	ASSERT_EQUAL("action", 2, action.getKey());

	usleep(4100);
	ASSERT_EQUAL("action", true,
		     timeouts.getNextTimeout(&tv, action));
	ASSERT_EQUAL("action", 1, action.getKey());

	ASSERT_EQUAL("timeval", false,
		     timeouts.getNextTimeout(&tv, action));
	ASSERT_TRUE("timeval", action.getKey() != 3);
}
