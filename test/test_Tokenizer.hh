//
// test_Tokenizer.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "Tokenizer.hh"

class TestTokenizer : public TestSuite {
public:
	TestTokenizer(void)
		: TestSuite("Tokenizer")
	{
	}
	virtual ~TestTokenizer(void)
	{
	}

	virtual bool run_test(TestSpec spec, bool status);

	static void testEmpty(void);
	static void testNewline(void);
	static void testEscapedNewline(void);
	static void testSpace(void);
	static void testTab(void);
};

static void
assert_token(const std::string& msg, Tokenizer& tok, bool success,
	     const std::string& str, bool is_break)
{
	bool res = tok.next();
	ASSERT_EQUAL(msg + " next", success, res);
	if (success) {
		ASSERT_EQUAL(msg + " *", str, *tok);
		ASSERT_EQUAL(msg + " break?", is_break, tok.isBreak());
	}
}

bool
TestTokenizer::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "empty", testEmpty());
	TEST_FN(spec, "newline", testNewline());
	TEST_FN(spec, "escaped newline", testEscapedNewline());
	TEST_FN(spec, "space", testSpace());
	TEST_FN(spec, "tab", testTab());
	return status;
}

void
TestTokenizer::testEmpty(void)
{
	Tokenizer tok("");
	ASSERT_EQUAL("empty", false, tok.next());
}

void
TestTokenizer::testNewline(void)
{
	Tokenizer tok("one\ntwo\n");
	assert_token("first", tok, true, "one", false);
	assert_token("2", tok, true, "\n", true);
	assert_token("3", tok, true, "two", false);
	assert_token("4", tok, true, "\n", true);
	assert_token("end", tok, false, "", false);
}

void
TestTokenizer::testEscapedNewline(void)
{
	Tokenizer tok("one\\ntwo\\n");
	assert_token("first", tok, true, "one", false);
	assert_token("2", tok, true, "\n", true);
	assert_token("3", tok, true, "two", false);
	assert_token("4", tok, true, "\n", true);
	assert_token("end", tok, false, "", false);
}

void
TestTokenizer::testSpace(void)
{
	Tokenizer tok(" a b c ");
	assert_token("first", tok, true, " ", false);
	assert_token("2", tok, true, "a", false);
	assert_token("3", tok, true, " ", false);
	assert_token("4", tok, true, "b", false);
	assert_token("5", tok, true, " ", false);
	assert_token("6", tok, true, "c", false);
	assert_token("7", tok, true, " ", false);
	assert_token("end", tok, false, "", false);
}

void
TestTokenizer::testTab(void)
{
	Tokenizer tok("\t\t");
	assert_token("first", tok, true, "\t", false);
	assert_token("2", tok, true, "\t", false);
	assert_token("end", tok, false, "", false);
}
