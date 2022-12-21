//
// test_Charset.cc for pekwm
// Copyright (C) 2021-2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "config.h"
#include "Charset.hh"

class TestCharset : public TestSuite {
public:
	TestCharset(void);
	~TestCharset(void);

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testToSystem(void);
	static void testFromSystem(void);
#ifdef PEKWM_HAVE_LOCALE_COMBINE
	static void test_no_grouping_numpunct(void);
#endif // PEKWM_HAVE_LOCALE_COMBINE
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
#ifdef PEKWM_HAVE_LOCALE_COMBINE
	TEST_FN(spec, "no_grouping_numpunct", test_no_grouping_numpunct());
#endif // PEKWM_HAVE_LOCALE_COMBINE
	return status;
}

#ifdef PEKWM_HAVE_LOCALE_COMBINE
void
TestCharset::test_no_grouping_numpunct(void)
{
	std::ostringstream oss;
	oss << 100200300;
	ASSERT_EQUAL("no grouping", "100200300", oss.str());
}
#endif // PEKWM_HAVE_LOCALE_COMBINE
