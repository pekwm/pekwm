//
// test_ActionHandler.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "wm/ActionHandler.hh"

class TestActionHandler : public TestSuite {
public:
	TestActionHandler();
	virtual ~TestActionHandler();

	virtual bool run_test(TestSpec, bool status);

private:
	static void testFillEdgeGeometry();
};

TestActionHandler::TestActionHandler()
	: TestSuite("ActionHandler")
{
}

TestActionHandler::~TestActionHandler()
{
}

bool
TestActionHandler::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "fillEdgeGeometry", testFillEdgeGeometry());
	return status;
}

void
TestActionHandler::testFillEdgeGeometry()
{
	Geometry gm;
	Geometry head(0, 0, 200, 100);

	ActionHandler::fillEdgeGeometry(head, LEFT_EDGE, 50, gm);
	ASSERT_EQUAL("LEFT_EDGE", Geometry(0, 0, 100, 100), gm);

	ActionHandler::fillEdgeGeometry(head, RIGHT_EDGE, 50, gm);
	ASSERT_EQUAL("LEFT_EDGE", Geometry(100, 0, 100, 100), gm);

	ActionHandler::fillEdgeGeometry(head, TOP_EDGE, 50, gm);
	ASSERT_EQUAL("LEFT_EDGE", Geometry(0, 0, 200, 50), gm);

	ActionHandler::fillEdgeGeometry(head, BOTTOM_EDGE, 50, gm);
	ASSERT_EQUAL("LEFT_EDGE", Geometry(0, 50, 200, 50), gm);
}
