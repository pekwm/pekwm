//
// Cond.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Cond.hh"
#include "Util.hh"

namespace Cond {

bool
eval_equal(const std::string lhs, const std::string rhs)
{
	try {
		double lhs_val = std::stod(lhs);
		double rhs_val = std::stod(rhs);
		return lhs_val == rhs_val;
	} catch (std::invalid_argument&) {
		return lhs == rhs;
	}
}

bool
eval_not_equal(const std::string lhs, const std::string rhs)
{
	try {
		double lhs_val = std::stod(lhs);
		double rhs_val = std::stod(rhs);
		return lhs_val != rhs_val;
	} catch (std::invalid_argument&) {
		return lhs != rhs;
	}
}

bool
eval_gt(const std::string lhs, const std::string rhs)
{
	try {
		double lhs_val = std::stod(lhs);
		double rhs_val = std::stod(rhs);
		return lhs_val > rhs_val;
	} catch (std::invalid_argument&) {
		return false;
	}
}

bool
eval_lt(const std::string lhs, const std::string rhs)
{
	try {
		double lhs_val = std::stod(lhs);
		double rhs_val = std::stod(rhs);
		return lhs_val < rhs_val;
	} catch (std::invalid_argument&) {
		return false;
	}
}

/**
 * Evaluate condition represented as a string. Empty condition evaluates to
 * true.
 *
 * Supported operators: =, !=, > and <
 *
 * Strings are enclosed in ", other values are treated as numbers except for
 * true and false.
 */
bool
eval(const std::string &cond)
{
	if (cond.empty()) {
		return true;
	}

	std::vector<std::string> parts = StringUtil::shell_split(cond);
	if (parts.size() != 3) {
		return parts.size() == 1 && parts[0] == "true";
	}

	if (parts[1] == "=") {
		return eval_equal(parts[0], parts[2]);
	} else if (parts[1] == "!=") {
		return eval_not_equal(parts[0], parts[2]);
	} else if (parts[1] == ">") {
		return eval_gt(parts[0], parts[2]);
	} else if (parts[1] == "<") {
		return eval_lt(parts[0], parts[2]);
	} else {
		return false;
	}
}

};
