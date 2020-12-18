//
// test_Config.cc for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Config.hh"

class TestConfig : public Config {
public:
    static void testParseActionSetGeometry(void) {
        Action a_empty;
        Config::parseActionSetGeometry(a_empty, "");
        assertAction("parseActionSetGeometry empty", a_empty, { }, { });

        Action a_no_head;
        Config::parseActionSetGeometry(a_no_head, "10x10+0+0");
        assertAction("parseActionSetGeometry no head", a_no_head, { -1, 0 }, { "10x10+0+0" });

        Action a_head;
        Config::parseActionSetGeometry(a_head, "10x10+0+0 0");
        assertAction("parseActionSetGeometry head", a_head, { 0, 0 }, { "10x10+0+0" });

        Action a_screen;
        Config::parseActionSetGeometry(a_screen, "10x10+0+0 screen");
        assertAction("parseActionSetGeometry screen", a_screen, { -1, 0 }, { "10x10+0+0" });

        Action a_current;
        Config::parseActionSetGeometry(a_current, "10x10+0+0 current");
        assertAction("parseActionSetGeometry current", a_current, { -2, 0 }, { "10x10+0+0" });

        Action a_strut;
        Config::parseActionSetGeometry(a_strut, "10x10+0+0 current HonourStrut");
        assertAction("parseActionSetGeometry strut", a_strut, { -2, 1 }, { "10x10+0+0" });
    }

    static void assertAction(std::string msg, const Action& action,
                             std::vector<int> e_int, std::vector<std::string> e_str) {
        for (int i = 0; i < e_int.size(); i++) {
            ASSERT_EQUAL("assertAction", msg + " I " + std::to_string(i),
                         e_int[i], action.getParamI(i));
        }
        for (int i = 0; i < e_str.size(); i++) {
            ASSERT_EQUAL("assertAction", msg + " S " + std::to_string(i),
                         e_str[i], action.getParamS(i));
        }
    }
};

int
main(int argc, char *argv[])
{
    try {
        TestConfig::testParseActionSetGeometry();
        return 0;
    } catch (AssertFailed &ex) {
        std::cerr << ex.file() << ":" << ex.line() << " " << ex.msg() << std::endl;
        return 1;
    }
}
