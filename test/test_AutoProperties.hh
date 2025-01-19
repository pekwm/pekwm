//
// test_AutoProperties.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "AutoProperties.hh"
#include "WinLayouter.hh"

class AutoPropertiesTest : public AutoProperties {
public:
	AutoPropertiesTest() : AutoProperties(nullptr) { }
	virtual ~AutoPropertiesTest() { }

	int doParsePlacement(const std::string &str)
	{
		return AutoProperties::parsePlacement(str);
	}
};

class TestAutoProperties : public TestSuite {
public:
	TestAutoProperties();
	virtual ~TestAutoProperties();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testParsePlacement();
};

TestAutoProperties::TestAutoProperties()
	: TestSuite("AutoProperties")
{
}

TestAutoProperties::~TestAutoProperties()
{
}

bool
TestAutoProperties::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parsePlacement", testParsePlacement());
	return status;
}

void
TestAutoProperties::testParsePlacement()
{
	AutoPropertiesTest apt;
	ASSERT_EQUAL("empty", 0, apt.doParsePlacement(""));
	ASSERT_EQUAL("single", WIN_LAYOUTER_SMART,
		     apt.doParsePlacement("SMART"));

	int expected = WIN_LAYOUTER_CENTERED
		| (WIN_LAYOUTER_MOUSECTOPLEFT << WIN_LAYOUTER_SHIFT);
	ASSERT_EQUAL("multi", expected,
		     apt.doParsePlacement("Centered MouseTopLeft"));
}
