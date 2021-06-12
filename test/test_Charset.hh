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

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testToSystem(void);
	static void testFromSystem(void);
	static void test_no_grouping_numpunct(void);
};

TestCharset::TestCharset(void)
	: TestSuite("Charset")
{
}

TestCharset::~TestCharset(void)
{
}

bool
TestCharset::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "no_grouping_numpunct", test_no_grouping_numpunct());
	return status;
}

void
TestCharset::test_no_grouping_numpunct(void)
{
	std::ostringstream oss;
	oss << 100200300;
	ASSERT_EQUAL("no grouping", "100200300", oss.str());
}
