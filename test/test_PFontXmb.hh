//
// test_PFontXmb.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "tk/PFontXmb.hh"

class MockPFontXmb : public PFontXmb {
public:
	MockPFontXmb(const PFont::Descr& descr)
		: PFontXmb(),
		  _native(PFontXmb::toNativeDescr(descr))
	{
	}
	virtual ~MockPFontXmb() { }

	const std::string& getNative() const { return _native; }

	virtual uint getWidth(const std::string&, uint chars=0) { return 0; }
	virtual void setColor(PFont::Color*) { }

private:
	virtual void drawText(PSurface*, int, int, const std::string&,
			      uint, bool) { }

private:
	std::string _native;
};

class TestPFontXmb : public TestSuite {
public:
	TestPFontXmb(void);
	~TestPFontXmb(void);

	bool run_test(TestSpec spec, bool status);

private:
	// toNativeDescr
	static void testToNativeDescrFamily();
	static void testToNativeDescrStyle();
	static void testToNativeDescrWeight();
	static void testToNativeDescrStretch();
	static void testToNativeDescrSize();
	static void testToNativeDescrFull();
};

TestPFontXmb::TestPFontXmb(void)
	: TestSuite("PFontXmb")
{
}

TestPFontXmb::~TestPFontXmb(void)
{
}

bool
TestPFontXmb::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "toNativeDescrFamily", testToNativeDescrFamily());
	TEST_FN(spec, "toNativeDescrStyle", testToNativeDescrStyle());
	TEST_FN(spec, "toNativeDescrWeight", testToNativeDescrWeight());
	TEST_FN(spec, "toNativeDescrStretch", testToNativeDescrStretch());
	TEST_FN(spec, "toNativeDescrSize", testToNativeDescrSize());
	TEST_FN(spec, "toNativeDescrFull", testToNativeDescrFull());
	return status;
}

void
TestPFontXmb::testToNativeDescrFamily()
{
	MockPFontXmb font(PFont::Descr("Sans", false));
	ASSERT_EQUAL("family", "-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrStyle()
{
	MockPFontXmb font(PFont::Descr(":slant=italic", false));
	ASSERT_EQUAL("style", "-*-helvetica-medium-i-*-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrWeight()
{
	MockPFontXmb font(PFont::Descr(":weight=extrabold", false));
	ASSERT_EQUAL("weight", "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrStretch()
{
	MockPFontXmb font(PFont::Descr(":width=semicondensed", false));
	ASSERT_EQUAL("stretch",
		     "-*-helvetica-medium-r-semicondensed-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrSize()
{
	MockPFontXmb font(PFont::Descr(":size=22", false));
	ASSERT_EQUAL("size", "-*-helvetica-medium-r-*-*-22-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrFull()
{
	PFont::Descr descr("Arial-10:weight=light:width=condensed", false);
	MockPFontXmb font(descr);
	ASSERT_EQUAL("full",
		     "-*-arial-light-r-condensed-*-10-*-*-*-*-*-*-*",
		     font.getNative());
}
