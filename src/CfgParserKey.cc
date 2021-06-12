//
// Copyright (C) 2005-2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Compat.hh"
#include "CfgParserKey.hh"
#include "Util.hh"

#include <iostream>
#include <cstdlib>

CfgParserKeyBool::CfgParserKeyBool(const char *name, bool &set,
                                   const bool default_val)
	: CfgParserKey(name),
	  _set(set),
	  _default(default_val)
{
}

CfgParserKeyBool::~CfgParserKeyBool(void)
{
}

/**
 * Parses value and sets _br_set.
 * Boolean true is represented either by case insensitive true or 1.
 * Boolean false is represented either by case insensitive false or 0.
 * @param value Value to parse.
 * @return true on success, else false and _br_set set to _b_default.
 */
void
CfgParserKeyBool::parseValue(const std::string &value)
{
	if ((value == "1") || ! strcasecmp(value.c_str(), "TRUE")) {
		_set = true;
	} else if ((value == "0") || ! strcasecmp(value.c_str(), "FALSE")) {
		_set = false;
	} else  {
		_set = _default;
		throw std::string("not bool value");
	}
}

CfgParserKeyString::CfgParserKeyString(const char *name, std::string &set,
                                       const std::string default_val,
                                       const std::string::size_type length_min)
	: CfgParserKey(name),
	  _set(set),
	  _length_min(length_min)
{
	_set = default_val;
}

CfgParserKeyString::~CfgParserKeyString(void)
{
}

/**
 * Parses value and sets _set.
 * @param value Value to parse.
 * @return true on success, else false and _set set to _default.
 */
void
CfgParserKeyString::parseValue(const std::string &value)
{
	if (value.size() < _length_min) {
		std::string msg = "string too short, min length "
			+ std::to_string(_length_min);
		throw msg;
	}
	_set = value;
}

CfgParserKeyPath::CfgParserKeyPath(const char *name, std::string &set,
                                   const std::string default_val)
	: CfgParserKey(name),
	  _set(set),
	  _default(default_val)
{
	Util::expandFileName(_default);
}

CfgParserKeyPath::~CfgParserKeyPath(void)
{
}

/**
 * Parses value and sets _set.
 *
 * @param value Value to parse.
 * @return true on success, else false and _set set to _default.
 */
void
CfgParserKeyPath::parseValue(const std::string &value)
{
	if (value.size()) {
		_set = value;
		Util::expandFileName(_set);
	} else {
		_set = _default;
		throw std::string("path too short");
	}
}
