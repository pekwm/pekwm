//
// test_pekwm_panel_sysinfo.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"

#define UNITTEST
#include "../src/panel/pekwm_panel_sysinfo.c"

class TestPekwmPanelSysinfo : public TestSuite {
public:
	TestPekwmPanelSysinfo()
		: TestSuite("PekwmPanelSysinfo")
	{
	}
	virtual ~TestPekwmPanelSysinfo()
	{
	}

	virtual bool run_test(TestSpec spec, bool status)
	{
		TEST_FN(spec, "read_sysinfo", testReadSysinfo());
		TEST_FN(spec, "print_sysinfo", testPrintSysinfo());
		TEST_FN(spec, "linux, /proc/meminfo", testLinuxProcMeminfo());
		return status;
	}

private:
	static void testReadSysinfo();
	static void testPrintSysinfo();
	static void testLinuxProcMeminfo();
};

void
TestPekwmPanelSysinfo::testReadSysinfo()
{
	struct pekwm_panel_sysinfo info = {};
	_read_sysinfo(&info);
}

void
TestPekwmPanelSysinfo::testPrintSysinfo()
{
	struct pekwm_panel_sysinfo info = {};
	_print_sysinfo(&info);
}

void
TestPekwmPanelSysinfo::testLinuxProcMeminfo()
{
	struct pekwm_panel_sysinfo info = {};
	// using an actual file as fmemopen is available in:
	// _POSIX_C_SOURCE >= 200809
	FILE *fp = fopen("../test/data/linux_proc_meminfo", "r");
	_linux_read_proc_meminfo(&info, fp);
	fclose(fp);

	ASSERT_EQUAL("ram_kb", 32793796, info.ram_kb);
	ASSERT_EQUAL("free_ram_kb", 2991568, info.free_ram_kb);
	ASSERT_EQUAL("cache_ram_kb", 22400008, info.cache_ram_kb);
	ASSERT_EQUAL("swap_kb", 33554428, info.swap_kb);
	ASSERT_EQUAL("free_swap_kb", 33554428, info.free_swap_kb);
}

int
main(int argc, char *argv[])
{
	TestPekwmPanelSysinfo sysinfo;
	return TestSuite::main(argc, argv);
}
