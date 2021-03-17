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

    static void testConstruct(void) {
        Action a;
        ASSERT_EQUAL(std::string("empty"),
                     ACTION_UNSET, a.getAction());
        Action a1(ACTION_FOCUS);
        ASSERT_EQUAL(std::string("action"),
                     ACTION_FOCUS, a1.getAction());
    }

    static void testParamI(void) {
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

    static void testParamS(void) {
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
};

class TestActionConfig : public TestSuite{
public:
    TestActionConfig()
        : TestSuite("Config")
    {
        register_test("parseActionSetGeometry",
                      TestActionConfig::testParseActionSetGeometry);
    }

    static void testParseActionSetGeometry(void)
    {
        Action a_empty;
        ActionConfig::parseActionSetGeometry(a_empty, "");
        assertAction("parseActionSetGeometry empty", a_empty, { }, { });

        Action a_no_head;
        ActionConfig::parseActionSetGeometry(a_no_head, "10x10+0+0");
        assertAction("parseActionSetGeometry no head",
                     a_no_head, { -1, 0 }, { "10x10+0+0" });

        Action a_head;
        ActionConfig::parseActionSetGeometry(a_head, "10x10+0+0 0");
        assertAction("parseActionSetGeometry head",
                     a_head, { 0, 0 }, { "10x10+0+0" });

        Action a_screen;
        ActionConfig::parseActionSetGeometry(a_screen, "10x10+0+0 screen");
        assertAction("parseActionSetGeometry screen",
                     a_screen, { -1, 0 }, { "10x10+0+0" });

        Action a_current;
        ActionConfig::parseActionSetGeometry(a_current, "10x10+0+0 current");
        assertAction("parseActionSetGeometry current",
                     a_current, { -2, 0 }, { "10x10+0+0" });

        Action a_strut;
        ActionConfig::parseActionSetGeometry(a_strut,
                                             "10x10+0+0 current HonourStrut");
        assertAction("parseActionSetGeometry strut",
                     a_strut, { -2, 1 }, { "10x10+0+0" });
    }

    static void assertAction(std::string msg, const Action& action,
                             std::vector<int> e_int,
                             std::vector<std::string> e_str) {
        for (uint i = 0; i < e_int.size(); i++) {
            auto is = std::to_string(i);
            ASSERT_EQUAL(msg + " I " + is,
                         e_int[i], action.getParamI(i));
        }
        for (uint i = 0; i < e_str.size(); i++) {
            auto is = std::to_string(i);
            ASSERT_EQUAL(msg + " S " + is,
                         e_str[i], action.getParamS(i));
        }
    }
};

TestAction::TestAction(void)
    : Action(),
      TestSuite("Action")
{
    register_test("construct", TestAction::testConstruct);
    register_test("ParamI", TestAction::testParamI);
    register_test("ParamS", TestAction::testParamS);
}

TestAction::~TestAction(void)
{
}
