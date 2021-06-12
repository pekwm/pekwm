//
// test_X11.cc for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "X11.hh"

class TestX11 : public X11,
                public TestSuite {
public:
	TestX11(void);
	virtual ~TestX11(void);

	virtual bool run_test(TestSpec spec, bool status);

	static void testParseGeometry(void);
	static void assertParseGeometry(std::string msg, std::string str,
					Geometry e_gm, int e_mask);
	static void testParseGeometryVal(void);
	static void assertParseGeometryVal(std::string msg, std::string str,
					   int e_ret, int e_val);
};

TestX11::TestX11(void)
	: X11(),
	  TestSuite("X11")
{
}

TestX11::~TestX11(void)
{
}

bool
TestX11::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parseGeometry", testParseGeometry());
	TEST_FN(spec, "parseGeometryVal", testParseGeometryVal());
	return status;
}

void
TestX11::testParseGeometry(void)
{
	assertParseGeometry("short", "",
			    Geometry(0, 0, 1, 1), 0);

	assertParseGeometry("size", "10x20",
			    Geometry(0, 0, 10, 20), WIDTH_VALUE | HEIGHT_VALUE);
	assertParseGeometry("size prefix", "=20x30",
			    Geometry(0, 0, 20, 30), WIDTH_VALUE | HEIGHT_VALUE);
	assertParseGeometry("size", "30%x40%",
			    Geometry(0, 0, 30, 40),
			    WIDTH_VALUE | WIDTH_PERCENT
			    | HEIGHT_VALUE | HEIGHT_PERCENT);
	assertParseGeometry("position", "+30-40",
			    Geometry(30, 40, 1, 1),
			    X_VALUE | Y_VALUE | Y_NEGATIVE);
	assertParseGeometry("position %", "+50%+60%",
			    Geometry(50, 60, 1, 1),
			    X_VALUE | X_PERCENT | Y_VALUE | Y_PERCENT);

	assertParseGeometry("full", "=10%x200+300-20%",
			    Geometry(300, 20, 10, 200),
			    X_VALUE | Y_VALUE | WIDTH_VALUE | HEIGHT_VALUE |
			    WIDTH_PERCENT | Y_NEGATIVE | Y_PERCENT);
}

void
TestX11::assertParseGeometry(std::string msg, std::string str,
                             Geometry e_gm, int e_mask)
{
	Geometry gm;
	int mask = parseGeometry(str, gm);
	ASSERT_EQUAL(msg + " mask", e_mask, mask);
	ASSERT_EQUAL(msg + " x", e_gm.x, gm.x);
	ASSERT_EQUAL(msg + " y", e_gm.y, gm.y);
	ASSERT_EQUAL(msg + " width", e_gm.width, gm.width);
	ASSERT_EQUAL(msg + " height", e_gm.height, gm.height);
}

void
TestX11::testParseGeometryVal(void)
{
	assertParseGeometryVal("invalid", "invalid", 0, 0);
	assertParseGeometryVal("number", "1", 1, 1);
	assertParseGeometryVal("percent", "10%", 2, 10);
}

void
TestX11::assertParseGeometryVal(std::string msg, std::string str,
                                int e_ret, int e_val)
{
	int ret, val;
	ret = parseGeometryVal(str.c_str(), str.c_str() + str.size(), val);
	ASSERT_EQUAL(msg + " ret", e_ret, ret);
	ASSERT_EQUAL(msg + " val", e_val, val);
}
