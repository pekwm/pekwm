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
	static void testMoveToEdge();
	static void testFillEdgeGeometry();
	static void testDetachClientSplitHorz();
	static void testDetachClientSplitVert();
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
	TEST_FN(spec, "moveToEdge", testMoveToEdge());
	TEST_FN(spec, "fillEdgeGeometry", testFillEdgeGeometry());
	TEST_FN(spec, "detachClientSplitHorz", testDetachClientSplitHorz());
	TEST_FN(spec, "detachClientSplitVert", testDetachClientSplitVert());
	return status;
}

void
TestActionHandler::testMoveToEdge()
{
	PWinObj wo;

	// head 0
	wo.moveResize(0, 0, 100, 100);
	ActionHandler::actionMoveToEdge(&wo, RIGHT_CENTER_EDGE);
	ASSERT_EQUAL("RIGHT_CENTER_EDGE", Geometry(700, 250, 100, 100),
		     wo.getGeometry());

	// head 1
	wo.moveResize(800, 0, 100, 100);
	ActionHandler::actionMoveToEdge(&wo, RIGHT_CENTER_EDGE);
	ASSERT_EQUAL("RIGHT_CENTER_EDGE", Geometry(1500, 250, 100, 100),
		     wo.getGeometry());
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

void
TestActionHandler::testDetachClientSplitHorz()
{
	PWinObj oldf;
	PWinObj newf;
	oldf.moveResize(100, 200, 500, 400);
	newf.moveResize(100, 200, 500, 400);
	ActionHandler::actionDetachClientSplitHorz(&oldf, &newf, 0.5);
	ASSERT_EQUAL("old x", 100, oldf.getX());
	ASSERT_EQUAL("old y", 400, oldf.getY());
	ASSERT_EQUAL("old width", 500, oldf.getWidth());
	ASSERT_EQUAL("old height", 200, oldf.getHeight());
	ASSERT_EQUAL("new x", 100, newf.getX());
	ASSERT_EQUAL("new y", 200, newf.getY());
	ASSERT_EQUAL("new width", 500, newf.getWidth());
	ASSERT_EQUAL("new height", 200, newf.getHeight());
}

void
TestActionHandler::testDetachClientSplitVert()
{
	PWinObj oldf;
	PWinObj newf;
	oldf.moveResize(100, 200, 500, 400);
	newf.moveResize(100, 200, 500, 400);
	ActionHandler::actionDetachClientSplitVert(&oldf, &newf, 0.25);
	ASSERT_EQUAL("old x", 225, oldf.getX());
	ASSERT_EQUAL("old y", 200, oldf.getY());
	ASSERT_EQUAL("old width", 375, oldf.getWidth());
	ASSERT_EQUAL("old height", 400, oldf.getHeight());
	ASSERT_EQUAL("new x", 100, newf.getX());
	ASSERT_EQUAL("new y", 200, newf.getY());
	ASSERT_EQUAL("new width", 125, newf.getWidth());
	ASSERT_EQUAL("new height", 400, newf.getHeight());
}
