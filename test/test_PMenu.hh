//
// test_PMenu.hh for pekwm
// Copyright (C) 2024-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "wm/PMenu.hh"

class TestPMenu : public TestSuite {
public:
	TestPMenu();
	~TestPMenu();

	bool run_test(TestSpec spec, bool status);

private:
	void testParsePMenuName();
	void testStripDuplicateShortcut();
	void testSelectItemNum();
	void testSelectItemNumSkipFirst();
	void testSelectItemNumSkipAll();
};

TestPMenu::TestPMenu()
	: TestSuite("PMenu")
{
}

TestPMenu::~TestPMenu()
{
}

bool
TestPMenu::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parsePMenuName", testParsePMenuName());
	TEST_FN(spec, "stripDuplicateShortcut", testStripDuplicateShortcut());
	TEST_FN(spec, "selectItemNum", testSelectItemNum());
	TEST_FN(spec, "selectItemNumSkipFirt", testSelectItemNumSkipFirst());
	TEST_FN(spec, "selectItemNumSkipAll", testSelectItemNumSkipAll());
	return status;
}

void
TestPMenu::testParsePMenuName()
{
	uint kc, kp;
	std::string name;

	name = parsePMenuName("No Underscore", kc, kp);
	ASSERT_EQUAL("no match", "No Underscore", name);
	ASSERT_EQUAL("no match", 0, kp);
	name = parsePMenuName("Räksm_örgås", kc, kp);
	ASSERT_EQUAL("UTF8 match", "Räksmörgås", name);
	ASSERT_EQUAL("UTF8 match", 6, kp);
	name = parsePMenuName("_multi, keep extra _ _ _", kc, kp);
	ASSERT_EQUAL("multi _", "multi, keep extra _ _ _", name);
	ASSERT_EQUAL("multi _", 0, kp);
}

void
TestPMenu::testStripDuplicateShortcut()
{
	PMenu menu("title", "test", "decor", false);

	PMenu::Item *item1 = new PMenu::Item("_S One");
	item1->setKeycode(1); // X11 not available, no keycode looked up
	menu.insert(item1);
	PMenu::Item *item2 = new PMenu::Item("_S Two");
	item2->setKeycode(1); // X11 not available, no keycode looked up
	menu.insert(item2);
	ASSERT_EQUAL("item1 keycode kept", 1, item1->getKeycode());
	ASSERT_EQUAL("item2 keycode stripped", 0, item2->getKeycode());
}

void
TestPMenu::testSelectItemNum()
{
	PMenu menu("title", "test", "decor", false);
	ASSERT_FALSE("non existing item should fail", menu.selectItemNum(0));
	ASSERT_FALSE("non existing item should fail", menu.selectItemNum(10));

	menu.insert(new PMenu::Item("first"));
	PMenu::Item *sep = new PMenu::Item("");
	sep->setType(PMenu::Item::MENU_ITEM_SEPARATOR);
	menu.insert(sep);
	menu.insert(new PMenu::Item("third"));

	ASSERT_TRUE("select first", menu.selectItemNum(0));
	ASSERT_EQUAL("select first", "first",
		     menu.getItemCurr()->getName());
	ASSERT_TRUE("select second", menu.selectItemNum(1));
	ASSERT_EQUAL("select second", "third",
		     menu.getItemCurr()->getName());
	ASSERT_FALSE("separator should be skipped", menu.selectItemNum(2));
}

void TestPMenu::testSelectItemNumSkipFirst()
{
	PMenu menu("title", "test", "decor", false);

	PMenu::Item *sep = new PMenu::Item("");
	sep->setType(PMenu::Item::MENU_ITEM_SEPARATOR);
	menu.insert(sep);
	menu.insert(new PMenu::Item("first"));

	ASSERT_TRUE("select first", menu.selectItemNum(0));
	ASSERT_EQUAL("select first", "first",
		     menu.getItemCurr()->getName());
}

void TestPMenu::testSelectItemNumSkipAll()
{
	PMenu menu("title", "test", "decor", false);

	PMenu::Item *sep;
	sep = new PMenu::Item("");
	sep->setType(PMenu::Item::MENU_ITEM_SEPARATOR);
	menu.insert(sep);
	sep = new PMenu::Item("");
	sep->setType(PMenu::Item::MENU_ITEM_SEPARATOR);
	menu.insert(sep);

	ASSERT_FALSE("select none", menu.selectItemNum(0));
}
