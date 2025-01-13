//
// test_Os.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Compat.hh"
#include "Os.hh"

class TestOsEnv : public TestSuite {
public:
	TestOsEnv()
		: TestSuite("OsEnv")
	{
		_os = mkOs();
	}
	~TestOsEnv()
	{
		delete _os;
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testEnv(void);
	static void testOverrideEnv(void);
	static void testCEnv(void);

private:
	static Os *_os;
};

Os *TestOsEnv::_os = nullptr;

bool
TestOsEnv::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "env", testEnv());
	TEST_FN(spec, "overrideEnv", testOverrideEnv());
	TEST_FN(spec, "cenv", testCEnv());
	return status;
}

void
TestOsEnv::testEnv(void)
{
	const std::string key("TestOsEnv::testEnv");
	const std::string value("VALUE");

	_os->setEnv(key, value);
	OsEnv env;
	ASSERT_EQUAL("env value", value, env.getEnv().find(key)->second);
}

void
TestOsEnv::testOverrideEnv(void)
{
	const std::string key("TestOsEnv::testEnv");
	const std::string value("initial");

	_os->setEnv(key, value);
	OsEnv env;
	env.override(key, "override");

	ASSERT_EQUAL("env value", "override", env.getEnv().find(key)->second);
}

void
TestOsEnv::testCEnv(void)
{
	const std::string key("TestOsEnv::testCEnv");
	const std::string value("VALUE");

	_os->setEnv(key, value);
	OsEnv env;
	char **c_env = env.getCEnv();

	for (size_t i = 0; c_env[i] != nullptr; i++) {
		if (std::string(c_env[i]) == "TestOsEnv::testCEnv=VALUE") {
			return ;
		}
	}

	ASSERT_EQUAL("missing env", false, true);
}
