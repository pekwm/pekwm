//
// test_PFontPango.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "tk/PFontPango.hh"

class MockPFontPango : public PFontPango {
public:
	MockPFontPango(const PFont::Descr& descr)
		: PFontPango(),
		  _native(PFontPango::toNativeDescr(descr))
	{
	}
	virtual ~MockPFontPango() { }

	const std::string& getNative() const { return _native; }

	virtual uint getWidth(const std::string&, uint chars=0) { return 0; }
	virtual void setColor(PFont::Color*) { }

private:
	virtual void drawText(PSurface*, int, int, const std::string&,
			      uint, bool) { }

private:
	std::string _native;
};

class TestPFontPango : public TestSuite {
public:
	TestPFontPango(void);
	~TestPFontPango(void);

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

TestPFontPango::TestPFontPango(void)
	: TestSuite("PFontPango")
{
}

TestPFontPango::~TestPFontPango(void)
{
}

bool
TestPFontPango::run_test(TestSpec spec, bool status)
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
TestPFontPango::testToNativeDescrFamily()
{
	MockPFontPango font(PFont::Descr("Sans", false));
	ASSERT_EQUAL("family", "Sans", font.getNative());
}

void
TestPFontPango::testToNativeDescrStyle()
{
	MockPFontPango font(PFont::Descr(":slant=italic", false));
	ASSERT_EQUAL("style", "italic", font.getNative());
}

void
TestPFontPango::testToNativeDescrWeight()
{
	MockPFontPango font(PFont::Descr(":weight=extrabold", false));
	ASSERT_EQUAL("weight", "Extra-Bold", font.getNative());
}

void
TestPFontPango::testToNativeDescrStretch()
{
	MockPFontPango font(PFont::Descr(":width=semicondensed", false));
	ASSERT_EQUAL("stretch", "Semi-Condensed", font.getNative());
}

void
TestPFontPango::testToNativeDescrSize()
{
	MockPFontPango font(PFont::Descr(":size=12", false));
	ASSERT_EQUAL("size", "12", font.getNative());
}

void
TestPFontPango::testToNativeDescrFull()
{
	PFont::Descr descr("Sans-10:weight=light:width=condensed", false);
	MockPFontPango font(descr);
	ASSERT_EQUAL("full", "Sans Light Condensed 10", font.getNative());
}
