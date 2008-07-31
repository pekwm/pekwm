//! @file
//! @author Claes Nasten <pekdon{@}pekdon{.}net
//! @date 2005-05-30
//! @brief Configuration value parser.

//
// Copyright (C) 2005 Claes Nasten <pekdon{@}pekdon{.}net>
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

//! @brief Parses and store integer value.
//! @param or_value Reference to string representing integer value.
//! @return true on success, else false and _ir_set to _i_default.
void
CfgParserKeyInt::parse_value (const std::string &or_value)
throw (std::string&)
{
    long value;
    char *endptr;

    // Get long value.
    value = strtol (or_value.c_str (), &endptr, 10);

    // Check for validity, 0 is returned on failiure with endptr set to the
    // beginning of the string, else we are (semi) ok.
    if ((value == 0) && (endptr == or_value.c_str ())) {
        _ir_set = _i_default;

    } else {
        if (value < _i_value_min) {
            _ir_set = _i_value_min;
            throw string ("value to low, min value "
                          + Util::to_string<int> (_i_value_min));
        } if (value > _i_value_max)  {
            _ir_set = _i_value_max;
            throw string ("value to high, max value "
                          + Util::to_string<int> (_i_value_max));
        }
    }

    _ir_set = value;
}

//! @brief Parses or_value and sets _br_set.
//! Boolean true is represented either by case insensitive true or 1.
//! Boolean false is represented either by case insensitive false or 0.
//! @param or_value Value to parse.
//! @return true on success, else false and _br_set set to _b_default.
void
CfgParserKeyBool::parse_value (const std::string &or_value)
throw (std::string&)
{
    if ((or_value == "1") || ! strcasecmp(or_value.c_str(), "TRUE")) {
        _br_set = true;
    } else if ((or_value == "0") || ! strcasecmp(or_value.c_str(), "FALSE")) {
        _br_set = false;
    } else  {
        _br_set = _b_default;
        throw string ("not bool value");
    }
}

//! @brief Parses or_value and sets _or_set.
//! @param or_value Value to parse.
//! @return true on success, else false and _or_set set to _o_default.
void
CfgParserKeyString::parse_value (const std::string &or_value)
throw (std::string&)
{
    _or_set = or_value;

    if ((_i_length_min != numeric_limits<int>::min ())
            && (static_cast<int> (_or_set.size()) < _i_length_min)) {
        _or_set = _o_default;
        throw string ("string too short, min length "
                      + Util::to_string<int> (_i_length_min));
    }
    if ((_i_length_max != numeric_limits<int>::max ())
            && (static_cast<int> (_or_set.size()) > _i_length_max))  {
        _or_set = _o_default;
        throw string ("string too long, max length "
                      + Util::to_string<int> (_i_length_max));
    }
}

//! @brief Parses or_value and sets _or_set.
//! @param or_value Value to parse.
//! @return true on success, else false and _or_set set to _o_default.
void
CfgParserKeyPath::parse_value (const std::string &or_value)
throw (std::string&)
{
    if (or_value.size ()) {
        _or_set = or_value;
        Util::expandFileName (_or_set);
    } else {
        _or_set = _o_default;
        throw string ("path too short");
    }
}
