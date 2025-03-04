//
// test_ExternalCommandData.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "panel/ExternalCommandData.hh"

class ExternalCommandDataTest : public ExternalCommandData {
public:
	ExternalCommandDataTest(const PanelConfig& cfg, VarData& var_data)
		: ExternalCommandData(cfg, var_data)
	{
	}
	virtual ~ExternalCommandDataTest() { }

	void append(const std::string &data, const std::string &assign)
	{
		std::string buf;
		ExternalCommandData::append(buf, data.c_str(), data.size(),
					    assign);
	}
};

class TestExternalCommandData : public TestSuite {
public:
	TestExternalCommandData(void);
	virtual ~TestExternalCommandData(void);

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testAppend();
	static void testAppendAssign();
};
TestExternalCommandData::TestExternalCommandData(void)
	: TestSuite("ExternalCommandData")
{
}

TestExternalCommandData::~TestExternalCommandData(void)
{
}

bool
TestExternalCommandData::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "append", testAppend());
	TEST_FN(spec, "appendAssign", testAppendAssign());
	return status;
}

void
TestExternalCommandData::testAppend()
{
	PanelConfig cfg(1.0);
	VarData var_data;
	ExternalCommandDataTest ecd(cfg, var_data);

	ecd.append("var value", "");
	ASSERT_EQUAL("incomplete line", "", var_data.get("var"));

	ecd.append("var single\n", "");
	ASSERT_EQUAL("single value", "single", var_data.get("var"));

	ecd.append("var1 1\nvar2 2\nvar3 3\n", "");
	ASSERT_EQUAL("multi value", "1", var_data.get("var1"));
	ASSERT_EQUAL("multi value", "2", var_data.get("var2"));
	ASSERT_EQUAL("multi value", "3", var_data.get("var3"));
}

void
TestExternalCommandData::testAppendAssign()
{
	PanelConfig cfg(1.0);
	VarData var_data;
	ExternalCommandDataTest ecd(cfg, var_data);

	ecd.append("incomplete line", "var");
	ASSERT_EQUAL("incomplete line", "", var_data.get("var"));

	ecd.append("single line\n", "var");
	ASSERT_EQUAL("single line", "single line", var_data.get("var"));

	ecd.append("first line\nsecond line\n", "var");
	ASSERT_EQUAL("multi line", "second line", var_data.get("var"));
}
