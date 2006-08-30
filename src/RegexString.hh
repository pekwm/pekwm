//
// RegexString.hh for pekwm
// Copyright (C) 2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "../config.h"

#ifndef _REGEX_STRING_HH_
#define _REGEX_STRING_HH_
 
#include <string>
#include <list>

extern "C" {
#include <sys/types.h>
#ifdef PCRE
#include <pcreposix.h>
#else // !PCRE
#include <regex.h>
#endif // PCRE
}

class RegexString
{
public:
	class RegListItem
	{
	public:
		RegListItem(const std::string& nstring, int ref) :
			_string(nstring), _ref(ref) { }
		~RegListItem() { }

		const std::string& getString(void) { return _string; }
		int getRef(void) { return _ref; }

	private:
		std::string _string;
		int _ref;
	};

	RegexString();
	RegexString(const std::string& exp_ref);
	RegexString(const std::string& exp, const std::string& ref);
	~RegexString();

	inline bool isOk(void) const { return _ok; }
	inline bool isRegOk(void) const { return _reg_ok; }
	inline bool isRefOk(void) const { return _ref_ok; }

	bool parse(const std::string& exp_ref);
	bool replace(std::string& rhs);

	inline RegexString& operator = (const std::string& rhs) {
		parseReg(rhs);
		return *this;
	}
	inline bool operator == (const std::string& rhs) {
		if (_reg_ok)
			return !regexec(&_regex, (const char*) rhs.c_str(), 0, 0, 0);
		return false;
	}

private:
	void parseReg(const std::string& exp);
	void parseRef(const std::string& ref);

	void freeReg(void);

private:
	regex_t _regex;

	unsigned int _ref_max;
	std::list<RegListItem> _ref_list;

	bool _ok, _reg_ok, _ref_ok;

	static const char* NUMBERS;
};

#endif // _REGEX_STRING_HH_
