//
// Copyright © 2005-2008 Claes Nästén <me@pekdon.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _CFG_PARSER_KEY_HH_
#define _CFG_PARSER_KEY_HH_

#include <string>
#include <limits>
#include <cstdlib>

#include "Util.hh"

//! @brief CfgParserKey value type.
enum CfgParserKeyType {
    KEY_SECTION, //!< Subsection.
    KEY_BOOL, //!< Boolean value.
    KEY_NUMERIC, //!< Integer value.
    KEY_STRING, //!< String value.
    KEY_PATH //!< Path value.
};

//! @brief CfgParserKey base class.
class CfgParserKey {
public:
    //! @brief CfgParserKey constructor.
    CfgParserKey(const char *name) : _name(name) { }
    //! @brief CfgParserKey destructor.
    virtual ~CfgParserKey(void) { }

    //! @brief Returns Key name.
    const char *get_name(void) const { return _name; }
    //! @brief Returns Key type.
    const CfgParserKeyType get_type(void) const { return _type; }

    //! @brief Parses value and sets Key value.
    virtual void parse_value(const std::string &value) throw (std::string&) { }

protected:
    CfgParserKeyType _type; //!< Key type.
    const char *_name; //!< Key name.
};

/**
 * CfgParserKey numeric type with minimum, maximum and default
 * value. The type must be available in numeric_limits.
 */
template<typename T>
class CfgParserKeyNumeric : public CfgParserKey {
public:
    /**
     * CfgParserKeyNumeric constructor, sets default values
     *
     * @param name Name of the key.
     * @param set Variable to store parsed value in.
     * @param default_val Default value for key, defaults to 0.
     * @param value_min Minimum value for key, defaults to limits<T>::min().
     * @param value_min Maximum value for key, defaults to limits<T>::max().
     */
    CfgParserKeyNumeric(const char *name, T &set, const T default_val = 0,
                        const T value_min = std::numeric_limits<T>::min(),
                        const T value_max = std::numeric_limits<T>::max())
        : CfgParserKey(name),
          _set(set), _default(default_val),
          _value_min(value_min), _value_max(value_max)
    {
        _type = KEY_NUMERIC;
    }

    /**
     * CfgParserKeyNumeric destructor.
     */
    virtual ~CfgParserKeyNumeric(void) { }

    /**
     * Parses and store integer value.
     *
     * @param value Reference to string representing integer value.
     */
    virtual void
    parse_value(const std::string &value_str)
        throw (std::string&)
    {
        long double value;
        char *endptr;

        // Get long value.
        value = strtold(value_str.c_str(), &endptr);

        // Check for validity, 0 is returned on failiure with endptr set to the
        // beginning of the string, else we are (semi) ok.
        if ((value == 0) && (endptr == value_str.c_str())) {
            _set = _default;

        } else {
            T value_for_type = static_cast<T>(value);

            if (value_for_type < _value_min) {
                _set = _value_min;
                throw std::string("value to low, min value " + Util::to_string<T>(_value_min));
            } if (value_for_type > _value_max)  {
                _set = _value_max;
                throw std::string("value to high, max value " + Util::to_string<T>(_value_max));
            }

            _set = value_for_type;
        }
    }

private:
    T &_set; /**< Reference to store parsed value in. */
    const T _default; /**< Default value. */
    const T _value_min; /**< Minimum value. */
    const T _value_max; /**< Maximum value. */
};

//! @brief CfgParser Key boolean value parser.
class CfgParserKeyBool : public CfgParserKey {
public:
    //! @brief CfgParserKeyBool constructor.
    CfgParserKeyBool(const char *name,
                     bool &set, const bool default_val = false)
        : CfgParserKey(name),
          _set(set), _default(default_val)
    {
        _type = KEY_BOOL;
    }
    //! @brief CfgParserKeyBool destructor.
    virtual ~CfgParserKeyBool(void) { }

    virtual void parse_value(const std::string &value) throw (std::string&);

private:
    bool &_set; //! Reference to stored parsed value in.
    const bool _default; //! Default value.
};

//! @brief CfgParser Key string value parser.
class CfgParserKeyString : public CfgParserKey {
public:
    //! @brief CfgParserKeyString constructor.
    CfgParserKeyString(const char *name,
                       std::string &set, const std::string default_val = "",
                       const int length_min = std::numeric_limits<int>::min(),
                       const int length_max = std::numeric_limits<int>::max())
        : CfgParserKey(name),
          _set(set), _default(default_val),
          _length_min(length_min), _length_max(length_max)
    {
        _type = KEY_STRING;
    }
    //! @brief CfgParserKeyString destructor.
    virtual ~CfgParserKeyString(void) { }

    virtual void parse_value(const std::string &value) throw (std::string&);

private:
    std::string &_set; //!< Reference to store parsed value in.
    const std::string _default; //!< Default value.
    const int _length_min; //!< Minimum lenght of string.
    const int _length_max; //!< Maximum length of string.
};

//! @brief CfgParser Key path parser.
class CfgParserKeyPath : public CfgParserKey {
public:
    //! @brief CfgParserKeyPath constructor.
    CfgParserKeyPath(const char *name,
                     std::string &set, const std::string default_val = "")
        : CfgParserKey(name),
          _set(set), _default(default_val)
    {
        _type = KEY_PATH;
	Util::expandFileName(_default);
    }
    //! @brief CfgParserKeyPath destructor.
    virtual ~CfgParserKeyPath(void) { }

    virtual void parse_value(const std::string &value) throw (std::string&);

private:
    std::string &_set; //!< Reference to store parsed value in.
    std::string _default; //!< Default value.
};

#endif // _CFG_PARSER_KEY_HH_
