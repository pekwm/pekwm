//
// test_Config.cc for pekwm
// Copyright (C) 2021-2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Config.hh"
#include "tk/Action.hh"
#include <sstream>

extern "C" {
#include <unistd.h>
}

class TestConfig : public TestSuite,
		   public Config {
public:
	TestConfig();
	virtual ~TestConfig();

	virtual bool run_test(TestSpec spec, bool status);

	void testParseMenuActionGotoItem();
	void testParseMenuActionInvalid();
	void testParseMoveResizeAction();
	void testLoadDebug();
	void testLoadTheme();
};

TestConfig::TestConfig()
	: TestSuite("Config"),
	  Config()
{
}

TestConfig::~TestConfig()
{
}

bool
TestConfig::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parseMenuActionGotoItem", testParseMenuActionGotoItem());
	TEST_FN(spec, "parseMenuActionInvalid", testParseMenuActionInvalid());
	TEST_FN(spec, "parseMoveResizeAction", testParseMoveResizeAction());
	TEST_FN(spec, "loadDebug", testLoadDebug());
	TEST_FN(spec, "loadTheme", testLoadTheme());
	return status;
}

void
TestConfig::testParseMenuActionGotoItem()
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
TestConfig::testParseMenuActionInvalid()
{
	Action action;
	ASSERT_EQUAL("parse", false, parseMenuAction("Unknown", action));
	ASSERT_EQUAL("action not set", ACTION_NO, action.getAction());
}

void
TestConfig::testParseMoveResizeAction()
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

void
TestConfig::testLoadDebug()
{
	// load of nullptr section, do nothing
	ASSERT_EQUAL("no section", false, loadDebug(nullptr));

	// load of empty section, setup defaults
	CfgParser::Entry empty("memory", 0, "", "");
	ASSERT_EQUAL("empty section", true, loadDebug(&empty));
	ASSERT_EQUAL("default log", getDebugFile(), "/dev/null");
	ASSERT_EQUAL("default level", getDebugLevel(), Debug::LEVEL_WARN);

	// load of section, use values
	std::ostringstream buf;
	buf << "/tmp/test_Config." << getpid() << ".log";
	CfgParser::Entry cfg("memory", 0, "", "");
	cfg.addEntry("memory", 1, "FILE", buf.str());
	cfg.addEntry("memory", 2, "LEVEL", "Debug");

	ASSERT_EQUAL("empty section", true, loadDebug(&cfg));
	ASSERT_EQUAL("default log", getDebugFile(), buf.str());
	ASSERT_EQUAL("default level", getDebugLevel(), Debug::LEVEL_DEBUG);
}

void
TestConfig::testLoadTheme()
{
	// load of nullptr section, do nothing
	ASSERT_EQUAL("no section", false, loadTheme(nullptr));

	// load
	{
		CfgParser::Entry screen("memory", 0, "", "");
		screen.addEntry("memory", 1, "THEMEBACKGROUND", "false");
		loadScreen(&screen);
	}

	// load of empty section, setup defaults
	CfgParser::Entry empty("memory", 0, "", "");
	ASSERT_EQUAL("empty section", true, loadTheme(&empty));
	ASSERT_EQUAL("default override", getThemeBackgroundOverride(), "");
	ASSERT_EQUAL("screen background disabled", getThemeBackground(), false);

	// load of section, use values
	CfgParser::Entry cfg("memory", 0, "", "");
	cfg.addEntry("memory", 1, "BACKGROUNDOVERRIDE", "Solid #aabbcc");

	ASSERT_EQUAL("empty section", true, loadTheme(&cfg));
	ASSERT_EQUAL("override", getThemeBackgroundOverride(), "Solid #aabbcc");
	ASSERT_EQUAL("screen background enabled", getThemeBackground(), true);
}
