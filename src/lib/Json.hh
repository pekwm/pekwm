//
// Json.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_JSON_HH_
#define _PEKWM_JSON_HH_

#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "Compat.hh"
#include "Charset.hh"

extern "C" {
#include <string.h>
}

enum JsonType {
	JSON_TYPE_OBJECT,
	JSON_TYPE_ARRAY,
	JSON_TYPE_STRING,
	JSON_TYPE_NUMBER,
	JSON_TYPE_BOOLEAN,
	JSON_TYPE_NULL
};

class JsonValue {
public:
	JsonValue(JsonType type)
		: _type(type)
	{
	}

	virtual ~JsonValue() { }
	enum JsonType getType() const { return _type; }

private:
	JsonType _type;
};

class JsonValueNull : public JsonValue {
public:
	JsonValueNull()
		: JsonValue(JSON_TYPE_NULL)
	{
	}
	virtual ~JsonValueNull() { }
};

extern JsonValueNull _null_value;

class JsonValueObject : public JsonValue {
public:
	typedef std::map<std::string, JsonValue*> map;

	JsonValueObject()
		: JsonValue(JSON_TYPE_OBJECT)
	{
	}
	virtual ~JsonValueObject()
	{
		map::iterator it(_values.begin());
		for (; it != _values.end(); ++it) {
			delete it->second;
		}
	}

	size_t size() const { return _values.size(); }
	map::const_iterator begin() const { return _values.begin(); }
	map::const_iterator end() const { return _values.end(); }

	JsonValue *operator[](const std::string &key)
	{
		map::iterator it(_values.find(key));
		return it == _values.end() ? &_null_value : it->second;
	}

	void set(const std::string &key, JsonValue *value)
	{
		map::iterator it(_values.find(key));
		if (it == _values.end()) {
			_values[key] = value;
		} else {
			delete it->second;
			it->second = value;
		}
	}

private:
	std::map<std::string, JsonValue*> _values;
};

std::ostream& operator<<(std::ostream &os, const JsonValueObject& val);

class JsonValueArray : public JsonValue {
public:
	typedef std::vector<JsonValue*> vector;

	JsonValueArray()
		: JsonValue(JSON_TYPE_ARRAY)
	{
	}
	virtual ~JsonValueArray()
	{
		vector::iterator it(_values.begin());
		for (; it != _values.end(); ++it) {
			delete *it;
		}
	}

	size_t size() const { return _values.size(); }
	vector::const_iterator begin() const { return _values.begin(); }
	vector::const_iterator end() const { return _values.end(); }

	JsonValue *operator[](size_t pos)
	{
		return pos < _values.size() ? _values[pos] : &_null_value;
	}

	void add(JsonValue* value) { _values.push_back(value); }

private:
	vector _values;
};

std::ostream& operator<<(std::ostream &os, const JsonValueArray& val);

class JsonValueString : public JsonValue {
public:
	JsonValueString(const std::string& value)
		: JsonValue(JSON_TYPE_STRING),
		  _value(value)
	{
	}
	virtual ~JsonValueString() { }

	const std::string &operator*() const { return _value; }

private:
	std::string _value;

};

class JsonValueNumber : public JsonValue {
public:
	JsonValueNumber(double value)
		: JsonValue(JSON_TYPE_NUMBER),
		  _value(value)
	{
	}
	virtual ~JsonValueNumber() { }

	double operator*() const { return _value; }

private:
	double _value;
};

class JsonValueBoolean : public JsonValue {
public:
	JsonValueBoolean(bool value)
		: JsonValue(JSON_TYPE_BOOLEAN),
		  _value(value)
	{
	}

	virtual ~JsonValueBoolean() { }

	bool operator*() const { return _value; }

private:
	bool _value;
};

std::ostream& operator<<(std::ostream &os, const JsonValue& val);

JsonValueObject *jsonGetObject(JsonValue *value, size_t pos);
JsonValueObject *jsonGetObject(JsonValue *value, const std::string &key);
JsonValueArray *jsonGetArray(JsonValue *value, size_t pos);
JsonValueArray *jsonGetArray(JsonValue *value, const std::string &key);
JsonValueString *jsonGetString(JsonValue *value, size_t pos);
JsonValueString *jsonGetString(JsonValue *value, const std::string &key);
JsonValueNumber *jsonGetNumber(JsonValue *value, size_t pos);
JsonValueNumber *jsonGetNumber(JsonValue *value, const std::string &key);
JsonValueBoolean *jsonGetBoolean(JsonValue *value, size_t pos);
JsonValueBoolean *jsonGetBoolean(JsonValue *value, const std::string &key);

class JsonParser {
public:
	JsonParser(const std::string &str);
	~JsonParser();

	JsonValueObject* parse();

	int getLine() const { return _line; }
	bool isError() const { return ! _error.str().empty(); }
	std::string getError() const { return _error.str(); }

protected:
	JsonValue* parseValue(char skip_error_on = '\0');
	JsonValueObject* parseObject();
	JsonValueArray* parseArray();
	JsonValueString* parseString();
	bool parseStringEscape(char c, std::string &str);
	bool parseStringEscapeHex(std::string &str);
	JsonValueNumber* parseNumber();
	JsonValueBoolean* parseBoolean();
	JsonValueNull* parseNull();

	const char *chr() const { return _buf.c_str(); }
	const char *next()
	{
		if (_pos != 0) {
			++_data;
		}
		const char *c = *_data;
		if (c[0] == '\0') {
		} else if (c[1] == '\0') {
			_pos += 1;
			if (c[0] == '\n') {
				_line++;
			}
		} else {
			_pos += strlen(c);
		}
		return c;

	}
	bool skipWhitespace(const char *scan);
	const std::string& value() { return _buf; }
	bool fill(size_t bytes);
	bool eof() const { return _data.end(); }

	std::string _data_buf;
	Charset::Utf8Iterator _data;
	std::stringstream _error;

private:
	void initLocale();

	int _line;
	int _pos;
	std::string _buf;
	std::string _decimal_point;
};

#endif // _PEKWM_JSON_HH_
