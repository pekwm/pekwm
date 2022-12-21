//
// test_Config.cc for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Config.hh"
#include "Action.hh"

class TestConfig : public TestSuite,
		   public Config {
public:
	TestConfig(void);
	virtual ~TestConfig(void);

	virtual bool run_test(TestSpec spec, bool status);

	void testParseMoveResizeAction(void);
};

TestConfig::TestConfig(void)
	: TestSuite("Config"),
	  Config()
{
}

TestConfig::~TestConfig(void)
{
}

bool
TestConfig::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parseMoveResizeAction", testParseMoveResizeAction());
	return status;
}

void
TestConfig::testParseMoveResizeAction(void)
{
	Action action;
	ASSERT_EQUAL("parse 1 ok", true,
		     parseMoveResizeAction("Movehorizontal 1", action));
	ASSERT_EQUAL("parsed value", 1, action.getParamI(0));
	ASSERT_EQUAL("parsed unit", UNIT_PIXEL, action.getParamI(1));

	ASSERT_EQUAL("parse -3% ok", true,
		     parseMoveResizeAction("Movehorizontal -3%", action));
	ASSERT_EQUAL("parsed value", -3, action.getParamI(0));
	ASSERT_EQUAL("parsed unit", UNIT_PERCENT, action.getParamI(1));
}
