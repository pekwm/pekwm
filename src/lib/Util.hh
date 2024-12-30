//
// Util.hh for pekwm
// Copyright (C) 2002-2024 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_UTIL_HH_
#define _PEKWM_UTIL_HH_

#include "config.h"

#include "Compat.hh"
#include "Types.hh"
#include "String.hh"

#include <algorithm>
#include <string>
#include <cstring>
#include <map>
#include <functional>
#include <sstream>
#include <vector>

extern "C" {
#include <string.h>
}

/**
 * String utilities, convenience functions for making life easier
 * when working with strings.
 */
namespace StringUtil {
	class Key {
	public:
		Key(const char *key);
		Key(const std::string &key);
		~Key(void);

		const std::string& str(void) const { return _key; }

		bool operator==(const std::string &rhs) const;
		bool operator!=(const std::string &rhs) const;
		bool operator<(const Key &rhs) const;
		bool operator>(const Key &rhs) const;

	private:
		std::string _key;
	};

	size_t safe_position(size_t pos, size_t fallback = 0, size_t add = 0);
	std::vector<std::string> shell_split(const std::string& str);
}

namespace Generator {

	/**
	 * Generate a sequence of numbers from start to num, wrapping at num
	 * if start is > 0.
	 *
	 * For start=3, max=5 the sequence: 3, 4, 0, 1, 2 is produced.
	 */
	template<typename T>
	class RangeWrap {
	public:
		RangeWrap(T start, T max, T step=1)
			: _start(start),
			  _max(max),
			  _step(step)
		{
			reset();
		}

		/**
		 * Reset generator, allowing for a full set of values to be
		 * returned.
		 */
		void reset(void)
		{
			_curr = _start;
			_wrapped = false;
		}

		/** Return true if the end has been reached */
		bool is_end(void) const
		{
			if (_start == 0) {
				return _curr >= _max;
			}
			return _wrapped && _curr >= _start;
		}

		/** Return current value. */
		T operator*(void) const { return _curr; }

		/**
		 * Move generator to next value, unless end has been reached
		 * and nothing is done.
		 */
		RangeWrap<T>& operator++(void)
		{
			if (is_end()) {
				return *this;
			}

			_curr += _step;
			if (_curr >= _max) {
				if (_start != 0 && ! _wrapped) {
					_curr -= _max;
					_wrapped = true;
				}
			}
			return *this;
		}

	private:
		T _start; /**< start value for generator */
		T _max; /** < max value (exclusive) */
		T _step; /** increment with step for each round. */
		T _curr; /**< current value */
		bool _wrapped; /**< set to true if end has been reached */
	};
}

namespace Util {
	template<typename T>
	class StringMap : public std::map<StringUtil::Key, T> {
	public:
		StringMap(void) { }
		virtual ~StringMap(void) { }

		T& get(const std::string& key) {
			typename StringMap<T>::iterator it = this->find(key);
			if (it == this->end()) {
				return this->operator[]("");
			}
			return it->second;
		}
	};

	std::string getEnv(const std::string& key);
	void setEnv(const std::string& key, const std::string &value);
	std::string getConfigDir(void);

	void forkExec(const std::string& command);
	std::string getHostname(void);
	bool setNonBlock(int fd);

	bool isFile(const std::string &file);
	bool isExecutable(const std::string &file);
	time_t getMtime(const std::string &file);

	bool copyTextFile(const std::string &from, const std::string &to);

	std::string getFileExt(const std::string &file);
	std::string getDir(const std::string &file);
	void expandFileName(std::string &file);

	const char* spaceChars(char escape);

	uint splitString(const std::string &str,
			 std::vector<std::string> &toks,
			 const char *sep, uint max = 0,
			 bool include_empty = false, char escape = 0);

	std::string to_string(void* v);
	void to_upper(std::string &str);
	void to_lower(std::string &str);

	/**
	 * Return value within bounds of min and max value.
	 */
	template<typename T>
	T between(T value, T min_val, T max_val) {
		if (value < min_val) {
			value = min_val;
		} else if (value > max_val) {
			value = max_val;
		}
		return value;
	}

	//! @brief Removes leading blanks( \n\t) from string.
	inline void trimLeadingBlanks(std::string &trim) {
		std::string::size_type first = trim.find_first_not_of(" \n\t");
		if ((first != std::string::npos) &&
		    (first != (std::string::size_type) trim[0])) {
			trim = trim.substr(first, trim.size() - first);
		}
	}

	bool isTrue(const std::string &value);

	//! @brief for_each delete utility.
	template<class T> struct Free : public std::unary_function<T, void> {
		void operator ()(T t) { delete t; }
	};

	template<typename T>
	struct StringTo {
		const char *name;
		T value;
	};

	template<typename T>
	T StringToGet(const Util::StringTo<T> *map, const std::string& key)
	{
		int i = 0;
		for (; map[i].name != nullptr; i++) {
			if (pekwm::ascii_ncase_equal(map[i].name, key)) {
				return map[i].value;
			}
		}
		return map[i].value;
	}

}

/**
 * OS environment wrapper, supports getting environments and providing
 * environment arrays with overriden values.
 */
class OsEnv {
public:
	OsEnv();
	~OsEnv();

	void override(const std::string& key, const std::string& value);

	const std::map<std::string, std::string>& getEnv();
	char** getCEnv();

private:
	OsEnv(const OsEnv&);
	OsEnv& operator=(const OsEnv&);

	char** toCEnv(const std::map<std::string, std::string> &env);
	void freeCEnv();

	void setEnvVar(std::map<std::string, std::string> &env,
		       const char *envp);

private:
	bool _dirty;
	std::map<std::string, std::string> _env;
	std::map<std::string, std::string> _env_override;
	char **_c_env;
};

#endif // _PEKWM_UTIL_HH_
