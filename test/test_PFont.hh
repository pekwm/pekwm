//
// test_PFont.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "test.hh"
#include "PFont.hh"

class MockPFont : public PFont {
public:
    MockPFont(const std::map<std::string, uint> &width_map);
    virtual ~MockPFont(void);

    virtual bool load(const std::string&) override { return true; }
    virtual uint getWidth(const std::string& text, uint max_chars = 0) override;
    virtual void setColor(PFont::Color*) override { }

private:
    virtual void drawText(Drawable, int, int, const std::string&,
                          uint, bool) override { }

private:
    std::map<std::string, uint> _width_map;
};

MockPFont::MockPFont(const std::map<std::string, uint> &width_map)
    : _width_map(width_map)
{
}

MockPFont::~MockPFont(void)
{
}

uint
MockPFont::getWidth(const std::string& text, uint max_chars)
{
    auto key = text + std::to_string(max_chars);
    auto it = _width_map.find(key);
    if (it == _width_map.end()) {
        std::cerr << "unexpected lookup " << key << std::endl;
        std::cerr << "|" << text.substr(0, max_chars) << "|" << std::endl;
        exit(1);
    }
    return _width_map[key];
}

class TestPFont : public TestSuite {
public:
    TestPFont(void);
    ~TestPFont(void);

private:
    static void testTrimEndNoTrim(void);
    static void testTrimEndTrim(void);
    static void testTrimEndNoSpace(void);
    static void testTrimEndUTF8(void);
    static void testTrimMiddle(void);
};

TestPFont::TestPFont(void)
    : TestSuite("PFont")
{
    register_test("trimEndNoTrim", TestPFont::testTrimEndNoTrim);
    register_test("trimEndTrim", TestPFont::testTrimEndTrim);
    register_test("trimEndNoSpace", TestPFont::testTrimEndNoSpace);
    register_test("trimEndUTF8", TestPFont::testTrimEndUTF8);
    register_test("trimMiddle", TestPFont::testTrimMiddle);
}

TestPFont::~TestPFont(void)
{
}

void
TestPFont::testTrimEndNoTrim(void)
{
    MockPFont font({{"test0", 100}});
    std::string str("test");
    font.trim(str, PFont::FONT_TRIM_END, 100);
    ASSERT_EQUAL("no trim", "test", str);
}

void
TestPFont::testTrimEndTrim(void)
{
    MockPFont font({{"test0", 100}, {"test3", 75}, {"test2", 50}});
    std::string str("test");
    font.trim(str, PFont::FONT_TRIM_END, 50);
    ASSERT_EQUAL("trim end", "te", str);
}

void
TestPFont::testTrimEndNoSpace(void)
{
    MockPFont font({{"test0", 100}, {"test3", 75},
                    {"test2", 50}, {"test1", 25}});
    std::string str("test");
    font.trim(str, PFont::FONT_TRIM_END, 10);
    ASSERT_EQUAL("trim end, does not fit", "", str);
}

void
TestPFont::testTrimEndUTF8(void)
{
    MockPFont font({{"Räksmörgås — M0", 100},
                    {"Räksmörgås — M17", 90},
                    {"Räksmörgås — M15", 80},
                    {"Räksmörgås — M14", 70},
                    {"Räksmörgås — M13", 60},
                    {"Räksmörgås — M12", 50},
                    {"Räksmörgås — M10", 40}});
    std::string str("Räksmörgås — M");
    font.trim(str, PFont::FONT_TRIM_END, 45);
    ASSERT_EQUAL("trim end, UTF8", "Räksmörg", str);
}

void
TestPFont::testTrimMiddle(void)
{
    MockPFont font({{"testtest0", 80},
                    {"..0", 20},

                    {"testtest7", 70},
                    {"testtest6", 60},
                    {"testtest5", 50},
                    {"testtest4", 40},
                    {"testtest3", 30},
                    {"testtest2", 20},

                    {"ttest0", 50},
                    {"test0", 40},
                    {"est0", 30},
                    {"st0", 20},

                    {"test2", 20},

                    {"te..st0", 60}});
    std::string str("testtest");
    font.setTrimString("..");
    font.trim(str, PFont::FONT_TRIM_MIDDLE, 60);
    ASSERT_EQUAL("trim middle", "te..st", str);
}
