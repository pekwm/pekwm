//
// test_SysResources.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "X11.hh"
#include "sys/SysResources.hh"
#include "test_Mock.hh"

class TestSysResources : public TestSuite {
public:
	TestSysResources();
	virtual ~TestSysResources();

	virtual bool run_test(TestSpec spec, bool status);
private:
	static void testSetXResourceDpi();
	static void testSetConfiguredXResources();
};

TestSysResources::TestSysResources()
	: TestSuite("SysResources")
{
}

TestSysResources::~TestSysResources()
{
}

bool
TestSysResources::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "setXResourceDpi", testSetXResourceDpi());
	TEST_FN(spec, "setConfiguredXResources", testSetConfiguredXResources());
	return status;
}

void
TestSysResources::testSetXResourceDpi()
{
	OsMock os;
	std::string config_file;
	os.getEnv("PEKWM_CONFIG_FILE", config_file, "");
	SysConfig cfg(config_file, &os);
	cfg.parseConfig();
	SysResources resources(cfg);
	ASSERT_EQUAL("dpi configured", 120.0, cfg.getDpi());

	// init xrm_db in order for calls to take effect
	X11::loadXrmResources("");

	resources.setXResourceDpi();
	XrmResourceCbCollect collect;
	X11::enumerateXrmResources(&collect);
	ASSERT_EQUAL("config resource", 1, collect.size());
	ASSERT_EQUAL("DPI", "Xft.dpi", collect.begin()->first);
	ASSERT_EQUAL("DPI", "120", collect.begin()->second);
}

void
TestSysResources::testSetConfiguredXResources()
{
	OsMock os;
	SysConfig cfg("", &os);
	SysResources resources(cfg);

	// init xrm_db in order for calls to take effect
	X11::loadXrmResources("");

	std::map<std::string, std::string> x_resources;
	x_resources["XTerm.background"] = "#666666";
	cfg.setXResources(TIME_OF_DAY_DAY, x_resources);

	// initial set of resources, theme overriding background and setting
	// new foreground
	x_resources["XTerm.background"] = "#000000";
	x_resources["XTerm.foreground"] = "#ffffff";
	cfg.setThemeXResources(x_resources);
	resources.setConfiguredXResources(TIME_OF_DAY_DAY);

	XrmResourceCbCollect collect1;
	X11::enumerateXrmResources(&collect1);
	ASSERT_EQUAL("config resource", 2, collect1.size());
	collect1.sort();
	ASSERT_EQUAL("bg", "XTerm.background", collect1.begin()->first);
	ASSERT_EQUAL("bg", "#000000", collect1.begin()->second);
	ASSERT_EQUAL("fg", "XTerm.foreground", (collect1.begin() + 1)->first);
	ASSERT_EQUAL("fg", "#ffffff", (collect1.begin() + 1)->second);

	// switch theme, no longer contains any xresources, original background
	// is used and foreground is removed
	x_resources.clear();
	cfg.setThemeXResources(x_resources);
	resources.setConfiguredXResources(TIME_OF_DAY_DAY);
	XrmResourceCbCollect collect2;
	X11::enumerateXrmResources(&collect2);
	ASSERT_EQUAL("config resource", 1, collect2.size());
	collect2.sort();
	ASSERT_EQUAL("bg", "XTerm.background", collect2.begin()->first);
	ASSERT_EQUAL("bg", "#666666", collect2.begin()->second);
}
