//
// test_Workspaces.hh for pekwm
// Copyright (C) 2024-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "ManagerWindows.hh"
#include "Workspaces.hh"

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
	pekwm::setRootWO(new RootWO(None, &hint_wo, &cfg));

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
