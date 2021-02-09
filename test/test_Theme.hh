//
// test_Theme.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Theme.hh"

class TestTheme : public TestSuite {
public:
    TestTheme()
        : TestSuite("Theme")
    {
        register_test("colorMapLoad", TestTheme::testColorMapLoad);
    }

    static void testColorMapLoad(void)
    {
        CfgParser parser;
        parser.parse("../test/data/theme_colormap.cfg");
        auto section = parser.getEntryRoot()->findSection("COLORMAPS");
        ASSERT_EQUAL("environment error", true, section != nullptr);

        auto it = section->begin();
        Theme::ColorMap color_map;
        ASSERT_EQUAL("load failed", true, color_map.load((*it)->getSection()));
        ASSERT_EQUAL("invalid count", 2, color_map.size());
    }
};
