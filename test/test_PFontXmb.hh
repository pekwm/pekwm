//
// test_PFontXmb.hh for pekwm
// Copyright (C) 2023-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "tk/PFontXmb.hh"

class MockPFontXmb : public PFontXmb {
public:
	MockPFontXmb(const PFont::Descr& descr, PPixmapSurface &surface)
		: PFontXmb(1.0, surface),
		  _native(PFontXmb::toNativeDescr(descr))
	{
	}
	virtual ~MockPFontXmb() { }

	const std::string& getNative() const { return _native; }

	virtual uint getWidth(const std::string&, uint chars=0) { return 0; }
	virtual void setColor(PFont::Color*) { }

private:
	virtual bool doLoadFont(const std::string& spec) { return true; }
	virtual void doUnloadFont() { }

	virtual void drawText(PSurface*, int, int, const std::string&,
			      uint, bool) { }
	virtual int doGetWidth(const std::string &text, int size) const
	{
		return 0;
	}
	virtual int doGetHeight() const { return 0; }

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
	PPixmapSurface surface;
	MockPFontXmb font(PFont::Descr("Sans", false), surface);
	ASSERT_EQUAL("family", "-*-helvetica-medium-r-*-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrStyle()
{
	PPixmapSurface surface;
	MockPFontXmb font(PFont::Descr(":slant=italic", false), surface);
	ASSERT_EQUAL("style", "-*-helvetica-medium-i-*-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrWeight()
{
	PPixmapSurface surface;
	MockPFontXmb font(PFont::Descr(":weight=extrabold", false), surface);
	ASSERT_EQUAL("weight", "-*-helvetica-bold-r-*-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrStretch()
{
	PPixmapSurface surface;
	MockPFontXmb font(PFont::Descr(":width=semicondensed", false), surface);
	ASSERT_EQUAL("stretch",
		     "-*-helvetica-medium-r-semicondensed-*-12-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrSize()
{
	PPixmapSurface surface;
	MockPFontXmb font(PFont::Descr(":size=22", false), surface);
	ASSERT_EQUAL("size", "-*-helvetica-medium-r-*-*-22-*-*-*-*-*-*-*",
		     font.getNative());
}

void
TestPFontXmb::testToNativeDescrFull()
{
	PPixmapSurface surface;
	PFont::Descr descr("Arial-10:weight=light:width=condensed", false);
	MockPFontXmb font(descr, surface);
	ASSERT_EQUAL("full",
		     "-*-arial-light-r-condensed-*-10-*-*-*-*-*-*-*",
		     font.getNative());
}
