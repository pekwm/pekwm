//
// test_Theme.hh for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "tk/Theme.hh"

class TestTheme : public TestSuite,
		  public Theme {
public:
	TestTheme()
		: TestSuite("Theme")
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testColorMapLoad();
	static void testColorMapLoadInvalid();

private:
	static bool loadColorMap(const char *cfg,
				 std::map<int, int> &color_map);

};

bool
TestTheme::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "colorMapLoad", testColorMapLoad());
	TEST_FN(spec, "colorMapLoadInvalid", testColorMapLoadInvalid());
	return status;
}

void
TestTheme::testColorMapLoad(void)
{
	const char *cfg =
		"ColorMaps {\n"
		"  ColorMap = \"Light\" {\n"
		"    Map = \"#aaaaaa\" { To = \"#eeeeee\" }\n"
		"    Map = \"#cccccc\" { To = \"#ffffff\" }\n"
		"  }\n"
		"}\n";

	std::map<int, int> color_map;
	ASSERT_EQUAL("load failed", true, loadColorMap(cfg, color_map));
	ASSERT_EQUAL("invalid count", 2, color_map.size());
}

void
TestTheme::testColorMapLoadInvalid(void)
{
	const char *cfg =
		"ColorMaps {\n"
		"  ColorMap = \"Invalid\" {\n"
		"    Map = \"#aaaaaa\" { To = \"#eeeee\" }\n"
		"    Map = \"#ccccc\" { To = \"#ffffff\" }\n"
		"    Map = \"\" { To = \"\" }\n"
		"  }\n"
		"}\n";

	std::map<int, int> color_map;
	ASSERT_EQUAL("load failed", true, loadColorMap(cfg, color_map));
	ASSERT_EQUAL("invalid count", 0, color_map.size());
}

bool
TestTheme::loadColorMap(const char *cfg, std::map<int, int> &color_map)
{
	CfgParserSourceString *source =
		new CfgParserSourceString(":memory:", cfg);

	CfgParser parser(CfgParserOpt(""));
	parser.parse(source);
	CfgParser::Entry *section =
		parser.getEntryRoot()->findSection("COLORMAPS");
	ASSERT_EQUAL("environment error", true, section != nullptr);

	CfgParser::Entry::entry_cit it = section->begin();
	return Theme::ColorMap::load((*it)->getSection(), color_map);
}
