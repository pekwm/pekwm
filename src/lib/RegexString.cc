//
// RegexString.cc for pekwm
// Copyright (C) 2003-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"

#include <iostream>

#include "Charset.hh"
#include "Debug.hh"
#include "RegexString.hh"
#include "Util.hh"

extern "C" {
#include <stdlib.h>
}

const char RegexString::SEPARATOR = '/';

RegexString::RegexString(void)
	: _reg_ok(false),
	  _reg_inverted(false),
	  _ref_max(1)
{
}

/**
 * RegexString constructor with default search
 */
RegexString::RegexString(const std::string &str, bool full)
	: _reg_ok(false),
	  _reg_inverted(false),
	  _ref_max(1)
{
	parse_match(str, full);
}

RegexString::~RegexString(void)
{
	free_regex();
}

/**
 * Simple ed s command lookalike.
 */
bool
RegexString::ed_s(std::string &str)
{
	if (! _reg_ok) {
		return false;
	}

	std::string mb_str = Charset::toSystem(str);

	const char *c_str = mb_str.c_str();
	regmatch_t *matches = new regmatch_t[_ref_max];

	// Match
	if (regexec(&_regex, mb_str.c_str(), _ref_max, matches, 0)) {
		delete[] matches;
		return false;
	}

	std::string result;
	uint ref, size;

	std::vector<RegexString::Part>::iterator it = _refs.begin();
	for (; it != _refs.end(); ++it) {
		if (it->get_reference() >= 0) {
			ref = it->get_reference();

			if (matches[ref].rm_so != -1) {
				size = matches[ref].rm_eo - matches[ref].rm_so;
				result.append(c_str + matches[ref].rm_so, size);
			}
		} else {
			result.append(Charset::toSystem(it->get_string()));
		}
	}

	// Replace area regexp matched.
	size = matches[0].rm_eo - matches[0].rm_so;
	mb_str.replace(matches[0].rm_so, size, result);
	delete[] matches;

	str = Charset::fromSystem(mb_str);

	return true;
}

//! @brief Parses match part of regular expression.
//! @param match Expression.
//! @param full Full expression if true (including flags). Defaults to false.
bool
RegexString::parse_match(const std::string &match, bool full)
{
	// Free resources
	if (_reg_ok) {
		free_regex();
	}
	if (! match.size()) {
		_reg_ok = false;
		return false;
	}

	int flags = REG_EXTENDED;
	std::string expression;

	// Full regular expression syntax, parse out flags etc
	std::string::size_type pos;
	if (match[0] == SEPARATOR
	    && (pos = match.find_last_of(SEPARATOR)) != std::string::npos) {
		// Main expression
		std::string expression_str = match.substr(1, pos - 1);

		// Expression flags
		for (std::string::size_type i = pos + 1;
		     i < match.size();
		     ++i) {
			switch (match[i]) {
			case 'i':
				flags |= REG_ICASE;
				break;
			case '!':
				_reg_inverted = true;
				break;
			default:
				USER_WARN("invalid flag \"" << match[i]
					  << "\" for regular expression");
				break;
			}
		}

		expression = Charset::toSystem(expression_str);
	} else {
		if (full) {
			USER_WARN("invalid format of regular expression, "
				  << "missing separator " << SEPARATOR);
		}
		expression = Charset::toSystem(match);
	}

	_reg_ok = ! regcomp(&_regex, expression.c_str(), flags);
	_pattern = match;

	return _reg_ok;
}

//! @brief Parses replace part of ed_s command.
//! Expects input in the style of /replace/me/. / can be any character
//! except \. References to sub expressions are made with \num. \0 Represents
//! the part of the string that matched.
bool
RegexString::parse_replace(const std::string &replace)
{
	_ref_max = 0;

	std::string part;
	std::string::size_type begin = 0, end = 0, last = 0;

	// Go through the string and split at \num points
	while ((end = replace.find_first_of('\\', begin))
	       != std::string::npos) {
		// Store string between references.
		if (end > last) {
			part = replace.substr(last, end - last);
			_refs.push_back(RegexString::Part(part));
		}

		// Get reference number.
		for (begin = ++end; isdigit(replace[end]); end++)
			;

		if (end > begin) {
			// Convert number and add item.
			part = replace.substr(begin, end - last);
			int ref;
			try {
				ref = std::stoi(part);
			} catch (std::invalid_argument&) {
				ref = -1;
			}
			if (ref >= 0) {
				_refs.push_back(RegexString::Part("", ref));
				if (ref > _ref_max) {
					_ref_max = ref;
				}
			}
		}

		last = end;
		begin = last + 1;
	}

	if (begin < replace.size()) {
		part = replace.substr(begin, replace.size() - begin);
		_refs.push_back(RegexString::Part(part));
	}

	_ref_max++;

	return true;
}

//! @brief Parses ed s style command. /from/to/
bool
RegexString::parse_ed_s(const std::string &ed_s)
{
	if (ed_s.size() < 3) {
		return false;
	}

	char c_delimeter = ed_s[0];
	std::string::size_type middle, end;

	// Middle.
	for (middle = 1; middle < ed_s.size(); middle++) {
		if ((ed_s[middle] == c_delimeter)
		    && (ed_s[middle - 1] != '\\')) {
			break;
		}
	}

	// End.
	for (end = middle + 1; end < ed_s.size(); end++) {
		if ((ed_s[end] == c_delimeter) && (ed_s[end - 1] != '\\')) {
			break;
		}
	}

	std::string match, replace;
	match = ed_s.substr(1, middle - 1);
	replace = ed_s.substr(middle + 1, end - middle - 1);

	parse_match(match);
	parse_replace(replace);

	return true;
}

//! @brief Matches RegexString against rhs, needs successfull parse_match.
bool
RegexString::operator==(const std::string &rhs) const
{
	if (! _reg_ok) {
		return false;
	}

	std::string mb_rhs = Charset::toSystem(rhs);
	bool match = regexec(&_regex, mb_rhs.c_str(), 0, 0, 0) == 0;

	return _reg_inverted ? ! match : match;
}

//! @brief Free resources used by RegexString.
void
RegexString::free_regex(void)
{
	if (_reg_ok) {
		regfree(&_regex);
		_reg_ok = false;
		_pattern = "";
	}
	_reg_inverted = false;
}
