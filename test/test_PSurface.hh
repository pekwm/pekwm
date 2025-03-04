//
// test_PSurface.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "tk/PSurface.hh"

class PSurfaceTest : public PSurface {
public:
	PSurfaceTest(int x, int y, uint width, uint height)
		: _x(x),
		  _y(y),
		  _width(width),
		  _height(height)
	{
	}

	virtual Drawable getDrawable() const { return None; }
	virtual int getX() const { return _x; }
	virtual int getY() const { return _y; }
	virtual uint getWidth() const { return _width; }
	virtual uint getHeight() const { return _height; }

private:
	int _x;
	int _y;
	uint _width;
	uint _height;
};

class TestPSurface : public TestSuite {
public:
	TestPSurface()
		: TestSuite("PSurface")
	{
	}
	virtual ~TestPSurface()
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testClipAbs();
};

bool
TestPSurface::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "clip abs", testClipAbs());
	return status;
}

void
TestPSurface::testClipAbs()
{
	PSurfaceTest surface(0, 0, 100, 200);

	uint width, height;

	width = 100;
	height = 200;
	ASSERT_TRUE("full size", surface.clip(0, 0, width, height));
	ASSERT_EQUAL("full width", 100, width);
	ASSERT_EQUAL("full height", 200, height);

	ASSERT_TRUE("clip width", surface.clip(50, 0, width, height));
	ASSERT_EQUAL("clipped width", 50, width);
	ASSERT_EQUAL("full height", 200, height);

	width = 100;
	ASSERT_TRUE("clip height", surface.clip(0, 100, width, height));
	ASSERT_EQUAL("full width", 100, width);
	ASSERT_EQUAL("clip height", 100, height);

	ASSERT_FALSE("x outside", surface.clip(110, 0, width, height));
	ASSERT_FALSE("x outside", surface.clip(0, 210, width, height));
}
