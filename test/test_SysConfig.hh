//
// test_SysConfig.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "sys/SysConfig.hh"
#include "test_Mock.hh"

class TestSysConfig : public TestSuite {
public:
	TestSysConfig();
	virtual ~TestSysConfig();

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testParseConfig();
	static void testParseConfigInvalidLatLong();
	static void testParseConfigXResources();
};
TestSysConfig::TestSysConfig()
	: TestSuite("SysConfig")
{
}

TestSysConfig::~TestSysConfig()
{
}

bool
TestSysConfig::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parseConfig", testParseConfig());
	TEST_FN(spec, "parseConfigInvalidLatLong",
		testParseConfigInvalidLatLong());
	TEST_FN(spec, "parseConfigXResources", testParseConfigXResources());
	return status;
}

void
TestSysConfig::testParseConfig()
{
	OsMock os;
	SysConfig cfg("../test/data/config.pekwm_sys", &os);
	ASSERT_TRUE("parseConfig", cfg.parseConfig());
	ASSERT_EQUAL("parseConfig", true, cfg.isXSettingsEnabled());
	ASSERT_EQUAL("parseConfig", false, cfg.isLocationLookup());
	ASSERT_DOUBLE_EQUAL("parseConfig", 32.01, cfg.getLatitude());
	ASSERT_DOUBLE_EQUAL("parseConfig", 16.01, cfg.getLongitude());
	ASSERT_TRUE("haveLocation", cfg.haveLocation());
	ASSERT_TRUE("monitorLoadOnChange", cfg.isMonitorLoadOnChange());
	ASSERT_FALSE("monitorAutoConfig", cfg.isMonitorAutoConfig());
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

void
TestSysConfig::testParseConfigInvalidLatLong()
{
	OsMock os;
	SysConfig cfg1("../test/data/config.pekwm_sys.invalid_lat", &os);

	ASSERT_TRUE("parseConfig", cfg1.parseConfig());
	ASSERT_FALSE("haveLocation", cfg1.haveLocation());
	ASSERT_TRUE("latitude NAN", isnan(cfg1.getLatitude()));
	ASSERT_FALSE("longitude NAN", isnan(cfg1.getLongitude()));

	SysConfig cfg2("../test/data/config.pekwm_sys.invalid_long", &os);
	ASSERT_TRUE("parseConfig", cfg2.parseConfig());
	ASSERT_FALSE("haveLocation", cfg2.haveLocation());
	ASSERT_FALSE("latitude NAN", isnan(cfg2.getLatitude()));
	ASSERT_TRUE("longitude NAN", isnan(cfg2.getLongitude()));
}

void
TestSysConfig::testParseConfigXResources()
{
	const char *cfg =
		"XResources {\n"
		"  \"XTerm*background\" = \"#22272e\"\n"
		"  \"XTerm*foreground\" = \"#a5b1be\"\n"
		"}\n";
	CfgParserSourceString *source =
		new CfgParserSourceString(":memory:", cfg);
	CfgParser parser(CfgParserOpt(""));
	parser.parse(source);

	std::map<std::string, std::string> xresources;
	SysConfig::parseConfigXResources(parser.getEntryRoot(), xresources,
					 "XRESOURCES");
	ASSERT_EQUAL("xresources", 2, xresources.size());
	ASSERT_EQUAL("background", "#22272e", xresources["XTerm*background"]);
	ASSERT_EQUAL("foreground", "#a5b1be", xresources["XTerm*foreground"]);
}
