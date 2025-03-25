//
// String.cc for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "String.hh"
#include "Types.hh"

extern "C" {
#include <string.h>
}

/**
 * Return true if str starts with start, else false.
 */
bool
pekwm::str_starts_with(const std::string& str, char start,
		       std::string::size_type pos)
{
	if (pos < str.size()) {
		return str[pos] == start;
	}
	return false;
}

/**
 * Return true if str contains with start at pos, else false.
 */
bool
pekwm::str_starts_with(const std::string& str, const std::string& start,
		       std::string::size_type pos)
{
	if (str.empty()
	    || start.empty()
	    || str.size() < (pos + start.size())) {
		return false;
	}
	return memcmp(str.c_str() + pos, start.c_str(), start.size()) == 0;
}

/**
 * Return true if str ends with end, else false.
 */
bool
pekwm::str_ends_with(const std::string& str, char end)
{
	if (str.empty()) {
		return false;
	}
	return str[str.size()-1] == end;
}

/**
 * Return true if str ends with end, else false.
 */
bool
pekwm::str_ends_with(const std::string& str, const std::string& end)
{
	if (str.empty() || end.empty() || str.size() < end.size()) {
		return false;
	}
	if (end.size() == str.size()) {
		return true;
	}
	return memcmp(str.c_str() + str.size() - end.size(),
		      end.c_str(), end.size()) == 0;
}

/**
 * Compute hash for string.
 */
uint
pekwm::str_hash(const std::string& str)
{
	return str_hash(str.c_str());
}

/**
 * Compute hash for C string.
 */
uint
pekwm::str_hash(const char* str)
{
	uint hash = 0;
	const uchar *p = reinterpret_cast<const uchar*>(str);
	for (; *p != '\0'; p++) {
	    hash = 31 * hash + *p;
	}
	return hash;
}

/**
 * Return lowercase version of chr, ASCII only.
 */
int
pekwm::ascii_tolower(int chr)
{
	if (chr >= 'A' && chr <= 'Z') {
		return chr + 32;
	}
	return chr;
}

int
pekwm::ascii_ncase_cmp(const std::string &lhs, const std::string &rhs)
{
	return ascii_ncase_cmp(lhs.c_str(), rhs.c_str());
}

int
pekwm::ascii_ncase_cmp(const std::string &lhs, const char *rhs)
{
	return ascii_ncase_cmp(lhs.c_str(), rhs);
}

int
pekwm::ascii_ncase_cmp(const char *lhs, const std::string &rhs)
{
	return ascii_ncase_cmp(lhs, rhs.c_str());
}

/**
 * Compare two strings ignoring case, ASCII only.
 */
int
pekwm::ascii_ncase_cmp(const char *lhs, const char *rhs)
{
	return ascii_ncase_ncmp(lhs, rhs, -1);
}

int
pekwm::ascii_ncase_ncmp(const std::string &lhs, const std::string &rhs,
				int n)
{
	return ascii_ncase_ncmp(lhs.c_str(), rhs.c_str(), n);
}

int
pekwm::ascii_ncase_ncmp(const std::string &lhs, const char *rhs, int n)
{
	return ascii_ncase_ncmp(lhs.c_str(), rhs, n);
}

int
pekwm::ascii_ncase_ncmp(const char *lhs, const std::string &rhs, int n)
{
	return ascii_ncase_ncmp(lhs, rhs.c_str(), n);
}

int
pekwm::ascii_ncase_ncmp(const char *lhs, const char *rhs, int n)
{
	for (; (n == -1 || n > 0) && *rhs && *lhs; rhs++, lhs++) {
		int diff = ascii_tolower(*lhs) - ascii_tolower(*rhs);
		if (diff != 0) {
			return diff;
		} else if (n != -1) {
			n--;
		}
	}
	if (n == 0) {
		return 0;
	} else if (*lhs == '\0' && *rhs == '\0') {
		return 0;
	} else if (*lhs == '\0') {
		return 0 - *rhs;
	} else {
		return *lhs;
	}
}

bool
pekwm::ascii_ncase_equal(const std::string &lhs, const std::string &rhs)
{
	return ascii_ncase_cmp(lhs, rhs) == 0;
}

bool
pekwm::ascii_ncase_equal(const std::string &lhs, const char *rhs)
{
	return ascii_ncase_cmp(lhs, rhs) == 0;
}

bool
pekwm::ascii_ncase_equal(const char *lhs, const std::string &rhs)
{
	return ascii_ncase_cmp(lhs, rhs) == 0;
}

/**
 * Check if two strings are equal ignoring case, ASCII only.
 */
bool
pekwm::ascii_ncase_equal(const char *lhs, const char *rhs)
{
	return ascii_ncase_cmp(lhs, rhs) == 0;
}

