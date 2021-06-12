//
// test_Frame.cc for pekwm
// Copyright (C) 2020 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Frame.hh"

class TestFrame : public Frame,
                  public TestSuite {
public:
	TestFrame(void);
	~TestFrame(void);

	virtual bool run_test(TestSpec spec, bool status);

	static void testApplyGeometry(void);
	static void assertApplyGeometry(std::string msg,
					Geometry gm,
					const Geometry &apply_gm, int mask,
					const Geometry &screen_gm,
					const Geometry &e_gm);
};

TestFrame::TestFrame(void)
	: Frame(),
	  TestSuite("Frame")
{
}

TestFrame::~TestFrame(void)
{
}

bool
TestFrame::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "applyGeometry", testApplyGeometry());
	return status;
}

void
TestFrame::testApplyGeometry(void)
{
	assertApplyGeometry("negative X pos (50)",
			    Geometry(), Geometry(50, 0, 102, 102),
			    X_VALUE | Y_VALUE | WIDTH_VALUE
			    | HEIGHT_VALUE | X_NEGATIVE,
			    Geometry(0, 0, 1024, 768),
			    // expected
			    Geometry(1024 - 102 - 50, 0, 102, 102));

	assertApplyGeometry("negative Y pos (100)",
			    Geometry(), Geometry(0, 100, 102, 102),
			    X_VALUE | Y_VALUE | WIDTH_VALUE
			    | HEIGHT_VALUE | Y_NEGATIVE,
			    Geometry(0, 0, 1024, 768),
			    // expected
			    Geometry(0, 768 - 102 - 100, 102, 102));

	assertApplyGeometry("percent size",
			    Geometry(10, 10, 100, 100), Geometry(0, 0, 50, 50),
			    WIDTH_VALUE | HEIGHT_VALUE
			    | WIDTH_PERCENT | HEIGHT_PERCENT,
			    Geometry(0, 0, 500, 400),
			    // expected
			    Geometry(10, 10, 250, 200));

	assertApplyGeometry("percent position",
			    Geometry(0, 0, 100, 100), Geometry(10, 20, 0, 0),
			    X_VALUE | Y_VALUE | X_PERCENT | Y_PERCENT,
			    Geometry(0, 0, 500, 400),
			    // expected
			    Geometry(50, 80, 100, 100));
}

void
TestFrame::assertApplyGeometry(std::string msg,
                               Geometry gm,
                               const Geometry &apply_gm, int mask,
                               const Geometry &screen_gm,
                               const Geometry &e_gm)
{
	applyGeometry(gm, apply_gm, mask, screen_gm);
	ASSERT_EQUAL(msg + " x", e_gm.x, gm.x);
	ASSERT_EQUAL(msg + " y", e_gm.y, gm.y);
	ASSERT_EQUAL(msg + " width", e_gm.width, gm.width);
	ASSERT_EQUAL(msg + " height", e_gm.height, gm.height);
}
