//
// test_ManagerWindows.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Config.hh"
#include "ManagerWindows.hh"

class TestRootWO : public TestSuite,
                   public RootWO {
public:
    TestRootWO(HintWO *hint_wo, Config *cfg);
    virtual ~TestRootWO(void);

    void testUpdateStrut(void) {
        Strut empty = {0, 0, 0, 0, 0};
        ASSERT_EQUAL("empty", empty, getStrut(0));

        Strut strut1 = {100, 0, 0, 0, 0};
        addStrut(&strut1);
        ASSERT_EQUAL("single", strut1, getStrut(0));

        Strut strut2 = {200, 0, 0, 50, 0};
        addStrut(&strut2);
        ASSERT_EQUAL("multi", strut2, getStrut(0));

        removeStrut(&strut2);
        ASSERT_EQUAL("single (removed)", strut1, getStrut(0));

        removeStrut(&strut1);
        ASSERT_EQUAL("empty (removed)", empty, getStrut(0));
    }
};

TestRootWO::TestRootWO(HintWO *hint_wo, Config *cfg)
    : TestSuite("RootWO"),
      RootWO(None, hint_wo, cfg)
{
    register_test("update strut",
                  std::bind(&TestRootWO::testUpdateStrut, this));

}

TestRootWO::~TestRootWO(void)
{
}
