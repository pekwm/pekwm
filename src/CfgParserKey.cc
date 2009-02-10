//
// Copyright © 2005-2009 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "CfgParserKey.hh"
#include "Util.hh"

#include <iostream>
#include <cstdlib>

using std::string;
using std::cerr;
using std::endl;
using std::numeric_limits;


//! @brief Parses value and sets _br_set.
//! Boolean true is represented either by case insensitive true or 1.
//! Boolean false is represented either by case insensitive false or 0.
//! @param value Value to parse.
//! @return true on success, else false and _br_set set to _b_default.
void
CfgParserKeyBool::parse_value(const std::string &value)
    throw (std::string&)
{
    if ((value == "1") || ! strcasecmp(value.c_str(), "TRUE")) {
        _set = true;
    } else if ((value == "0") || ! strcasecmp(value.c_str(), "FALSE")) {
        _set = false;
    } else  {
        _set = _default;
        throw string("not bool value");
    }
}

//! @brief Parses value and sets _set.
//! @param value Value to parse.
//! @return true on success, else false and _set set to _default.
void
CfgParserKeyString::parse_value(const std::string &value)
    throw (std::string&)
{
    _set = value;

    if ((_length_min != numeric_limits<int>::min())
        && (static_cast<int>(_set.size()) < _length_min)) {
        _set = _default;
        throw string("string too short, min length " + Util::to_string<int>(_length_min));
    }
    if ((_length_max != numeric_limits<int>::max())
        && (static_cast<int>(_set.size()) > _length_max))  {
        _set = _default;
        throw string("string too long, max length " + Util::to_string<int>(_length_max));
    }
}

//! @brief Parses value and sets _set.
//! @param value Value to parse.
//! @return true on success, else false and _set set to _default.
void
CfgParserKeyPath::parse_value(const std::string &value)
    throw (std::string&)
{
    if (value.size()) {
        _set = value;
        Util::expandFileName(_set);
    } else {
        _set = _default;
        throw string("path too short");
    }
}
