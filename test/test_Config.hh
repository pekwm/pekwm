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

	void testParseMenuActionGotoItem(void);
	void testParseMenuActionInvalid(void);
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
	TEST_FN(spec, "parseMenuActionGotoItem", testParseMenuActionGotoItem());
	TEST_FN(spec, "parseMenuActionInvalid", testParseMenuActionInvalid());
	TEST_FN(spec, "parseMoveResizeAction", testParseMoveResizeAction());
	return status;
}

void
TestConfig::testParseMenuActionGotoItem(void)
{
	Action action;
	ASSERT_EQUAL("parse", true,
		     parseMenuAction("GotoItem 4", action));
	ASSERT_EQUAL("action set", ACTION_MENU_GOTO, action.getAction());
	// menu items start from 0, first item is adressed as 1
	ASSERT_EQUAL("action param", 3, action.getParamI(0));

	// not a number, result in no action
	ASSERT_EQUAL("parse", false,
		     parseMenuAction("GotoItem NaN", action));
	ASSERT_EQUAL("action set", ACTION_NO, action.getAction());
}

void
TestConfig::testParseMenuActionInvalid(void)
{
	Action action;
	ASSERT_EQUAL("parse", false, parseMenuAction("Unknown", action));
	ASSERT_EQUAL("action not set", ACTION_NO, action.getAction());
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
