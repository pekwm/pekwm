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
	PekwmSys sys(&os);
}
