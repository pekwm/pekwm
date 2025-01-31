//
// test_ManagerWindows.cc for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "wm/Config.hh"
#include "wm/ManagerWindows.hh"

class TestRootWO : public TestSuite,
		   public RootWO {
public:
	TestRootWO(HintWO *hint_wo, Config *cfg);
	virtual ~TestRootWO(void);

	virtual bool run_test(TestSpec spec, bool status);

	void testUpdateStrut(void);
};

TestRootWO::TestRootWO(HintWO *hint_wo, Config *cfg)
	: TestSuite("RootWO"),
	  RootWO(None, hint_wo, cfg)
{
}

TestRootWO::~TestRootWO(void)
{
}

bool
TestRootWO::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "update strut", testUpdateStrut());
	return status;
}

void
TestRootWO::testUpdateStrut(void)
{
	Strut empty(0, 0, 0, 0, 0);
	ASSERT_EQUAL("empty", empty, getStrut(0));

	Strut strut1(100, 0, 0, 0, 0);
	addStrut(&strut1);
	ASSERT_EQUAL("single", strut1, getStrut(0));

	Strut strut2(200, 0, 0, 50, 0);
	addStrut(&strut2);
	ASSERT_EQUAL("multi", strut2, getStrut(0));

	removeStrut(&strut2);
	ASSERT_EQUAL("single (removed)", strut1, getStrut(0));

	removeStrut(&strut1);
	ASSERT_EQUAL("empty (removed)", empty, getStrut(0));
}
