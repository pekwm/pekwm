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

class TestAction : public Action {
public:
    static void testConstruct(void) {
        Action a;
        ASSERT_EQUAL("construct", std::string("empty"),
                     ACTION_UNSET, a.getAction());
        Action a1(ACTION_FOCUS);
        ASSERT_EQUAL("construct", std::string("action"),
                     ACTION_FOCUS, a1.getAction());
    }

    static void testParamI(void) {
        Action a;
        ASSERT_EQUAL("testParamI", std::string("not set"),
                     0, a.getParamI(0));

        a.setParamI(0, 1);
        ASSERT_EQUAL("testParamI", std::string("set/get first"),
                     1, a.getParamI(0));

        a.setParamI(2, 3);
        ASSERT_EQUAL("testParamI", std::string("gap"),
                     1, a.getParamI(0));
        ASSERT_EQUAL("testParamI", std::string("gap"),
                     0, a.getParamI(1));
        ASSERT_EQUAL("testParamI", std::string("gap"),
                     3, a.getParamI(2));

        a.setParamI(1, 2);
        ASSERT_EQUAL("testParamI", std::string("second"),
                     2, a.getParamI(1));
    }

    static void testParamS(void) {
        Action a;
        ASSERT_EQUAL("testParamS", std::string("not set"),
                     std::string(""), a.getParamS());

        a.setParamS("first");
        ASSERT_EQUAL("testParamS", std::string("set/get first"),
                     std::string("first"), a.getParamS());

        a.setParamS(2, "third");
        ASSERT_EQUAL("testParamS", std::string("gap"),
                     std::string("first"), a.getParamS(0));
        ASSERT_EQUAL("testParamS", std::string("gap"),
                     std::string(""), a.getParamS(1));
        ASSERT_EQUAL("testParamS", std::string("gap"),
                     std::string("third"), a.getParamS(2));

        a.setParamS(1, "second");
        ASSERT_EQUAL("testParamS", std::string("second"),
                     std::string("second"), a.getParamS(1));
    }
};

int
main(int argc, char *argv[])
{
    try {
        TestAction::testConstruct();
        TestAction::testParamI();
        TestAction::testParamS();
        return 0;
    } catch (AssertFailed &ex) {
        std::cerr << ex.file() << ":" << ex.line() << " " << ex.msg() << std::endl;
        return 1;
    }
}
