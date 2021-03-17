//
// test_Util.cc for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Util.hh"

class TestString : public TestSuite {
public:
    TestString()
        : TestSuite("String")
    {
        register_test("shell_split", TestString::testShellSplit);
    }

    static void testShellSplit(void) {
        assertShellSplit("basic", "one two three", {"one", "two", "three"});
        assertShellSplit("skip space", "1   2\t\t\t3 \t", {"1", "2", "3"});
        assertShellSplit("single quote", "'o n e' '' three",
                         {"o n e", "", "three"});
        assertShellSplit("double quote", "one \"t w o '' \" three",
                         {"one", "t w o '' ", "three"});
    }

    static void assertShellSplit(std::string msg, std::string str,
                                  std::vector<std::string> expected) {
        auto res = String::shell_split(str);
        ASSERT_EQUAL(msg + " size", expected.size(), res.size());
        for (uint i = 0; i < expected.size(); i++) {
            auto is =  std::to_string(i);
            ASSERT_EQUAL(msg + " res[" + is + "]",
                         expected[i], res[i]);
        }
    }
};

class TestUtil : public TestSuite {
public:
    TestUtil()
        : TestSuite("Util")
    {
        register_test("splitString", TestUtil::testSplitString);
    }

    static void testSplitString(void) {
        assertSplitString("empty string", 0, std::vector<std::string>(),
                          "", ",");
        assertSplitString("no limit", 3, std::vector<std::string>{"1","2","3"},
                          "1,2,3", ",");
    }

    static void assertSplitString(std::string msg,
                                  uint e_ret, std::vector<std::string> e_toks,
                                  const std::string str, const char *sep,
                                  uint max = 0,
                                  bool include_empty = false,
                                  char escape = '\\') {
        std::vector<std::string> toks;
        uint ret = Util::splitString(str, toks, sep, max,
                                     include_empty, escape);
        ASSERT_EQUAL(msg + " ret", e_ret, ret);
        ASSERT_EQUAL(msg + " tokens", e_toks.size(), toks.size());
        for (uint i = 0; i < e_toks.size(); i++) {
            auto is = std::to_string(i);
            ASSERT_EQUAL(msg + " tok " + is, e_toks[i], toks[i]);
        }
    }
};
