//
// String.cc for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "String.hh"

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
	int diff;
	for (; (n == -1 || n > 0) && *rhs && *lhs; rhs++, lhs++) {
		diff = ascii_tolower(*lhs) - ascii_tolower(*rhs);
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

