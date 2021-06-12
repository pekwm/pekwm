//
// test_Action.cc for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Action.hh"

class TestAction : public Action,
                   public TestSuite {
public:
	TestAction(void);
	virtual ~TestAction(void);

	bool run_test(TestSpec spec, bool status);

	static void testConstruct(void);
	static void testParamI(void);
	static void testParamS(void);
};

TestAction::TestAction(void)
	: Action(),
	  TestSuite("Action")
{
}

TestAction::~TestAction(void)
{
}

bool
TestAction::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "construct", testConstruct());
	TEST_FN(spec, "ParamI", testParamI());
	TEST_FN(spec, "ParamS", testParamS());
	return status;
}

void
TestAction::testConstruct(void)
{
	Action a;
	ASSERT_EQUAL(std::string("empty"),
		     ACTION_UNSET, a.getAction());
	Action a1(ACTION_FOCUS);
	ASSERT_EQUAL(std::string("action"),
		     ACTION_FOCUS, a1.getAction());
}

void
TestAction::testParamI(void)
{
	Action a;
	ASSERT_EQUAL(std::string("not set"),
		     0, a.getParamI(0));

	a.setParamI(0, 1);
	ASSERT_EQUAL(std::string("set/get first"),
		     1, a.getParamI(0));

	a.setParamI(2, 3);
	ASSERT_EQUAL(std::string("gap"),
		     1, a.getParamI(0));
	ASSERT_EQUAL(std::string("gap"),
		     0, a.getParamI(1));
	ASSERT_EQUAL(std::string("gap"),
		     3, a.getParamI(2));

	a.setParamI(1, 2);
	ASSERT_EQUAL(std::string("second"),
		     2, a.getParamI(1));
}

void
TestAction::testParamS(void)
{
	Action a;
	ASSERT_EQUAL(std::string("not set"),
		     std::string(""), a.getParamS());

	a.setParamS("first");
	ASSERT_EQUAL(std::string("set/get first"),
		     std::string("first"), a.getParamS());

	a.setParamS(2, "third");
	ASSERT_EQUAL(std::string("gap"),
		     std::string("first"), a.getParamS(0));
	ASSERT_EQUAL(std::string("gap"),
		     std::string(""), a.getParamS(1));
	ASSERT_EQUAL(std::string("gap"),
		     std::string("third"), a.getParamS(2));

	a.setParamS(1, "second");
	ASSERT_EQUAL(std::string("second"),
		     std::string("second"), a.getParamS(1));
}

class TestActionConfig : public TestSuite{
public:
	TestActionConfig()
		: TestSuite("Config")
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testParseActionSetGeometry(void);
	static void assertAction(std::string msg, const Action& action,
				 std::vector<int> e_int,
				 std::vector<std::string> e_str);
};

bool
TestActionConfig::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parseActionSetGeometry", testParseActionSetGeometry());
	return status;
}

void
TestActionConfig::testParseActionSetGeometry(void)
{
	Action a_empty;
	ActionConfig::parseActionSetGeometry(a_empty, "");
	assertAction("parseActionSetGeometry empty", a_empty,
		     std::vector<int>(), std::vector<std::string>());

	Action a_no_head;
	ActionConfig::parseActionSetGeometry(a_no_head, "10x10+0+0");
	std::vector<int> no_head_i;
	no_head_i.push_back(-1);
	no_head_i.push_back(0);
	std::vector<std::string> no_head_str;
	no_head_str.push_back("10x10+0+0");
	assertAction("parseActionSetGeometry no head",
		     a_no_head, no_head_i, no_head_str);

	Action a_head;
	ActionConfig::parseActionSetGeometry(a_head, "10x10+0+0 0");
	std::vector<int> head_i;
	no_head_i.push_back(-1);
	no_head_i.push_back(0);
	std::vector<std::string> head_str;
	no_head_str.push_back("10x10+0+0");
	assertAction("parseActionSetGeometry head",
		     a_head, head_i, head_str);

	Action a_screen;
	ActionConfig::parseActionSetGeometry(a_screen, "10x10+0+0 screen");
	std::vector<int> screen_i;
	screen_i.push_back(-1);
	screen_i.push_back(0);
	std::vector<std::string> screen_str;
	screen_str.push_back("10x10+0+0");
	assertAction("parseActionSetGeometry screen",
		     a_screen, head_i, screen_str);

	Action a_current;
	ActionConfig::parseActionSetGeometry(a_current, "10x10+0+0 current");
	std::vector<int> current_i;
	current_i.push_back(-2);
	current_i.push_back(0);
	std::vector<std::string> current_str;
	current_str.push_back("10x10+0+0");
	assertAction("parseActionSetGeometry current",
		     a_current, current_i, current_str);

	Action a_strut;
	ActionConfig::parseActionSetGeometry(a_strut,
					     "10x10+0+0 current HonourStrut");
	std::vector<int> strut_i;
	strut_i.push_back(-2);
	strut_i.push_back(1);
	std::vector<std::string> strut_str;
	strut_str.push_back("10x10+0+0");
	assertAction("parseActionSetGeometry strut",
		     a_strut, strut_i, strut_str);
}

void
TestActionConfig::assertAction(std::string msg, const Action& action,
                               std::vector<int> e_int,
                               std::vector<std::string> e_str) {
	for (uint i = 0; i < e_int.size(); i++) {
		std::string is = std::to_string(i);
		ASSERT_EQUAL(msg + " I " + is,
			     e_int[i], action.getParamI(i));
	}
	for (uint i = 0; i < e_str.size(); i++) {
		std::string is = std::to_string(i);
		ASSERT_EQUAL(msg + " S " + is,
			     e_str[i], action.getParamS(i));
	}
}
