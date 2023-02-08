//
// Copyright (C) 2013-2023 Claes Nästén
// Copyright (C) 2005-2013 the pekwm development team
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CFGPARSERKEY_HH_
#define _PEKWM_CFGPARSERKEY_HH_

#include "Compat.hh"
#include "Util.hh"

#include <algorithm>
#include <string>
#include <cstdlib>

#ifdef PEKWM_HAVE_LIMITS
#include <limits>
#endif // PEKWM_HAVE_LIMITS

//! @brief CfgParserKey base class.
class CfgParserKey {
public:
	//! @brief CfgParserKey constructor.
	CfgParserKey(const char *name) : _name(name) { }
	//! @brief CfgParserKey destructor.
	virtual ~CfgParserKey(void) { }

	//! @brief Returns Key name.
	const char *getName(void) const { return _name; }

	//! @brief Parses value and sets Key value.
	virtual void parseValue(const std::string &) { }

protected:
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
	parseValue(const std::string &value_str)
	{
		double value;
		char *endptr;

		// Get long value.
		value = strtod(value_str.c_str(), &endptr);

		// Check for validity, 0 is returned on failiure with endptr
		// set to the beginning of the string, else we are (semi) ok.
		if ((value == 0) && (endptr == value_str.c_str())) {
			_set = _default;

		} else {
			T value_for_type = static_cast<T>(value);

			if (value_for_type < _value_min) {
				_set = _value_min;
				throw std::string("value to low, min value "
						  + std::to_string(_value_min));
			} if (value_for_type > _value_max)  {
				_set = _value_max;
				throw std::string("value to high, max value "
						  + std::to_string(_value_max));
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
	CfgParserKeyBool(const char *name,
			 bool &set, const bool default_val = false);
	virtual ~CfgParserKeyBool(void);

	virtual void parseValue(const std::string &value);

private:
	bool &_set; //! Reference to stored parsed value in.
	const bool _default; //! Default value.
};

//! @brief CfgParser Key string value parser.
class CfgParserKeyString : public CfgParserKey {
public:
	CfgParserKeyString(const char *name,
			   std::string &set,
			   const std::string& default_val = "",
			   const std::string::size_type length_min = 0);
	virtual ~CfgParserKeyString(void);

	virtual void parseValue(const std::string &value);

private:
	std::string &_set; //!< Reference to store parsed value in.
	const std::string::size_type _length_min; //!< Minimum length of string.
};

//! @brief CfgParser Key path parser.
class CfgParserKeyPath : public CfgParserKey {
public:
	//! @brief CfgParserKeyPath constructor.
	CfgParserKeyPath(const char *name,
			 std::string &set,
			 const std::string &default_val = "");
	virtual ~CfgParserKeyPath(void);

	virtual void parseValue(const std::string &value);

private:
	std::string &_set; //!< Reference to store parsed value in.
	std::string _default; //!< Default value.
};

/**
 * Convenience vector implementaiton for cfg parser keys, cares of memory
 * management and makes constructions more compact.
 */
class CfgParserKeys : public std::vector<CfgParserKey*>
{
public:
	CfgParserKeys(void) { }
	virtual ~CfgParserKeys(void)
	{
		std::for_each(begin(), end(), Util::Free<CfgParserKey*>());
	}

	void add_bool(const char *key, bool &value,
		      const bool default_val = false)
	{
		push_back(new CfgParserKeyBool(key, value, default_val));
	}

	template<typename T>
	void add_numeric(const char *name, T &set, const T default_val = 0,
			 const T value_min = std::numeric_limits<T>::min(),
			 const T value_max = std::numeric_limits<T>::max())
	{
		push_back(new CfgParserKeyNumeric<T>(name, set, default_val,
						     value_min, value_max));
	}

	void add_path(const char *key, std::string &value,
		      const std::string& default_val = "")
	{
		push_back(new CfgParserKeyPath(key, value, default_val));
	}

	void add_string(const char *key, std::string &value,
			const std::string& default_val = "",
			const std::string::size_type length_min = 0)
	{
		push_back(new CfgParserKeyString(key, value, default_val,
						 length_min));
	}

	virtual void clear(void)
	{
		std::for_each(begin(), end(), Util::Free<CfgParserKey*>());
		std::vector<CfgParserKey*>::clear();
	}
};

#endif // _PEKWM_CFGPARSERKEY_HH_
