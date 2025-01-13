//
// test_Mem.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Mem.hh"

extern "C" {
#include <string.h>
}

class TestMem : public TestSuite {
public:
	TestMem();
	~TestMem();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testDestruct();
	static void testBuf();
};

TestMem::TestMem(void)
	: TestSuite("Mem")
{
}

TestMem::~TestMem(void)
{
}

bool
TestMem::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "Destruct", testDestruct());
	TEST_FN(spec, "Buf", testBuf());
	return status;
}

class DestructTest {
public:
	~DestructTest()
	{
		_destruct_count++;
	}

	static int getDestructCount() { return _destruct_count; }

private:
	static int _destruct_count;
};

int DestructTest::_destruct_count = 0;

void
TestMem::testDestruct()
{
	{
		Destruct<DestructTest> destruct(new DestructTest());
		ASSERT_EQUAL("destruct", 0, DestructTest::getDestructCount());
	}
	ASSERT_EQUAL("destruct", 1, DestructTest::getDestructCount());
}

void
TestMem::testBuf()
{
	const char *value = "initial value";

	Buf<char> buf(32);
	ASSERT_EQUAL("buf", 32, buf.size());
	memset(*buf, '\0', buf.size());
	memcpy(*buf, value, strlen(value));
	buf.grow(true);
	ASSERT_EQUAL("buf", 64, buf.size());
	ASSERT_EQUAL("buf", 0, memcmp(*buf, value, strlen(value) + 1));
}
