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
//! @return true on success, else false and m_ir_set to m_i_default.
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
    m_ir_set = m_i_default;

  } else {
    if (value < m_i_value_min) {
      m_ir_set = m_i_value_min;
      throw string("value to low, min value "
                   + Util::to_string<int> (m_i_value_min));
    } if (value > m_i_value_max)  {
      m_ir_set = m_i_value_max;
      throw string("value to high, max value "
                   + Util::to_string<int> (m_i_value_max));
    }
  }

  m_ir_set = value;
}

//! @brief Parses or_value and sets m_br_set.
//! Boolean true is represented either by case insensitive true or 1.
//! Boolean false is represented either by case insensitive false or 0.
//! @param or_value Value to parse.
//! @return true on success, else false and m_br_set set to m_b_default.
void
CfgParserKeyBool::parse_value (const std::string &or_value)
  throw (std::string&)
{
  if ((or_value == "1") || !strcasecmp (or_value.c_str(), "TRUE")) {
    m_br_set = true;
  } else if ((or_value == "0") || !strcasecmp (or_value.c_str(), "FALSE")) {
    m_br_set = false;
  } else  {
    m_br_set = m_b_default;
    throw string ("not bool value");
  }
}

//! @brief Parses or_value and sets m_or_set.
//! @param or_value Value to parse.
//! @return true on success, else false and m_or_set set to m_o_default.
void
CfgParserKeyString::parse_value (const std::string &or_value)
  throw (std::string&)
{
  m_or_set = or_value;

  if ((m_i_length_min != numeric_limits<int>::min ())
      && (static_cast<int> (m_or_set.size()) < m_i_length_min)) {
    m_or_set = m_o_default;
    throw string ("string too short, min length "
                  + Util::to_string<int> (m_i_length_min));
  }
  if ((m_i_length_max != numeric_limits<int>::max ())
      && (static_cast<int> (m_or_set.size()) > m_i_length_max))  {
    m_or_set = m_o_default;
    throw string ("string too long, max length "
                  + Util::to_string<int> (m_i_length_max));
  }
}

//! @brief Parses or_value and sets m_or_set.
//! @param or_value Value to parse.
//! @return true on success, else false and m_or_set set to m_o_default.
void
CfgParserKeyPath::parse_value (const std::string &or_value)
  throw (std::string&)
{
  if (or_value.size ()) {
    m_or_set = or_value;
    Util::expandFileName (m_or_set);
  } else {
    m_or_set = m_o_default;
    throw ("path too short");
  }
}
