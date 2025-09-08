//
// test_Md5.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Md5.hh"

extern "C" {
#include <string.h>
}

class TestMd5 : public TestSuite {
public:
	TestMd5();
	~TestMd5();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testString();
	static void testStringView();
	static void testUpdateFinalize();
};

TestMd5::TestMd5()
	: TestSuite("Md5")
{
}

TestMd5::~TestMd5()
{
}

bool
TestMd5::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "String", testString());
	TEST_FN(spec, "StringView", testStringView());
	TEST_FN(spec, "UpdateFinalize", testUpdateFinalize());
	return status;
}

void
TestMd5::testString()
{
	Md5 md5(std::string("hello world!"));
	ASSERT_EQUAL("digest", "fc3ff98e8c6a0d3087d515c0473f8677",
		     md5.hexDigest());
}

void
TestMd5::testStringView()
{
	Md5 md5(StringView("hello world!", 12));
	ASSERT_EQUAL("digest", "fc3ff98e8c6a0d3087d515c0473f8677",
		     md5.hexDigest());
}

void
TestMd5::testUpdateFinalize()
{
	Md5 md5;
	md5.update("hello ");
	md5.update("world!", true);
	ASSERT_EQUAL("digest", "fc3ff98e8c6a0d3087d515c0473f8677",
		     md5.hexDigest());
}
