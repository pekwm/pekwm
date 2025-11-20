//
// test_Workspaces.hh for pekwm
// Copyright (C) 2024-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "wm/ManagerWindows.hh"
#include "wm/Workspaces.hh"

extern "C" {
#include <X11/Xlib.h>
}

class TestWorkspaces : public TestSuite,
		       public Workspaces {
public:
	TestWorkspaces();
	virtual ~TestWorkspaces();

	bool run_test(TestSpec spec, bool status);

	void testSwapInStack();
	void testStackAbove();
	void testFindWOAndFocusFind();
	void testLayoutOnHeadTypes();
	void testGotoWorkspaceBackAndForth();
	void testSetSize();
};

TestWorkspaces::TestWorkspaces()
	: TestSuite("Workspaces"),
	  Workspaces()
{
}

TestWorkspaces::~TestWorkspaces()
{
}

bool
TestWorkspaces::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "swapInStack", testSwapInStack());
	TEST_FN(spec, "stackAbove", testStackAbove());
	TEST_FN(spec, "findWOAndFocusFind", testFindWOAndFocusFind());
	TEST_FN(spec, "layoutOnHeadTypes", testLayoutOnHeadTypes());
	TEST_FN(spec, "gotoWorkspaceBackAndForth",
		testGotoWorkspaceBackAndForth());
	TEST_FN(spec, "setSize", testSetSize());
	return status;
}

void
TestWorkspaces::testSwapInStack()
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
TestWorkspaces::testStackAbove()
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

class TestPWinObj : public PWinObj {
public:
	TestPWinObj(PWinObj::Type type, bool focusable, bool mapped)
		: PWinObj()
	{
		_type = type;
		_focusable = focusable;
		_mapped = mapped;
	}
	virtual ~TestPWinObj()
	{
	}
};

void
TestWorkspaces::testFindWOAndFocusFind()
{
	_wobjs.clear();

	// empty
	PWinObj *null_wo = nullptr;
	ASSERT_EQUAL("empty", null_wo, findWOAndFocusFind(true));
	ASSERT_EQUAL("empty", null_wo, findWOAndFocusFind(false));

	// no focusable objects
	TestPWinObj wo_nf(PWinObj::WO_FRAME, false, true);
	_wobjs.push_back(&wo_nf);
	_mru.push_back((Frame*) &wo_nf);
	ASSERT_EQUAL("no focusable", null_wo, findWOAndFocusFind(true));
	ASSERT_EQUAL("no focusable", null_wo, findWOAndFocusFind(false));

	// no mru, fallback to stacking
	TestPWinObj wo1(PWinObj::WO_FRAME, true, true);
	_wobjs.push_back(&wo1);
	ASSERT_EQUAL("no mru", &wo1, findWOAndFocusFind(true));
	ASSERT_EQUAL("no mru", &wo1, findWOAndFocusFind(false));

	// mru and stacking differ
	TestPWinObj wo2(PWinObj::WO_FRAME, true, true);
	_wobjs.push_back(&wo2);
	_mru.push_back((Frame*) &wo1);
	_mru.push_back((Frame*) &wo2);
	ASSERT_EQUAL("mru and stacking", &wo2, findWOAndFocusFind(true));
	ASSERT_EQUAL("mru and stacking", &wo1, findWOAndFocusFind(false));

	_wobjs.clear();
	_mru.clear();
}

void
TestWorkspaces::testLayoutOnHeadTypes()
{
	Workspaces::init();
	HintWO hint_wo(None);
	Config cfg;
	pekwm::setRootWO(new RootWO(None, &hint_wo, &cfg, true));

	// not using MouseTopCenter will result in being placed at 0x0
	std::vector<std::string> models;
	models.push_back("SMART");

	int win_layouter_types = WIN_LAYOUTER_MOUSECTOPLEFT;
	PWinObj wo;
	Geometry gm;
	ASSERT_EQUAL("layout", true,
		     layoutOnHeadTypes(&wo, win_layouter_types, None, gm,
				       100, 200));
	ASSERT_EQUAL("layout", 100, wo.getX());
	ASSERT_EQUAL("layout", 200, wo.getY());

	pekwm::setRootWO(nullptr);
	Workspaces::cleanup();
}

void
TestWorkspaces::testGotoWorkspaceBackAndForth()
{
	HintWO hint_wo(None);
	Config cfg;
	pekwm::setRootWO(new RootWO(None, &hint_wo, &cfg, true));
	setSize(4);

	ASSERT_TRUE("WorkspacesBackAndForth",
		    pekwm::config()->isWorkspacesBackAndForth());
	pekwm::config()->setShowWorkspaceIndicator(0);

	ASSERT_EQUAL("active at start", 0, getActive());
	bool switched = gotoWorkspace(1, false, false);
	ASSERT_TRUE("move from 0 -> 1", switched);
	ASSERT_EQUAL("move from 0 -> 1", 1, getActive());

	switched = gotoWorkspace(1, false, false);
	ASSERT_TRUE("move from 1 -> 1, go back to 0", switched);
	ASSERT_EQUAL("move from 1 -> 1, go back to 0", 0, getActive());

	switched = gotoWorkspace(1, false, false);
	ASSERT_TRUE("move from 0 -> 1", switched);
	ASSERT_EQUAL("move from 0 -> 1", 1, getActive());
	switched = gotoWorkspace(3, false, false);
	ASSERT_TRUE("move from 1 -> 3", switched);
	ASSERT_EQUAL("move from 1 -> 3", 3, getActive());

	switched = gotoWorkspace(3, false, false);
	ASSERT_TRUE("move from 3 -> 3, go back to 1", switched);
	ASSERT_EQUAL("move from 3 -> 3, go back to 1", 1, getActive());
}

void
TestWorkspaces::testSetSize()
{
	// size down to 1, everything on the same workspace
	ASSERT_TRUE("size 0, result in 1", setSize(0));
	ASSERT_EQUAL("size 1", 1, size());
	ASSERT_EQUAL("active 0", 0, getActive());
	ASSERT_EQUAL("previous 0", 0, getPrevious());

	// size up above number of configured names, use default names
	ASSERT_TRUE("size 4, grow to 4", setSize(4));
	ASSERT_EQUAL("size 4", 4, size());
	ASSERT_EQUAL("name 0", "1", getWorkspace(0).getName());
	ASSERT_EQUAL("name 1", "2", getWorkspace(1).getName());
	ASSERT_EQUAL("name 2", "3", getWorkspace(2).getName());
	ASSERT_EQUAL("name 3", "4", getWorkspace(3).getName());

	// size same, don't change
	ASSERT_FALSE("size unchanged", setSize(4));
}
