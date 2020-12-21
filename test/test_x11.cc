//
// test_x11.cc for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "x11.hh"

class TestX11 : public X11 {
public:
    static void testParseGeometry(void) {
        assertParseGeometry("short", "",
                            Geometry(0, 0, 1, 1), 0);

        assertParseGeometry("size", "10x20",
                            Geometry(0, 0, 10, 20), WIDTH_VALUE | HEIGHT_VALUE);
        assertParseGeometry("size prefix", "=20x30",
                            Geometry(0, 0, 20, 30), WIDTH_VALUE | HEIGHT_VALUE);
        assertParseGeometry("size", "30%x40%",
                            Geometry(0, 0, 30, 40),
                            WIDTH_VALUE | WIDTH_PERCENT | HEIGHT_VALUE | HEIGHT_PERCENT);
        assertParseGeometry("position", "+30-40",
                            Geometry(30, 40, 1, 1), X_VALUE | Y_VALUE | Y_NEGATIVE);
        assertParseGeometry("position %", "+50%+60%",
                            Geometry(50, 60, 1, 1),
                            X_VALUE | X_PERCENT | Y_VALUE | Y_PERCENT);

        assertParseGeometry("full", "=10%x200+300-20%",
                            Geometry(300, 20, 10, 200),
                            X_VALUE | Y_VALUE | WIDTH_VALUE | HEIGHT_VALUE |
                            WIDTH_PERCENT | Y_NEGATIVE | Y_PERCENT);
    }

    static void assertParseGeometry(std::string msg, std::string str,
                                    Geometry e_gm, int e_mask)
    {
        Geometry gm;
        int mask = parseGeometry(str, gm);
        ASSERT_EQUAL("parseGeometry", msg + " mask", e_mask, mask);
        ASSERT_EQUAL("parseGeometry", msg + " x", e_gm.x, gm.x);
        ASSERT_EQUAL("parseGeometry", msg + " y", e_gm.y, gm.y);
        ASSERT_EQUAL("parseGeometry", msg + " width", e_gm.width, gm.width);
        ASSERT_EQUAL("parseGeometry", msg + " height", e_gm.height, gm.height);
    }

    static void testParseGeometryVal(void) {
        assertParseGeometryVal("invalid", "invalid", 0, 0);
        assertParseGeometryVal("number", "1", 1, 1);
        assertParseGeometryVal("percent", "10%", 2, 10);
    }

    static void assertParseGeometryVal(std::string msg, std::string str,
                                       int e_ret, int e_val) {
        int ret, val;
        ret = parseGeometryVal(str.c_str(), str.c_str() + str.size(), val);
        ASSERT_EQUAL("parseGeometryVal", msg + " ret", e_ret, ret);
        ASSERT_EQUAL("parseGeometryVal", msg + " val", e_val, val);
    }
};

int
main(int argc, char *argv[])
{
    try {
        TestX11::testParseGeometryVal();
        TestX11::testParseGeometry();
        return 0;
    } catch (AssertFailed &ex) {
        std::cerr << ex.file() << ":" << ex.line() << " " << ex.msg() << std::endl;
        return 1;
    }
}
