//
// test_SysConfig.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "sys/SysConfig.hh"

class OsMock : public Os {
public:
	OsMock() : Os() { }
	virtual ~OsMock() { }

	virtual bool getEnv(const std::string &key, std::string &val,
			    const std::string &val_default)
	{
		if (key == "PEKWM_CONFIG_FILE") {
			val = "../test/data/config.pekwm_sys";
			return true;
		}
		val = val_default;
		return false;
	}

	virtual bool setEnv(const std::string&, const std::string&)
	{
		return false;
	}

	virtual pid_t processExec(const std::vector<std::string>&, OsEnv*)
	{
		return -1;
	}

	virtual ChildProcess *childExec(const std::vector<std::string>&,
					int flags, OsEnv *env)
	{
		return nullptr;
	}

	virtual bool processSignal(pid_t, int)
	{
		return false;
	}

	virtual bool isProcessAlive(pid_t)
	{
		return false;
	}

};

class TestSysConfig : public TestSuite {
public:
	TestSysConfig(void);
	virtual ~TestSysConfig(void);

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testParseConfig();
};
TestSysConfig::TestSysConfig(void)
	: TestSuite("SysConfig")
{
}

TestSysConfig::~TestSysConfig(void)
{
}

bool
TestSysConfig::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parseConfig", testParseConfig());
	return status;
}

void
TestSysConfig::testParseConfig()
{
	OsMock os;
	SysConfig cfg(&os);
	ASSERT_TRUE("parseConfig", cfg.parseConfig());
	ASSERT_EQUAL("parseConfig", true, cfg.isXSettingsEnabled());
	ASSERT_EQUAL("parseConfig", false, cfg.isLocationLookup());
	ASSERT_DOUBLE_EQUAL("parseConfig", 32.01, cfg.getLatitude());
	ASSERT_DOUBLE_EQUAL("parseConfig", 16.01, cfg.getLongitude());
	ASSERT_EQUAL("parseConfig", "AUTO", cfg.getTimeOfDay());
	ASSERT_EQUAL("parseConfig", "Raleigh", cfg.getNetTheme());
	const SysConfig::string_vector &dcmds = cfg.getDaytimeCommands();
	ASSERT_EQUAL("daytime commands", 2, dcmds.size());
	ASSERT_EQUAL("daytime command 1", "daytime1", dcmds[0]);
	ASSERT_EQUAL("daytime command 2", "daytime2", dcmds[1]);
	const SysConfig::string_vector &lcmds = cfg.getLocationCommands();
	ASSERT_EQUAL("location commands", 2, lcmds.size());
	ASSERT_EQUAL("location command 1", "location1", lcmds[0]);
	ASSERT_EQUAL("location command 2", "location2", lcmds[1]);
	const SysConfig::string_map &dawn_xsrcs =
		 cfg.getXResources(TIME_OF_DAY_DAWN);
	ASSERT_EQUAL("xrsrc dawn", 1, dawn_xsrcs.size());
	ASSERT_EQUAL("xrsrc", "#999999", dawn_xsrcs.at("XTerm*background"));
	const SysConfig::string_map &day_xsrcs =
		 cfg.getXResources(TIME_OF_DAY_DAY);
	ASSERT_EQUAL("xrsrc day", 1, day_xsrcs.size());
	ASSERT_EQUAL("xrsrc", "#ffffff", day_xsrcs.at("XTerm*background"));
	const SysConfig::string_map &dusk_xsrcs =
		 cfg.getXResources(TIME_OF_DAY_DUSK);
	ASSERT_EQUAL("xrsrc dusk", 1, dusk_xsrcs.size());
	ASSERT_EQUAL("xrsrc", "#666666", dusk_xsrcs.at("XTerm*background"));
	const SysConfig::string_map &night_xsrcs =
		 cfg.getXResources(TIME_OF_DAY_NIGHT);
	ASSERT_EQUAL("xrsrc night", 1, night_xsrcs.size());
	ASSERT_EQUAL("xrsrc", "#333333", night_xsrcs.at("XTerm*background"));
}
