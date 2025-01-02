//
// test_Geometry.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Geometry.hh"

class TestGeometry : public TestSuite {
public:
	TestGeometry();
	virtual ~TestGeometry();

	virtual bool run_test(TestSpec, bool status);

private:
	static void testFromCordinates();
	static void testCenter();
	static void testArea();
	static void testIsOverlap();
};

TestGeometry::TestGeometry()
	: TestSuite("Geometry")
{
}

TestGeometry::~TestGeometry()
{
}

bool
TestGeometry::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "fromCordinates", testFromCordinates());
	TEST_FN(spec, "center", testCenter());
	TEST_FN(spec, "area", testArea());
	TEST_FN(spec, "isOverlap", testIsOverlap());
	return status;
}

void
TestGeometry::testFromCordinates()
{
	Geometry e1(10, 20, 100, 200);
	Geometry a1 = Geometry::fromCordinates(10, 20, 110, 220);
	ASSERT_EQUAL("from coordinates", e1, a1);
}

void
TestGeometry::testCenter()
{
	Geometry head(100, 200, 1000, 2000);

	ASSERT_EQUAL("center same size", head, head.center(head));
	ASSERT_EQUAL("center smaller",
		     Geometry(550, 1100, 100, 200),
		     head.center(Geometry(0, 0, 100, 200)));
	ASSERT_EQUAL("center larger",
		     Geometry(-400, -800, 2000, 4000),
		     head.center(Geometry(0, 0, 2000, 4000)));
}

void
TestGeometry::testArea()
{
	Geometry head(0, 0, 0, 0);
	ASSERT_EQUAL("empty", 0, head.area());

	head.width = 10;
	ASSERT_EQUAL("width only", 0, head.area());

	head.height = 10;
	ASSERT_EQUAL("width + height", 100, head.area());
}

void
TestGeometry::testIsOverlap()
{
	Geometry g1(100, 100, 100, 100);
	Geometry g2(0, 0, 50, 50);
	ASSERT_FALSE("no overlap", g1.isOverlap(g2));
	ASSERT_FALSE("no overlap", g2.isOverlap(g1));

	Geometry g3(50, 0, 100, 50);
	ASSERT_FALSE("no overlap, x only ", g1.isOverlap(g3));
	ASSERT_FALSE("no overlap, x only", g3.isOverlap(g1));

	Geometry g4(0, 50, 50, 100);
	ASSERT_FALSE("no overlap, y only ", g1.isOverlap(g4));
	ASSERT_FALSE("no overlap, y only", g4.isOverlap(g1));

	Geometry g5(140, 140, 20, 20);
	ASSERT_TRUE("overlap ", g1.isOverlap(g5));
	ASSERT_TRUE("overlap", g5.isOverlap(g1));

	Geometry g6(0, 24, 654, 1056);
	ASSERT_FALSE("on edge", g6.isOverlap(Geometry(654, 24, 1262, 1056)));
	ASSERT_FALSE("on edge", g6.isOverlap(Geometry(0, 0, 1920, 24)));
}

class TestGeometryOverlap : public TestSuite {
public:
	TestGeometryOverlap();
	virtual ~TestGeometryOverlap();

	virtual bool run_test(TestSpec, bool status);

private:
	static void testGetOverlapPercent();
};

TestGeometryOverlap::TestGeometryOverlap()
	: TestSuite("GeometryOverlap")
{
}

TestGeometryOverlap::~TestGeometryOverlap()
{
}

bool
TestGeometryOverlap::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "getOverlapPercent", testGetOverlapPercent());
	return status;
}

void
TestGeometryOverlap::testGetOverlapPercent()
{
	Geometry base(100, 200, 300, 400);
	GeometryOverlap overlap(base);

	// base geometry only
	ASSERT_EQUAL("base geometry", 0, overlap.getOverlapPercent());

	// add non-overlap geometry
	Geometry no_overlap(0, 100, 50, 50);
	ASSERT_FALSE("non overlap", overlap.addOverlap(no_overlap));
	ASSERT_EQUAL("non overlap", 0, overlap.getOverlapPercent());

	// add fully covered geometry
	Geometry all_overlap(0, 0, 500, 800);
	ASSERT_TRUE("all overlap", overlap.addOverlap(all_overlap));
	ASSERT_EQUAL("all overlap", 100, overlap.getOverlapPercent());

	// add overlap full width top
	overlap = GeometryOverlap(base);
	Geometry top_fw(50, 150, 400, 150);
	ASSERT_TRUE("overlap full top", overlap.addOverlap(top_fw));
	ASSERT_EQUAL("overlap full top", 25, overlap.getOverlapPercent());

	// add overlap full width bottom
	overlap = GeometryOverlap(base);
	Geometry bottom_fw(50, 500, 400, 150);
	ASSERT_TRUE("overlap full bottom", overlap.addOverlap(bottom_fw));
	ASSERT_EQUAL("overlap full bottom", 25, overlap.getOverlapPercent());

	// add overlap full width middle
	overlap = GeometryOverlap(base);
	Geometry middle_fw(50, 300, 400, 100);
	ASSERT_TRUE("overlap full middle", overlap.addOverlap(middle_fw));
	ASSERT_EQUAL("overlap full middle", 25, overlap.getOverlapPercent());

	// add center geometry
	overlap = GeometryOverlap(Geometry(0, 0, 300, 300));
	Geometry center(100, 100, 100, 100);
	ASSERT_TRUE("overlap center", overlap.addOverlap(center));
	ASSERT_EQUAL("overlap center", 11, overlap.getOverlapPercent());

	overlap = GeometryOverlap(Geometry(0, 100, 300, 300));
	Geometry middle_top(0, 0, 100, 200);
	ASSERT_TRUE("middle top", overlap.addOverlap(middle_top));
	ASSERT_EQUAL("middle top", 11, overlap.getOverlapPercent());

	overlap = GeometryOverlap(Geometry(0, 0, 300, 300));
	Geometry middle_bottom(0, 200, 100, 200);
	ASSERT_TRUE("middle bottom", overlap.addOverlap(middle_bottom));
	ASSERT_EQUAL("middle bottom", 11, overlap.getOverlapPercent());

	// add square on the left
	//     --------
	//     |      |
	//  --------  |
	//  |      |  |
	//  --------  |
	//     |      |
	//     --------
	overlap = GeometryOverlap(Geometry(100, 0, 300, 300));
	Geometry middle_left(0, 100, 200, 100);
	ASSERT_TRUE("middle left", overlap.addOverlap(middle_left));
	ASSERT_EQUAL("middle left", 11, overlap.getOverlapPercent());

	// add square on the right
	// --------
	// |      |
	// |   --------
	// |   |      |
	// |   --------
	// |      |
	// --------
	overlap = GeometryOverlap(Geometry(0, 0, 300, 300));
	Geometry middle_right(200, 100, 200, 100);
	ASSERT_TRUE("middle right", overlap.addOverlap(middle_right));
	ASSERT_EQUAL("middle right", 11, overlap.getOverlapPercent());
}
