//
// test_ColorPalette.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "tk/ColorPalette.hh"

extern "C" {
#include <string.h>
}

class TestColorPalette : public TestSuite {
public:
	TestColorPalette();
	~TestColorPalette();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testGetColorsAsStrings();
	static void testGetColorsInputValidation();
};

TestColorPalette::TestColorPalette(void)
	: TestSuite("ColorPalette")
{
}

TestColorPalette::~TestColorPalette(void)
{
}

bool
TestColorPalette::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "getColorsAsStrings", testGetColorsAsStrings());
	TEST_FN(spec, "getColorsInputValidation",
		testGetColorsInputValidation());
	return status;
}

void
TestColorPalette::testGetColorsAsStrings()
{
	bool res;
	std::vector<std::string> strs;
	res = ColorPalette::getColors(ColorPalette::PALETTE_COMPLEMENTARY,
				      ColorPalette::BASE_COLOR_RED, 0, 1.0,
				      strs);
	ASSERT_TRUE("complementary", res);
	ASSERT_EQUAL("complementary", 2, strs.size());
	ASSERT_EQUAL("complementary", "#fbd3d0", strs[0]);
	ASSERT_EQUAL("complementary", "#bfdfd0", strs[1]);

	strs.clear();
	res = ColorPalette::getColors(ColorPalette::PALETTE_TETRAD,
				      ColorPalette::BASE_COLOR_ORANGE, 4,
				      1.0, strs);
	ASSERT_TRUE("tetrad wrap", res);
	ASSERT_EQUAL("tetrad wrap", 4, strs.size());
	ASSERT_EQUAL("tetrad wrap", "#9a4f06", strs[0]);
	ASSERT_EQUAL("tetrad wrap", "#37075c", strs[1]);
	ASSERT_EQUAL("tetrad wrap", "#013c73", strs[2]);
	ASSERT_EQUAL("tetrad wrap", "#a29600", strs[3]);

	strs.clear();
	res = ColorPalette::getColors(ColorPalette::PALETTE_SINGLE,
				      ColorPalette::BASE_COLOR_PURPLE, 0,
				      1.0, strs);
	ASSERT_TRUE("single", res);
	ASSERT_EQUAL("single", 1, strs.size());
	ASSERT_EQUAL("single", "#b8add5", strs[0]);

	// brightness
	strs.clear();
	res = ColorPalette::getColors(ColorPalette::PALETTE_SINGLE,
				      ColorPalette::BASE_COLOR_PURPLE, 0,
				      0.5, strs);
	ASSERT_TRUE("single", res);
	ASSERT_EQUAL("single", 1, strs.size());
	ASSERT_EQUAL("single", "#5c566a", strs[0]);

}

void
TestColorPalette::testGetColorsInputValidation()
{
	bool res;
	std::vector<std::string> strs;
	res = ColorPalette::getColors(ColorPalette::PALETTE_NO,
				      ColorPalette::BASE_COLOR_RED, 0, 1.0,
				      strs);
	ASSERT_FALSE("invalid mode", res);
	ASSERT_EQUAL("invalid mode", 0, strs.size());

	res = ColorPalette::getColors(ColorPalette::PALETTE_TRIAD,
				      ColorPalette::BASE_COLOR_NO, 0, 1.0,
				      strs);
	ASSERT_FALSE("invalid base color", res);
	ASSERT_EQUAL("invalid mode", 0, strs.size());

	res = ColorPalette::getColors(ColorPalette::PALETTE_TRIAD,
				      ColorPalette::BASE_COLOR_RED, 10, 1.0,
				      strs);
	ASSERT_FALSE("invalid intensity", res);
	ASSERT_EQUAL("invalid mode", 0, strs.size());
}
