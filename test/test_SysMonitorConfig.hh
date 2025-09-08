//
// test_SysMonitorConfig.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "sys/SysMonitorConfig.hh"
#include "test_Mock.hh"

#include <sstream>

class TestSysMonitorConfig : public TestSuite {
public:
	TestSysMonitorConfig();
	virtual ~TestSysMonitorConfig();

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testLoad();
	static void testSave();
	static void testAutoConfig();
};
TestSysMonitorConfig::TestSysMonitorConfig()
	: TestSuite("SysMonitorConfig")
{
}

TestSysMonitorConfig::~TestSysMonitorConfig()
{
}

bool
TestSysMonitorConfig::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "load", testLoad());
	TEST_FN(spec, "save", testSave());
	TEST_FN(spec, "autoConfig", testAutoConfig());
	return status;
}

void
TestSysMonitorConfig::testLoad()
{
}

void
TestSysMonitorConfig::testSave()
{
	// empty
	SysMonitorConfig config;
	std::stringstream empty;
	ASSERT_TRUE("empty", config.save(empty));
	ASSERT_EQUAL("empty", "", empty.str());

	// single monitors with single monitor
	SysMonitorConfig::MonitorsConfig monitors(1920, 1080, 597, 336);
	SysMonitorConfig::MonitorConfig monitor1(
		"DP-1", "1920x1080", "c327b0c8d7bcd49fb23754ffd879eca2",
		75.0, 0, 0, RR_Rotate_0);
	monitors.add(monitor1);
	config.add(monitors);

	std::stringstream single;
	std::string single_expected =
		"Monitors = \"7a981ce8cd90a4e138a14c4fc734cdd9\" {\n"
		"\tWidth = \"1920\"\n"
		"\tHeight = \"1080\"\n"
		"\tWidthMm = \"597\"\n"
		"\tHeightMm = \"336\"\n"
		"\tMonitor = \"DP-1\" {\n"
		"\t\tModeName = \"1920x1080\"\n"
		"\t\tEdid = \"c327b0c8d7bcd49fb23754ffd879eca2\"\n"
		"\t\tRefreshRate = \"75.00\"\n"
		"\t\tPosX = \"0\"\n"
		"\t\tPosY = \"0\"\n"
		"\t\tRotate = \"0\"\n"
		"\t}\n"
		"}\n";
	ASSERT_TRUE("single", config.save(single));
	ASSERT_EQUAL("single", single_expected, single.str());

	// multiple monitors with multiple monitor
	SysMonitorConfig::MonitorConfig monitor2(
		"DP-2", "1600x1200", "aaaab0c8d7bcd49fb23754ffd879aaaa",
		120.5, 1920, 0, RR_Rotate_90);
	monitors.setWidth(3520);
	monitors.setHeight(1200);
	monitors.setWidthMm(1185);
	monitors.setHeightMm(381);
	monitors.add(monitor2);
	config.add(monitors);

	std::stringstream multi;
	std::string multi_expected =
		single_expected +
		"\n"
		"Monitors = \"b60c67c648260a065b4d8d8fa9ca1ea0\" {\n"
		"\tWidth = \"3520\"\n"
		"\tHeight = \"1200\"\n"
		"\tWidthMm = \"1185\"\n"
		"\tHeightMm = \"381\"\n"
		"\tMonitor = \"DP-1\" {\n"
		"\t\tModeName = \"1920x1080\"\n"
		"\t\tEdid = \"c327b0c8d7bcd49fb23754ffd879eca2\"\n"
		"\t\tRefreshRate = \"75.00\"\n"
		"\t\tPosX = \"0\"\n"
		"\t\tPosY = \"0\"\n"
		"\t\tRotate = \"0\"\n"
		"\t}\n"
		"\n"
		"\tMonitor = \"DP-2\" {\n"
		"\t\tModeName = \"1600x1200\"\n"
		"\t\tEdid = \"aaaab0c8d7bcd49fb23754ffd879aaaa\"\n"
		"\t\tRefreshRate = \"120.50\"\n"
		"\t\tPosX = \"1920\"\n"
		"\t\tPosY = \"0\"\n"
		"\t\tRotate = \"90\"\n"
		"\t}\n"
		"}\n";
	ASSERT_TRUE("multi", config.save(multi));
	ASSERT_EQUAL("multi", multi_expected, multi.str());
}

void
TestSysMonitorConfig::testAutoConfig()
{


}
