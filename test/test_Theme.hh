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
        const char *colormap_cfg =
            "ColorMaps {\n"
            "  ColorMap = \"Light\" {\n"
            "    Map = \"#aaaaaa\" { To = \"#eeeeee\" }\n"
            "    Map = \"#cccccc\" { To = \"#ffffff\" }\n"
            "  }\n"
            "}\n";

        auto source = new CfgParserSourceString(":memory:", colormap_cfg);

        CfgParser parser;
        parser.parse(source);
        auto section = parser.getEntryRoot()->findSection("COLORMAPS");
        ASSERT_EQUAL("environment error", true, section != nullptr);

        auto it = section->begin();
        std::map<int, int> color_map;
        ASSERT_EQUAL("load failed", true,
                     Theme::ColorMap::load((*it)->getSection(), color_map));
        ASSERT_EQUAL("invalid count", 2, color_map.size());
    }
};
