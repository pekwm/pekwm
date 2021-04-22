//
// test_InputDialog.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "InputDialog.hh"

class TestInputBuffer : public TestSuite {
public:
    TestInputBuffer(void);
    ~TestInputBuffer(void);

    virtual bool run_test(TestSpec spec, bool status);

private:
    static void testConstruct(void);
    static void testAdd(void);
    static void testRemove(void);
    static void testClear(void);
    static void testKill(void);
    static void testChangePos(void);
};

TestInputBuffer::TestInputBuffer(void)
    : TestSuite("InputBuffer")
{
}

TestInputBuffer::~TestInputBuffer(void)
{
}

bool
TestInputBuffer::run_test(TestSpec spec, bool status)
{
    TEST_FN(spec, "construct", testConstruct());
    TEST_FN(spec, "add", testAdd());
    TEST_FN(spec, "remove", testRemove());
    TEST_FN(spec, "clear", testClear());
    TEST_FN(spec, "kill", testKill());
    TEST_FN(spec, "changePos", testChangePos());
    return status;
}

void
TestInputBuffer::testConstruct(void)
{
    InputBuffer buf;
    ASSERT_EQUAL("construct", "", buf.str());
    ASSERT_EQUAL("construct", 0, buf.pos());
}

void
TestInputBuffer::testAdd(void)
{
    InputBuffer buf;

    buf.add("r");
    ASSERT_EQUAL("add", "r", buf.str());
    ASSERT_EQUAL("add", 1, buf.pos());

    buf.add("ä");
    ASSERT_EQUAL("add (utf8)", "rä", buf.str());
    ASSERT_EQUAL("add (utf8)", 3, buf.pos());

    buf.add("k");
    ASSERT_EQUAL("add", "räk", buf.str());
    ASSERT_EQUAL("add", 4, buf.pos());
}

void
TestInputBuffer::testRemove(void)
{
    InputBuffer buf;

    buf.add("räk");
    ASSERT_EQUAL("remove", "räk", buf.str());
    ASSERT_EQUAL("remove", 4, buf.pos());

    buf.remove();
    ASSERT_EQUAL("remove", "rä", buf.str());
    ASSERT_EQUAL("remove", 3, buf.pos());

    buf.remove();
    ASSERT_EQUAL("remove (utf8)", "r", buf.str());
    ASSERT_EQUAL("remove (utf8)", 1, buf.pos());

    buf.remove();
    ASSERT_EQUAL("remove", "", buf.str());
    ASSERT_EQUAL("remove", 0, buf.pos());
}

void
TestInputBuffer::testClear(void)
{
    InputBuffer buf("content");

    buf.clear();
    ASSERT_EQUAL("remove", "", buf.str());
    ASSERT_EQUAL("remove", 0, buf.pos());
}

void
TestInputBuffer::testKill(void)
{
    InputBuffer buf("content", 4);

    buf.kill();
    ASSERT_EQUAL("kill", "cont", buf.str());
    ASSERT_EQUAL("kill", 4, buf.pos());

    buf.kill();
    ASSERT_EQUAL("kill", "cont", buf.str());
    ASSERT_EQUAL("kill", 4, buf.pos());
}

void
TestInputBuffer::testChangePos(void)
{
    InputBuffer buf("räksmörgås", -1);

    buf.changePos(-3);
    buf.kill();
    ASSERT_EQUAL("changePos", "räksmör", buf.str());

    size_t pos = buf.pos();
    buf.changePos(-3);
    buf.changePos(3);
    ASSERT_EQUAL("changePos", pos, buf.pos());
}
