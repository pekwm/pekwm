//
// test_XSettings.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "../src/sys/XSettings.hh"

#include <cstring>
#include <sstream>

class XSettingsTest : public XSettings {
public:
	virtual ~XSettingsTest() { }

	std::string writeProp()
	{
		std::stringstream os;
		XSettings::writeProp(os, 12345678);
		return os.str();
	}
};

class TestXSettings : public TestSuite {
public:
	TestXSettings();
	~TestXSettings();

	virtual bool run_test(TestSpec spec, bool status);

private:
	static void testWrite();
	static void testToString();
	static void testMkXSetting();
	static void testSave();
	static void testLoad();
	static void testValidateName();
	static void testSetUpdateLastChanged();
};

TestXSettings::TestXSettings()
	: TestSuite("XSettings")
{
}

TestXSettings::~TestXSettings()
{
}

bool
TestXSettings::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "write", testWrite());
	TEST_FN(spec, "toString", testToString());
	TEST_FN(spec, "mkXSetting", testMkXSetting());
	TEST_FN(spec, "save", testSave());
	TEST_FN(spec, "load", testLoad());
	TEST_FN(spec, "validateName", testValidateName());
	TEST_FN(spec, "setUpdateLastChanged",
		testSetUpdateLastChanged());
	return status;
}

void
TestXSettings::testWrite()
{
	XSettingsTest xs;
	std::string key, str;
	int32_t serial, num_settings;
	int n = 1;
	char bo = reinterpret_cast<char*>(&n)[0] == 1 ? LSBFirst : MSBFirst;

	// empty, verify header
	std::string empty = xs.writeProp();
	ASSERT_EQUAL("write empty", 12, empty.size());
	ASSERT_EQUAL("write empty", empty[0], bo);
	ASSERT_EQUAL("write empty", 0, empty[1]);
	ASSERT_EQUAL("write empty", 0, empty[2]);
	ASSERT_EQUAL("write empty", 0, empty[3]);
	memcpy(reinterpret_cast<void*>(&serial), empty.data() + 4, 4);
	ASSERT_EQUAL("write empty", 12345678, serial);
	memcpy(reinterpret_cast<void*>(&num_settings), empty.data() + 8, 4);
	ASSERT_EQUAL("write empty", 0, num_settings);
}

void
TestXSettings::testToString()
{
	ASSERT_EQUAL("string", "sTest", XSettingString("Test").toString());
	ASSERT_EQUAL("string (quote)", "s\\\\ and \\\"",
		     XSettingString("\\ and \"").toString());

	ASSERT_EQUAL("integer", "i42", XSettingInteger(42).toString());
	ASSERT_EQUAL("integer (negative)", "i-42",
		     XSettingInteger(-42).toString());

	ASSERT_EQUAL("color", "c(10, 20, 30, 65535)",
		     XSettingColor(10, 20, 30, 65535).toString());
}

void
TestXSettings::testMkXSetting()
{
	XSetting *s;
	s = mkXSetting("sTest");
	ASSERT_TRUE("string", s != nullptr);
	ASSERT_EQUAL("string", s->getType(), XSETTING_TYPE_STRING);
	ASSERT_EQUAL("string", "Test",
		     static_cast<XSettingString*>(s)->getValue());
	delete s;

	s = mkXSetting("i42");
	ASSERT_TRUE("integer", s != nullptr);
	ASSERT_EQUAL("integer", s->getType(), XSETTING_TYPE_INTEGER);
	ASSERT_EQUAL("integer", 42,
		     static_cast<XSettingInteger*>(s)->getValue());
	delete s;

	s = mkXSetting("i-42");
	ASSERT_TRUE("integer (negative)", s != nullptr);
	ASSERT_EQUAL("integer (negative)", s->getType(),
		     XSETTING_TYPE_INTEGER);
	ASSERT_EQUAL("integer (negative)", -42,
		     static_cast<XSettingInteger*>(s)->getValue());
	delete s;

	s = mkXSetting("c(10, 20, 30, 65535)");
	ASSERT_TRUE("color", s != nullptr);
	ASSERT_EQUAL("color", s->getType(), XSETTING_TYPE_COLOR);
	XSettingColor *c = static_cast<XSettingColor*>(s);
	ASSERT_EQUAL("color", 10, c->getR());
	ASSERT_EQUAL("color", 20, c->getG());
	ASSERT_EQUAL("color", 30, c->getB());
	ASSERT_EQUAL("color", 65535, c->getA());
	delete s;
}

void
TestXSettings::testSave()
{
	XSettings xs;
	xs.setString("Key1", "StringValue");
	xs.setInt32("Key2", 42);
	xs.setColor("Key3", 10, 20, 30, 40);

	std::stringstream buf;
	xs.save(buf);
	ASSERT_EQUAL("save",
		     "# written by pekwm_sys, overwritten by Sys XSave\n"
		     "Settings {\n"
		     "\t\"Key1\" = \"sStringValue\"\n"
		     "\t\"Key2\" = \"i42\"\n"
		     "\t\"Key3\" = \"c(10, 20, 30, 40)\"\n"
		     "}\n", buf.str());
}

void
TestXSettings::testLoad()
{
	XSettings xs;
	ASSERT_TRUE("load",
		    xs.load("Settings {\n"
			    "\t\"Key1\" = \"sStringValue\"\n"
			    "\t\"Key2\" = \"i42\"\n"
			    "\t\"Key3\" = \"c(10, 20, 30, 40)\"\n"
			    "}\n", CfgParserSource::SOURCE_STRING));
	const XSettingString *s =
		dynamic_cast<const XSettingString*>(xs.get("Key1"));
	ASSERT_EQUAL("load", "StringValue", s->getValue());
	const XSettingInteger *i =
		dynamic_cast<const XSettingInteger*>(xs.get("Key2"));
	ASSERT_EQUAL("load", 42, i->getValue());
	const XSettingColor *c =
		dynamic_cast<const XSettingColor*>(xs.get("Key3"));
	ASSERT_EQUAL("load", 10, c->getR());
	ASSERT_EQUAL("load", 20, c->getG());
	ASSERT_EQUAL("load", 30, c->getB());
	ASSERT_EQUAL("load", 40, c->getA());
}

void
TestXSettings::testValidateName()
{
	std::string error;
	ASSERT_FALSE("empty", XSetting::validateName("", error));
	ASSERT_EQUAL("empty", "name must not be empty", error);

	ASSERT_FALSE("//", XSetting::validateName("test//me", error));
	ASSERT_EQUAL("//", "name must not contain //", error);

	ASSERT_FALSE("leading /", XSetting::validateName("/test", error));
	ASSERT_EQUAL("leading /", "name must start with a-z, A-Z", error);

	ASSERT_FALSE("leading 0", XSetting::validateName("0test", error));
	ASSERT_EQUAL("leading 0", "name must start with a-z, A-Z", error);

	ASSERT_FALSE("trailing /", XSetting::validateName("test/", error));
	ASSERT_EQUAL("trailing /", "name must not end with /", error);

	ASSERT_FALSE("invalid char", XSetting::validateName("test%", error));
	ASSERT_EQUAL("invalid char",
		     "name can only contain a-z, A-Z, 0-9 and /", error);

	ASSERT_TRUE("valid", XSetting::validateName("My/Name9", error));
	ASSERT_EQUAL("valid", "", error);
}

void
TestXSettings::testSetUpdateLastChanged()
{
	XSettings xs;

	xs.setString("Key", "Test1");
	ASSERT_EQUAL("last changed", 0, xs.get("Key")->getLastChanged());

	xs.setString("Key", "Test2");
	ASSERT_EQUAL("last changed", 1, xs.get("Key")->getLastChanged());
}
