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

#ifndef _CFG_PARSER_KEY_HH_
#define _CFG_PARSER_KEY_HH_

#include <string>
#include <limits>

//! @brief CfgParserKey value type.
enum CfgParserKeyType {
  KEY_SECTION, //!< Subsection.
  KEY_BOOL, //!< Boolean value.
  KEY_INT, //!< Integer value.
  KEY_STRING, //!< String value.
  KEY_PATH //!< Path value.
};

//! @brief CfgParserKey base class.
class CfgParserKey {
public:
  //! @brief CfgParserKey constructor.
  CfgParserKey (const char *op_name) : m_op_name (op_name) { }
  //! @brief CfgParserKey destructor.
  virtual ~CfgParserKey (void) { }

  //! @brief Returns Key name.
  const char *get_name (void) const { return m_op_name; }
  //! @brief Returns Key type.
  const CfgParserKeyType get_type (void) const { return m_type; }

  //! @brief Parses or_value and sets Key value.
  virtual void parse_value (const std::string &or_value) throw (std::string&) {(&or_value); }

protected:
  CfgParserKeyType m_type; //!< Key type.
  const char *m_op_name; //!< Key name.
};


//! @brief CfgParser Key integer value parser.
class CfgParserKeyInt : public CfgParserKey {
public:
  //! @brief CfgParserKeyInt constructor.
  CfgParserKeyInt (const char *op_name,
                   int &ir_set, const int i_default = 0,
                   const int i_value_min = std::numeric_limits<int>::min (),
                   const int i_value_max = std::numeric_limits<int>::max ()) :
    CfgParserKey (op_name),
    m_ir_set (ir_set), m_i_default (i_default),
    m_i_value_min (i_value_min), m_i_value_max (i_value_max) {
    m_type = KEY_INT;
  }
  //! @brief CfgParserKeyInt destructor.
  virtual ~CfgParserKeyInt (void) { }

  virtual void parse_value (const std::string &or_value) throw (std::string&);

private:
  int &m_ir_set; //! Reference to store parsed value in.
  const int m_i_default; //! Default value.
  const int m_i_value_min; //! Minimum value.
  const int m_i_value_max; //! Maximum value.
};

//! @brief CfgParser Key boolean value parser.
class CfgParserKeyBool : public CfgParserKey {
public:
  //! @brief CfgParserKeyBool constructor.
  CfgParserKeyBool (const char *op_name,
                    bool &ib_set, const bool b_default = false) :
    CfgParserKey (op_name),
    m_br_set (ib_set), m_b_default (b_default) { m_type = KEY_BOOL; }
  //! @brief CfgParserKeyBool destructor.
  virtual ~CfgParserKeyBool (void) { }

  virtual void parse_value (const std::string &or_value) throw (std::string&);

private:
  bool &m_br_set; //! Reference to stored parsed value in.
  const bool m_b_default; //! Default value.
};

//! @brief CfgParser Key string value parser.
class CfgParserKeyString : public CfgParserKey {
public:
  //! @brief CfgParserKeyString constructor.
  CfgParserKeyString (const char *op_name,
                      std::string &or_set, const std::string o_default = "",
                      const int i_length_min = std::numeric_limits<int>::min (),
                      const int i_length_max = std::numeric_limits<int>::max ()) :
    CfgParserKey (op_name),
    m_or_set (or_set), m_o_default (o_default),
    m_i_length_min (i_length_min), m_i_length_max (i_length_max) {
    m_type = KEY_STRING;
  }
  //! @brief CfgParserKeyString destructor.
  virtual ~CfgParserKeyString (void) { }

  virtual void parse_value (const std::string &or_value) throw (std::string&);

private:
  std::string &m_or_set; //!< Reference to store parsed value in.
  const std::string m_o_default; //!< Default value.
  const int m_i_length_min; //!< Minimum lenght of string.
  const int m_i_length_max; //!< Maximum length of string.
};

//! @brief CfgParser Key path parser.
class CfgParserKeyPath : public CfgParserKey {
public:
  //! @brief CfgParserKeyPath constructor.
  CfgParserKeyPath (const char *op_name,
                    std::string &or_set, const std::string o_default = "") :
    CfgParserKey (op_name),
    m_or_set (or_set), m_o_default (o_default) { m_type = KEY_PATH; }
  //! @brief CfgParserKeyPath destructor.
  virtual ~CfgParserKeyPath (void) { }

  virtual void parse_value (const std::string &or_value) throw (std::string&);

private:
  std::string &m_or_set; //!< Reference to store parsed value in.
  const std::string m_o_default; //!< Default value.
};

#endif // _CFG_PARSER_KEY_HH_
