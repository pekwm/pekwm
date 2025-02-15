//
// test_TextFormatter.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "panel/TextFormatter.hh"

class TestTextFormatter : public TestSuite {
public:
	TestTextFormatter();
	virtual ~TestTextFormatter();

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testPreprocess();
	static void testFormat();
};

TestTextFormatter::TestTextFormatter()
	: TestSuite("TextFormatter")
{
}

TestTextFormatter::~TestTextFormatter()
{
}

bool
TestTextFormatter::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "preprocess", testPreprocess());
	TEST_FN(spec, "format", testFormat());
	return status;
}

void
TestTextFormatter::testPreprocess()
{
	VarData var_data;
	WmState wm_state(var_data);
	TextFormatter tf(var_data, wm_state);

	ASSERT_EQUAL("no vars", "no vars", tf.preprocess("no vars"));
	ASSERT_EQUAL("expand env", "current user: " + Util::getEnv("USER"),
		     tf.preprocess("current user: %_USER"));
	std::string str("\"%FIELD\"");
	ASSERT_EQUAL("track vars", str, tf.preprocess(str))
	ASSERT_EQUAL("track vars", 1, tf.getFields().size());
	ASSERT_EQUAL("track vars", "FIELD", tf.getFields()[0]);
}

void
TestTextFormatter::testFormat()
{
	VarData var_data;
	var_data.set("FIELD", "value");
	WmState wm_state(var_data);
	TextFormatter tf(var_data, wm_state);

	std::string str("\"%FIELD\"");
	ASSERT_EQUAL("expand vars", "\"value\"", tf.format(str))
}
