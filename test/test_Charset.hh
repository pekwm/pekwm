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
    static void testToSystem(void);
    static void testFromSystem(void);
    static void test_no_grouping_numpunct(void);
};

TestCharset::TestCharset(void)
    : TestSuite("Charset")
{
    register_test("no_grouping_numpunct",
                  TestCharset::test_no_grouping_numpunct);
    register_test("testToSystem", TestCharset::testToSystem);
    register_test("testFromSystem", TestCharset::testFromSystem);
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
TestCharset::testToSystem(void)
{
}

void
TestCharset::testFromSystem(void)
{
}
