//
// test_Json.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Json.hh"

extern "C" {
#include <math.h>
}

class TestJsonParser : public JsonParser {
public:
	TestJsonParser()
		: JsonParser("")
	{
	}

	TestJsonParser(const std::string &str, size_t prefill_bytes)
		: JsonParser(str)
	{
		fill(prefill_bytes);
	}
	virtual ~TestJsonParser() { }

	JsonValueObject* parseObject(const std::string& str,
				     size_t prefill_bytes)
	{
		TestJsonParser p(str, prefill_bytes);
		JsonValueObject *jobj = p.doParseObject();
		_error.str(p.getError());
		return jobj;
	}

	JsonValueObject* doParseObject() { return JsonParser::parseObject(); }

	JsonValueArray* parseArray(const std::string& str,
				   size_t prefill_bytes)
	{
		TestJsonParser p(str, prefill_bytes);
		JsonValueArray *jarr = p.doParseArray();
		_error.str(p.getError());
		return jarr;
	}

	JsonValueArray* doParseArray() { return JsonParser::parseArray(); }

	JsonValueString* parseString(const std::string& str,
				     size_t prefill_bytes)
	{
		TestJsonParser p(str, prefill_bytes);
		JsonValueString *jstr = p.doParseString();
		_error.str(p.getError());
		return jstr;
	}

	JsonValueString* doParseString() { return JsonParser::parseString(); }

	JsonValueNumber* parseNumber(const std::string& str,
				     size_t prefill_bytes)
	{
		TestJsonParser p(str, prefill_bytes);
		JsonValueNumber *jnum = p.doParseNumber();
		_error.str(p.getError());
		return jnum;
	}

	JsonValueNumber* doParseNumber() { return JsonParser::parseNumber(); }

	JsonValueBoolean* parseBoolean(const std::string& str,
				       size_t prefill_bytes)
	{
		TestJsonParser p(str, prefill_bytes);
		JsonValueBoolean *jbool = p.doParseBoolean();
		_error.str(p.getError());
		return jbool;
	}

	JsonValueBoolean* doParseBoolean()
	{
		return JsonParser::parseBoolean();
	}

	JsonValueNull* parseNull(const std::string& str,
				 size_t prefill_bytes)
	{
		TestJsonParser p(str, prefill_bytes);
		JsonValueNull *jnull = p.doParseNull();
		_error.str(p.getError());
		return jnull;
	}

	JsonValueNull* doParseNull() { return JsonParser::parseNull(); }

private:
	void prefill(size_t bytes)
	{
		if (bytes) {
			fill(bytes);
		}
	}
};

class TestJson : public TestSuite {
public:
	TestJson();
	virtual ~TestJson();

	virtual bool run_test(TestSpec, bool status);

private:
	static void testParseObject();
	static void testParseArray();
	static void testParseString();
	static void testParseNumber();
	static void testParseBoolean();
	static void testParseNull();
};

TestJson::TestJson()
	: TestSuite("Json")
{
}

TestJson::~TestJson()
{
}

bool
TestJson::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "parseObject", testParseObject());
	TEST_FN(spec, "parseArray", testParseArray());
	TEST_FN(spec, "parseString", testParseString());
	TEST_FN(spec, "parseNumber", testParseNumber());
	TEST_FN(spec, "parseBoolean", testParseBoolean());
	TEST_FN(spec, "parseNull", testParseNull());
	return status;
}

void
TestJson::testParseObject()
{
	TestJsonParser parser;
	JsonValueObject *value;

	// empty
	value = parser.parseObject("{}", 1);
	ASSERT_EQUAL("empty", "", parser.getError());
	ASSERT_TRUE("empty", value != nullptr);
	ASSERT_EQUAL("empty", 0, value->size());
	delete value;

	// single value
	value = parser.parseObject("{ \"key\" : \"value\" }", 1);
	ASSERT_EQUAL("single value", "", parser.getError());
	ASSERT_TRUE("single value", value != nullptr);
	ASSERT_EQUAL("single value", 1, value->size());
	ASSERT_EQUAL("single value", "value", *(*jsonGetString(value, "key")));
	delete value;

	// multiple values
	value = parser.parseObject("{\"key1\":1,\"key2\":2}", 1);
	ASSERT_EQUAL("multiple values", "", parser.getError());
	ASSERT_TRUE("multiple values", value != nullptr);
	ASSERT_EQUAL("multiple values", 2, value->size());
	ASSERT_EQUAL("multiple values", 1, *(*jsonGetNumber(value, "key1")));
	ASSERT_EQUAL("multiple values", 2, *(*jsonGetNumber(value, "key2")));
	delete value;

	// UTF-8
	value = parser.parseObject("{\"macka\": \"räksmörgås\"}", 1);
	ASSERT_EQUAL("UTF-8", "", parser.getError());
	ASSERT_TRUE("UTF-8", value != nullptr);
	ASSERT_EQUAL("UTF-8", "räksmörgås",
		     *(*jsonGetString(value, "macka")));
	delete value;

	// geoip.pw
	std::string json =
		"{\"results\":[{\"query\":\"8.8.8.8\",\"ip\":\"8.8.8.8\","
		"\"ip_type\":4,\"summary\":\"United States, NA\","
		"\"city\":\"\",\"subdivision\":\"\","
		"\"country\":\"United States\",\"country_abbr\":\"US\","
		"\"continent\":\"North America\",\"continent_abbr\":\"NA\","
		"\"latitude\":37.751,\"longitude\":-97.822,"
		"\"timezone\":\"America/Chicago\",\"postal_code\":\"\","
		"\"accuracy_radius_km\":1000,\"network\":\"8.8.8.0/24\","
		"\"asn\":\"AS15169\",\"asn_org\":\"GOOGLE\"}],\"errors\":[]}";
	value = parser.parseObject(json, 1);
	ASSERT_EQUAL("geoip.pw", "", parser.getError());
	ASSERT_TRUE("geoip.pw", value != nullptr);
	delete value;

	// missing begin
	value = parser.parseObject("\"key\": 42}", 1);
	ASSERT_EQUAL("missing begin", "expected {, got: 34",
		     parser.getError());
	ASSERT_TRUE("missing begin", value == nullptr);

	// missing end
	value = parser.parseObject("{\"key\": 42", 1);
	ASSERT_EQUAL("missing end", "EOF reached while scanning for \" or }",
		     parser.getError());
	ASSERT_TRUE("missing end", value == nullptr);
}

void
TestJson::testParseArray()
{
	TestJsonParser parser;
	JsonValueArray *value;

	// empty
	value = parser.parseArray("[]", 1);
	ASSERT_EQUAL("empty", "", parser.getError());
	ASSERT_TRUE("empty", value != nullptr);
	ASSERT_EQUAL("empty", 0, value->size());
	delete value;

	// single value
	value = parser.parseArray("[ 42 ]", 1);
	ASSERT_EQUAL("single value", "", parser.getError());
	ASSERT_TRUE("single value", value != nullptr);
	ASSERT_EQUAL("single value", 1, value->size());
	ASSERT_EQUAL("single value", 42, *(*jsonGetNumber(value, 0)));
	delete value;

	// multiple values
	value = parser.parseArray("[\"v1\",2]", 1);
	ASSERT_EQUAL("multiple values", "", parser.getError());
	ASSERT_TRUE("multiple values", value != nullptr);
	ASSERT_EQUAL("multiple values", 2, value->size());
	ASSERT_EQUAL("multiple values", "v1", *(*jsonGetString(value, 0)));
	ASSERT_EQUAL("multiple values", 2, *(*jsonGetNumber(value, 1)));
	delete value;

	// missing begin
	value = parser.parseArray("]", 1);
	ASSERT_EQUAL("missing begin", "expected [, got: 93", parser.getError());
	ASSERT_TRUE("missing begin", value == nullptr);
	delete value;

	// missing end
	value = parser.parseArray("[1,2", 1);
	ASSERT_EQUAL("missing end", "EOF reached while scanning for , or ]",
		     parser.getError());
	ASSERT_TRUE("missing end", value == nullptr);
	delete value;
}

void
TestJson::testParseString()
{
	TestJsonParser parser;
	JsonValueString *value;

	// empty
	value = parser.parseString("\"\"", 1);
	ASSERT_TRUE("empty", value != nullptr);
	ASSERT_EQUAL("empty", "", *(*value));
	delete value;

	// simple
	value = parser.parseString("\"simple\"", 1);
	ASSERT_EQUAL("simple", "", parser.getError());
	ASSERT_TRUE("simple", value != nullptr);
	ASSERT_EQUAL("simple", "simple", *(*value));
	delete value;

	// missing begin
	value = parser.parseString("no begin\"", 1);
	ASSERT_EQUAL("no begin", "expected \", got: 110", parser.getError());
	ASSERT_TRUE("no begin", value == nullptr);

	// missing end
	value = parser.parseString("\"no end", 1);
	ASSERT_EQUAL("no end", "EOF reached while scanning for \"",
		     parser.getError());
	ASSERT_TRUE("no end", value == nullptr);

	// invalid character
	value = parser.parseString("\"\003\"", 1);
	ASSERT_EQUAL("invalid character", "invalid character 3 in string",
		     parser.getError());
	ASSERT_TRUE("invalid character", value == nullptr);

	// simple escape values
	value = parser.parseString("\"\\\"\\\\\\/\"", 1);
	ASSERT_EQUAL("non-ws escape", "", parser.getError());
	ASSERT_TRUE("non-ws escape", value != nullptr);
	ASSERT_EQUAL("non-ws escape", "\"\\/", *(*value));
	delete value;

	// control escape
	value = parser.parseString("\"\\b\\f\\n\\r\\t\"", 1);
	ASSERT_EQUAL("ws escape", "", parser.getError());
	ASSERT_TRUE("ws escape", value != nullptr);
	ASSERT_EQUAL("ws escape", "\b\f\n\r\t", *(*value));
	delete value;

	// valid hex escape
	value = parser.parseString("\"\\u09E6\"", 1);
	ASSERT_EQUAL("hex escape", "", parser.getError());
	ASSERT_TRUE("hex escape", value != nullptr);
	ASSERT_EQUAL("hex escape", "\xe0\xa7\xa6", *(*value));
	delete value;

	// short hex escape
	value = parser.parseString("\"\\u09E", 1);
	ASSERT_EQUAL("hex escape", "unexpected EOF, reading 4 bytes",
		     parser.getError());
	ASSERT_TRUE("hex escape", value == nullptr);

	// non hex, hex escape
	value = parser.parseString("\"\\uz9E6\"", 1);
	ASSERT_EQUAL("hex escape", "z9E6 not a valid unicode escape sequence",
		     parser.getError());
	ASSERT_TRUE("hex escape", value == nullptr);
}

void
TestJson::testParseNumber()
{
	TestJsonParser parser;
	JsonValueNumber *value;

	// leading -
	value = parser.parseNumber("-1", 1);
	ASSERT_EQUAL("leading -", "", parser.getError());
	ASSERT_TRUE("leading -", value != nullptr);
	ASSERT_DOUBLE_EQUAL("leading -", -1.0, *(*value));
	delete value;

	// leading -0.
	value = parser.parseNumber("-0.721", 1);
	ASSERT_EQUAL("leading -0.", "", parser.getError());
	ASSERT_TRUE("leading -0.", value != nullptr);
	ASSERT_DOUBLE_EQUAL("leading -0.", -0.721, *(*value));
	delete value;

	// 0
	value = parser.parseNumber("0", 1);
	ASSERT_EQUAL("0.", "", parser.getError());
	ASSERT_TRUE("0", value != nullptr);
	ASSERT_DOUBLE_EQUAL("0", 0.0, *(*value));
	delete value;

	// leading 0.
	value = parser.parseNumber("0.833", 1);
	ASSERT_EQUAL("leading 0.", "", parser.getError());
	ASSERT_TRUE("leading 0.", value != nullptr);
	ASSERT_DOUBLE_EQUAL("leading 0.", 0.833, *(*value));
	delete value;

	// integer
	value = parser.parseNumber("1238", 1);
	ASSERT_EQUAL("integer", "", parser.getError());
	ASSERT_TRUE("integer", value != nullptr);
	ASSERT_DOUBLE_EQUAL("integer", 1238.0, *(*value));
	delete value;

	// fraction
	value = parser.parseNumber("123.456", 1);
	ASSERT_EQUAL("fraction", "", parser.getError());
	ASSERT_TRUE("fraction", value != nullptr);
	ASSERT_DOUBLE_EQUAL("fraction", 123.456, *(*value));
	delete value;

	// e
	value = parser.parseNumber("3e5", 1);
	ASSERT_EQUAL("e", "", parser.getError());
	ASSERT_TRUE("e", value != nullptr);
	ASSERT_DOUBLE_EQUAL("e", 300000, *(*value));
	delete value;

	// exponent E
	value = parser.parseNumber("3E10", 1);
	ASSERT_EQUAL("E", "", parser.getError());
	ASSERT_TRUE("E", value != nullptr);
	ASSERT_DOUBLE_EQUAL("E", 30000000000, *(*value));
	delete value;

	// exponent e-
	value = parser.parseNumber("-1.23e-08", 1);
	ASSERT_EQUAL("e-", "", parser.getError());
	ASSERT_TRUE("e-", value != nullptr);
	ASSERT_DOUBLE_EQUAL("e-", -0.0000000123, *(*value));
	delete value;

	// exponent E+
	value = parser.parseNumber("1.23E+2", 1);
	ASSERT_EQUAL("E+", "", parser.getError());
	ASSERT_TRUE("E+", value != nullptr);
	ASSERT_DOUBLE_EQUAL("E+", 123, *(*value));
	delete value;

	// invalid numbers
	value = parser.parseNumber("00", 1);
	ASSERT_EQUAL("leading 00", "only one leading 0 is allowed in number",
		     parser.getError());

	value = parser.parseNumber("-00", 1);
	ASSERT_EQUAL("leading -00", "only one leading 0 is allowed in number",
		     parser.getError());

	value = parser.parseNumber("10-10", 1);
	ASSERT_EQUAL("middle -", "- is only allowed as first character and "
		     "after e,E in number", parser.getError());

	value = parser.parseNumber("10+10", 1);
	ASSERT_EQUAL("middle +", "+ is only allowed after e,E in number",
		     parser.getError());

	value = parser.parseNumber(".23", 1);
	ASSERT_EQUAL("leading .", ". is not allowed as first character in "
		     "number", parser.getError());

	value = parser.parseNumber("1.2.3", 1);
	ASSERT_EQUAL("multiple .", "only one . is allowed in number",
		     parser.getError());



}

void
TestJson::testParseBoolean()
{
	TestJsonParser parser;
	JsonValueBoolean *value;

	value = parser.parseBoolean("true", 1);
	ASSERT_TRUE("true", value != nullptr);
	ASSERT_TRUE("true", *(*value));
	delete value;

	value = parser.parseBoolean("false", 1);
	ASSERT_TRUE("false", value != nullptr);
	ASSERT_FALSE("false", *(*value));
	delete value;

	value = parser.parseBoolean("fa", 1);
	ASSERT_TRUE("short", value == nullptr);
	ASSERT_EQUAL("short", "unexpected EOF, reading 4 bytes",
		     parser.getError());

	value = parser.parseBoolean("finvalid", 1);
	ASSERT_TRUE("invalid", value == nullptr);
	ASSERT_EQUAL("invalid", "expected true or false, got: finva",
		     parser.getError());
}

void
TestJson::testParseNull()
{
	TestJsonParser parser;
	JsonValueNull *value;

	value = parser.parseNull("null", 1);
	ASSERT_TRUE("null", value != nullptr);
	delete value;

	value = parser.parseNull("nu", 1);
	ASSERT_TRUE("short", value == nullptr);
	ASSERT_EQUAL("short", "unexpected EOF, reading 3 bytes",
		     parser.getError());

	value = parser.parseNull("notanull", 1);
	ASSERT_TRUE("invalid", value == nullptr);
	ASSERT_EQUAL("invalid", "expected null, got: nota", parser.getError());
}
