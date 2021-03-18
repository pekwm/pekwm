//
// test_Charset.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Charset.hh"

class TestCharset : public TestSuite {
public:
    TestCharset(void);
    ~TestCharset(void);

private:
    static void test_to_mb_str(void);
    static void test_no_grouping_numpunct(void);
};

TestCharset::TestCharset(void)
    : TestSuite("Charset")
{
    register_test("no_grouping_numpunct",
                  TestCharset::test_no_grouping_numpunct);
    register_test("test_to_mb_str", TestCharset::test_to_mb_str);
}

TestCharset::~TestCharset(void)
{
}

void
TestCharset::test_no_grouping_numpunct(void)
{
    std::ostringstream oss;
    oss << 100200300;
    ASSERT_EQUAL("no grouping", "100200300", oss.str());
}

void
TestCharset::test_to_mb_str(void)
{
    auto wide_cstr = L"Ge idéerna plats — Mozilla Firefox";
    auto mb_str = Charset::to_mb_str(wide_cstr);
    auto wide_str = Charset::to_wide_str(mb_str);
    if (wide_str != wide_cstr) {
        ASSERT_FAILED("wide -> mb -> wide");
    }
}
