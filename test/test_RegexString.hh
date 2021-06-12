//
// test_RegexString.hh for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "RegexString.hh"

class TestRegexString : public TestSuite {
public:
	TestRegexString()
		: TestSuite("RegeXString")
	{
	}

	virtual bool run_test(TestSpec spec, bool status);
	static void testEdS(void);
};

bool
TestRegexString::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "ed_s", testEdS());
	return status;
}

void
TestRegexString::testEdS(void)
{
	RegexString ed_s;
	ed_s.parse_ed_s("/(.*) - Test/T: \\1/");
	std::string str("My - Test");
	ASSERT_TRUE("ed_s", ed_s.ed_s(str));
	ASSERT_EQUAL("ed_s", "T: My", str);
}

