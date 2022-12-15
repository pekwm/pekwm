//
// String.hh for pekwm
// Copyright (C) 2022 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_STRING_HH_
#define _PEKWM_STRING_HH_

#include <string>

namespace pekwm {

	int ascii_tolower(int chr);
	int ascii_ncase_cmp(const std::string &lhs, const std::string &rhs);
	int ascii_ncase_cmp(const std::string &lhs, const char *rhs);
	int ascii_ncase_cmp(const char *lhs, const std::string &rhs);
	int ascii_ncase_cmp(const char *lhs, const char *rhs);
	int ascii_ncase_ncmp(const std::string &lhs, const std::string &rhs,
			     int n);
	int ascii_ncase_ncmp(const std::string &lhs, const char *rhs, int n);
	int ascii_ncase_ncmp(const char *lhs, const std::string &rhs, int n);
	int ascii_ncase_ncmp(const char *lhs, const char *rhs, int n);
	bool ascii_ncase_equal(const std::string &lhs, const std::string &rhs);
	bool ascii_ncase_equal(const std::string &lhs, const char *rhs);
	bool ascii_ncase_equal(const char *lhs, const std::string &rhs);
	bool ascii_ncase_equal(const char *lhs, const char *rhs);

}

#endif // _PEKWM_STRING_HH_
