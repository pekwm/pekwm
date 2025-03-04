//
// test_PImage.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "tk/PImage.hh"

class TestPImage : public TestSuite {
public:
	TestPImage()
		: TestSuite("PImage")
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	void testGetScaledDataPixel();
};

bool
TestPImage::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "getScaledDataPixel", testGetScaledDataPixel());
	return status;
}

void
TestPImage::testGetScaledDataPixel()
{
}
