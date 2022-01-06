//
// test_FontHandler.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "FontHandler.hh"

class TestFontHandler : public TestSuite,
			public FontHandler {
public:
	TestFontHandler()
		: TestSuite("FontHandler"),
		  FontHandler(false, "")
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	void testReplaceFontCharset(void);
};

bool
TestFontHandler::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "replaceFontCharset", testReplaceFontCharset());
	return status;
}

void
TestFontHandler::testReplaceFontCharset(void)
{
	std::string font;

	font = "";
	ASSERT_EQUAL("empty string", false, replaceFontCharset(font));
	ASSERT_EQUAL("empty string", "", font);

	std::string fixed =
		"-misc-fixed-medium-r-normal-*-12-*-75-75-*-*-jisx0201.1976-0";
	font = fixed;
	ASSERT_EQUAL("empty replace", false, replaceFontCharset(font));
	ASSERT_EQUAL("empty replace", fixed, font);

	setCharsetOverride("iso8859-1");
	std::string fixed_iso8869_1 =
		"-misc-fixed-medium-r-normal-*-12-*-75-75-*-*-iso8859-1";
	ASSERT_EQUAL("replace charset", true, replaceFontCharset(font));
	ASSERT_EQUAL("replace charset", fixed_iso8869_1, font);

	font = "fixed";
	ASSERT_EQUAL("replace no charset", false, replaceFontCharset(font));
	ASSERT_EQUAL("replace no charset", "fixed", font);

	font = "-";
	ASSERT_EQUAL("single -", false, replaceFontCharset(font));
	ASSERT_EQUAL("single -", "-", font);

	font = "--";
	ASSERT_EQUAL("replace only charset", true, replaceFontCharset(font));
	ASSERT_EQUAL("replace only charset", "-iso8859-1", font);
}
