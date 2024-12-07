//
// test_Workspaces.hh for pekwm
// Copyright (C) 2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Workspaces.hh"

#include <utility>

extern "C" {
#include <X11/Xlib.h>
}

class TestWorkspaces : public TestSuite,
		       public Workspaces {
public:
	TestWorkspaces(void);
	virtual ~TestWorkspaces(void);

	bool run_test(TestSpec spec, bool status);

	void testSwapInStack(void);
	void testStackAbove(void);
};

TestWorkspaces::TestWorkspaces(void)
	: TestSuite("Workspaces"),
	  Workspaces()
{
}

TestWorkspaces::~TestWorkspaces(void)
{
}

bool
TestWorkspaces::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "swapInStack", testSwapInStack());
	TEST_FN(spec, "stackAbove", testStackAbove());
	return status;
}

void
TestWorkspaces::testSwapInStack(void)
{
	_wobjs.clear();

	PWinObj wobjs[5];
	for (int i = 0; i < 5; i++) {
		insert(&wobjs[i]);
	}

	// initial order, as inserted
	for (int i = 0; i < 5; i++) {
		ASSERT_EQUAL("stack order", _wobjs[i], &wobjs[i]);
	}

	// move 2 to the top
	ASSERT_TRUE("swap", swapInStack(&wobjs[2], &wobjs[4]));
	ASSERT_EQUAL("stack order", _wobjs[0], &wobjs[0]);
	ASSERT_EQUAL("stack order", _wobjs[1], &wobjs[1]);
	ASSERT_EQUAL("stack order", _wobjs[2], &wobjs[4]);
	ASSERT_EQUAL("stack order", _wobjs[3], &wobjs[3]);
	ASSERT_EQUAL("stack order", _wobjs[4], &wobjs[2]);

	// move 2 to the bottom
	ASSERT_TRUE("swap", swapInStack(&wobjs[2], &wobjs[0]));
	ASSERT_EQUAL("stack order", _wobjs[0], &wobjs[2]);
	ASSERT_EQUAL("stack order", _wobjs[1], &wobjs[1]);
	ASSERT_EQUAL("stack order", _wobjs[2], &wobjs[4]);
	ASSERT_EQUAL("stack order", _wobjs[3], &wobjs[3]);
	ASSERT_EQUAL("stack order", _wobjs[4], &wobjs[0]);
}

void
TestWorkspaces::testStackAbove(void)
{
	_wobjs.clear();
	PWinObj wobjs[5];
	for (int i = 0; i < 5; i++) {
		insert(&wobjs[i]);
	}

	// move 1 above 3 making it appear at position 3
	ASSERT_TRUE("stack", stackAbove(&wobjs[1], &wobjs[3]));
	ASSERT_EQUAL("stack order", _wobjs[0], &wobjs[0]);
	ASSERT_EQUAL("stack order", _wobjs[1], &wobjs[2]);
	ASSERT_EQUAL("stack order", _wobjs[2], &wobjs[3]);
	ASSERT_EQUAL("stack order", _wobjs[3], &wobjs[1]);
	ASSERT_EQUAL("stack order", _wobjs[4], &wobjs[4]);
}
