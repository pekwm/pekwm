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

class TestUtil {
public:
    static void testSplitString(void) {
        assertSplitString("empty string", 0, std::vector<std::string>(), "", ",");
        assertSplitString("no limit", 3, std::vector<std::string>{"1","2","3"}, "1,2,3", ",");
    }

    static void assertSplitString(std::string msg, uint e_ret, std::vector<std::string> e_toks,
                                  const std::string str, const char *sep, uint max = 0,
                                  bool include_empty = false, char escape = '\\') {
        std::vector<std::string> toks;
        int ret = Util::splitString(str, toks, sep, max, include_empty, escape);
        ASSERT_EQUAL("splitString", msg + " ret", e_ret, ret);
        ASSERT_EQUAL("splitString", msg + " tokens", e_toks.size(), toks.size());
        for (int i = 0; i < e_toks.size(); i++) {
            ASSERT_EQUAL("splitString", msg + " tok " + std::to_string(i),
                         e_toks[i], toks[i]);
        }
    }
};

int
main(int argc, char *argv[])
{
    try {
        TestUtil::testSplitString();
        return 0;
    } catch (AssertFailed &ex) {
        std::cerr << ex.file() << ":" << ex.line() << " " << ex.msg() << std::endl;
        return 1;
    }
}
