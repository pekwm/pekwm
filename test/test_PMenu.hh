//
// test_PMenu.cc for pekwm
// Copyright (C) 2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "PMenu.hh"

class TestPMenu : public TestSuite {
public:
	TestPMenu();
	~TestPMenu();

	bool run_test(TestSpec spec, bool status);

private:
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
	TEST_FN(spec, "selectItemNum", testSelectItemNum());
	TEST_FN(spec, "selectItemNumSkipFirt", testSelectItemNumSkipFirst());
	TEST_FN(spec, "selectItemNumSkipAll", testSelectItemNumSkipAll());
	return status;
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

	ASSERT_FALSE("select none", menu.selectItemNum(0));
}
