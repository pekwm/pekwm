//
// Json.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "Json.hh"

#include <sstream>

extern "C" {
#include <locale.h>
}

JsonValueNull _null_value = JsonValueNull();

static bool
_is_space(char chr)
{
	switch(chr) {
	case ' ':
	case '\n':
	case '\r':
	case '\t':
		return true;
	default:
		return false;
	}
}

std::ostream&
operator<<(std::ostream &os, const JsonValueObject& val)
{
	os << "{";
	JsonValueObject::map::const_iterator it(val.begin());
	for (; it != val.end(); ++it) {
		if (it != val.begin()) {
			os << ", ";
		}
		os << "\"" << it->first << "\": " << *it->second;
	}
	os << "}";
	return os;
}

std::ostream&
operator<<(std::ostream &os, const JsonValueArray& val)
{
	os << "[";
	JsonValueArray::vector::const_iterator it(val.begin());
	for (; it != val.end(); ++it) {
		if (it != val.begin()) {
			os << ", ";
		}
		os << *(*it);
	}
	os << "]";
	return os;
}

std::ostream&
operator<<(std::ostream &os, const JsonValue& val)
{
	switch (val.getType()) {
	case JSON_TYPE_OBJECT:
		os << static_cast<const JsonValueObject&>(val);
		break;
	case JSON_TYPE_ARRAY:
		os << static_cast<const JsonValueArray&>(val);
		break;
	case JSON_TYPE_STRING:
		os << "\"" << *static_cast<const JsonValueString&>(val)
		   << "\"";
		break;
	case JSON_TYPE_NUMBER:
		os << *static_cast<const JsonValueNumber&>(val);
		break;
	case JSON_TYPE_BOOLEAN:
		if (*static_cast<const JsonValueBoolean&>(val)) {
			os << "true";
		} else {
			os << "false";
		}
		break;
	case JSON_TYPE_NULL:
		os << "null";
		break;
	default:
		os << "ERROR";
		break;
	};
	return os;
}

template<typename T>
static T*
_jsonGetArray(JsonValue *value, size_t pos)
{
	JsonValueArray *arr = dynamic_cast<JsonValueArray*>(value);
	if (arr) {
		return dynamic_cast<T*>((*arr)[pos]);
	}
	return nullptr;
}

template<typename T>
static T*
_jsonGetObject(JsonValue *value, const std::string &key)
{
	JsonValueObject *obj = dynamic_cast<JsonValueObject*>(value);
	if (obj) {
		return dynamic_cast<T*>((*obj)[key]);
	}
	return nullptr;
}

JsonValueObject*
jsonGetObject(JsonValue *value, size_t pos)
{
	return _jsonGetArray<JsonValueObject>(value, pos);
}

JsonValueObject*
jsonGetObject(JsonValue *value, const std::string &key)
{
	return _jsonGetObject<JsonValueObject>(value, key);
}

JsonValueArray*
jsonGetArray(JsonValue *value, size_t pos)
{
	return _jsonGetArray<JsonValueArray>(value, pos);
}

JsonValueArray*
jsonGetArray(JsonValue *value, const std::string &key)
{
	return _jsonGetObject<JsonValueArray>(value, key);
}

JsonValueString*
jsonGetString(JsonValue *value, size_t pos)
{
	return _jsonGetArray<JsonValueString>(value, pos);
}

JsonValueString*
jsonGetString(JsonValue *value, const std::string &key)
{
	return _jsonGetObject<JsonValueString>(value, key);
}

JsonValueNumber*
jsonGetNumber(JsonValue *value, size_t pos)
{
	return _jsonGetArray<JsonValueNumber>(value, pos);
}

JsonValueNumber*
jsonGetNumber(JsonValue *value, const std::string &key)
{
	return _jsonGetObject<JsonValueNumber>(value, key);
}

JsonValueBoolean*
jsonGetBoolean(JsonValue *value, size_t pos)
{
	return _jsonGetArray<JsonValueBoolean>(value, pos);
}

JsonValueBoolean*
jsonGetBoolean(JsonValue *value, const std::string &key)
{
	return _jsonGetObject<JsonValueBoolean>(value, key);
}

/**
 * Create a new JSON parser
 */
JsonParser::JsonParser(const std::string &str)
	: _data_buf(str),
	  _data(_data_buf, 0),
	  _line(0),
	  _pos(0)
{
	initLocale();
}

JsonParser::~JsonParser()
{
}

void
JsonParser::initLocale()
{
	struct lconv *lc = localeconv();
	_decimal_point = lc->decimal_point;
}

JsonValueObject*
JsonParser::parse()
{
	if (! skipWhitespace("{")) {
		return nullptr;
	}
	return parseObject();
}

bool
JsonParser::fill(size_t bytes)
{
	size_t bytes_left = bytes;
	while (! isError() && bytes_left > 0) {
		const char *c = next();
		if (*c) {
			_buf += c;
			bytes_left -= std::min(bytes_left, strlen(c));
		} else {
			_error << "unexpected EOF, reading " << bytes
			       << " bytes";
		}
	}
	return bytes_left == 0;
}

JsonValue*
JsonParser::parseValue(char skip_error_on)
{
	if (! skipWhitespace("value start")) {
		return nullptr;
	}

	char c = chr()[0];
	switch (c) {
	case '"':
		return parseString();
	case '-':
		return parseNumber();
	case '{':
		return parseObject();
	case '[':
		return parseArray();
	case 't':
	case 'f':
		return parseBoolean();
	case 'n':
		return parseNull();
	default:
		if (c >= '0' && c <= '9') {
			return parseNumber();
		} else if (! (_data == skip_error_on)) {
			_error << "unexpected character: "
			       << static_cast<int>(c);
		}
		return nullptr;
	}
}

JsonValueObject*
JsonParser::parseObject()
{
	if (! (_data == '{')) {
		_error << "expected {, got: "
		       << static_cast<int>(chr()[0]);
		return nullptr;
	}

	bool end = false;
	JsonValueObject *obj = new JsonValueObject();
	char expect = '\0';
	while (! end && ! isError() && skipWhitespace("\" or }")) {
		if (_data == '}') {
			end = true;
		} else if (expect != '\0' && _data == expect) {
			expect = '\0';
		} else if (_data == '"') {
			JsonValueString *key = parseString();
			if (key != nullptr) {
				skipWhitespace(":");
				if (_data == ':') {
					JsonValue *value = parseValue();
					if (value != nullptr) {
						obj->set(*(*key), value);
						expect = ',';
					}
				}
				delete key;
			}
		} else {
			_error << "expected \" or }, got: "
			       << static_cast<int>(chr()[0]);
		}
	}

	if (! end || isError()) {
		if (! isError()) {
			_error << "EOF reached while scanning for }";
		}
		delete obj;
		return nullptr;
	}
	return obj;
}

JsonValueArray*
JsonParser::parseArray()
{
	if (! (_data == '[')) {
		_error << "expected [, got: " << static_cast<int>(chr()[0]);
		return nullptr;
	}

	bool end = false;
	JsonValueArray *arr = new JsonValueArray();
	char expect = '\0';
	do {
		if (_data == ']') {
			end = true;
		} else if (expect != '\0' && _data == expect) {
			expect = '\0';
		} else {
			JsonValue *value = parseValue(']');
			if (value == nullptr) {
				end = chr()[0] == ']';
			} else {
				arr->add(value);
				skipWhitespace(", or ]");
				expect = ',';
			}
		}
	} while (! end && ! isError());

	if (! end || isError()) {
		if (! isError()) {
			_error << "EOF reached while scanning for ]";
		}
		delete arr;
		return nullptr;
	}
	return arr;
}

JsonValueString*
JsonParser::parseString()
{
	if (! (_data == '"')) {
		_error << "expected \", got: " << static_cast<int>(chr()[0]);
		return nullptr;
	}

	const char *c;
	std::string str;
	bool in_end = false;
	bool in_escape = false;
	while (! in_end && ! isError() && *(c = next())) {
		if (in_escape) {
			parseStringEscape(c[0], str);
			in_escape = false;
		} else if (c[0] == '\\') {
			in_escape = true;
		} else if (c[0] == '"') {
			in_end = true;
		} else if (c[1] == '\0' && c[0] >= 0 && c[0] < 32) {
			// error on control character
			_error << "invalid character "
			       << static_cast<int>(c[0]) << " in string";
		} else {
			str += c;
		}
	}

	if (in_end) {
		return new JsonValueString(str);
	} else if (! isError()) {
		_error << "EOF reached while scanning for \"";
	}
	return nullptr;
}

bool
JsonParser::parseStringEscape(char c, std::string &str)
{
	bool error = false;
	switch (c) {
	case '"':
	case '\\':
	case '/':
		str += c;
		break;
	case 'b':
		str += '\b';
		break;
	case 'f':
		str += '\f';
		break;
	case 'n':
		str += '\n';
		break;
	case 'r':
		str += '\r';
		break;
	case 't':
		str += '\t';
		break;
	case 'u':
		error = parseStringEscapeHex(str);
		break;
	default:
		error = true;
		_error << "unexpected string escape character "
		       << static_cast<int>(chr()[0]);
		break;
	}
	return error;
}

static bool _from_hex(char c, uint32_t &val, int shift)
{
	if (c >= '0' && c <= '9') {
		val |= (c - '0') << shift;
	} else if (c >= 'a' && c <= 'f') {
		val |= (10 + (c - 'a')) << shift;
	} else if (c >= 'A' && c <= 'F') {
		val |= (10 + (c - 'A')) << shift;
	} else {
		return false;
	}
	return true;
}

bool
JsonParser::parseStringEscapeHex(std::string &str)
{
	if (! fill(4)) {
		return false;
	}
	uint32_t val = 0;
	if (_from_hex(value()[1], val, 12) && _from_hex(value()[2], val, 8)
	    && _from_hex(value()[3], val, 4) && _from_hex(value()[4], val, 0)) {
		// encode value as UTF-8 character
		Charset::toUtf8(val, str);
		return true;
	}
	_error << value().substr(1) << " not a valid unicode escape sequence";
	return false;
}

JsonValueNumber*
JsonParser::parseNumber()
{
	bool dot_allowed = true;
	bool minus_allowed = true;
	bool plus_allowed = false;
	bool e_allowed = true;
	std::string num;
	const char *c = chr();
	do {
		if (c[0] == '-') {
			if (! num.empty() && ! minus_allowed) {
				_error << "- is only allowed as first "
				       << "character and after e,E in number";
			}
			minus_allowed = false;
		} else if (c[0] == '+') {
			if (! plus_allowed) {
				_error << "+ is only allowed after e,E in "
				       << "number";
			}
			plus_allowed = false;
		} else if (c[0] == '0') {
			if (num.size() < 3 && (num == "0" || num == "-0")) {
				_error << "only one leading 0 is allowed in "
				       << "number";
			}
		} else if (isdigit(c[0])) {
			minus_allowed = false;
			plus_allowed = false;
		} else if (c[0] == 'e' || c[0] == 'E') {
			if (e_allowed) {
				e_allowed = false;
				minus_allowed = true;
				plus_allowed = true;
			} else {
				_error << "only one " << c[0] << " is allowed "
				       << "in number";
			}
		} else if (c[0] == '.') {
			if (num.empty()) {
				_error << ". is not allowed as first "
				       << "character in number";
			} else if (! dot_allowed) {
				_error << "only one . is allowed in number";
			} else {
				dot_allowed = false;
				c = _decimal_point.c_str();
			}
		} else {
			--_data;
			break;
		}

		if (! isError()) {
			num += c;
		}
	} while (! isError() && *(c = next()));

	if (isError()) {
		return nullptr;
	}

	try {
		double value = std::stod(num);
		return new JsonValueNumber(value);
	} catch (std::invalid_argument &ex) {
		_error << "invalid number " << num << ": " << ex.what();
		return nullptr;
	}
}

JsonValueBoolean*
JsonParser::parseBoolean()
{
	if (_data == 't') {
		if (fill(3) && value() == "true") {
			return new JsonValueBoolean(true);
		}
	} else if (_data == 'f') {
		if (fill(4) && value() == "false") {
			return new JsonValueBoolean(false);
		}
	}

	if (! eof()) {
		_error << "expected true or false, got: " << value();
	}
	return nullptr;
}

JsonValueNull*
JsonParser::parseNull()
{
	if (_data == 'n' && fill(3) && value() == "null") {
		return new JsonValueNull();
	}
	if (! eof()) {
		_error << "expected null, got: " << value();
	}
	return nullptr;

}

bool
JsonParser::skipWhitespace(const char *scan)
{
	const char *c;
	while (*(c = next())) {
		if (! _is_space(c[0])) {
			_buf = c;
			return true;
		}
	}
	_error << "EOF reached while scanning for " << scan;
	return false;
}
