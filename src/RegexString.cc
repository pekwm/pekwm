//
// RegexString.cc for pekwm
// Copyright (C) 2003 Claes Nasten <pekdon@gmx.net>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "RegexString.hh"

#ifdef DEBUG
#include <iostream>
using std::cerr;
using std::endl;
#endif // DEBUGE
using std::string;
using std::list;

const char* RegexString::NUMBERS = "1234567890";

//! @fn    RegexString()
//! @brief Constructor for RegexString class
RegexString::RegexString() :
_ref_max(1),
_ok(false), _reg_ok(false), _ref_ok(false)
{
}

//! @fn    RegexString(const string& exp_ref)
//! @brief Constructor for RegexString class
RegexString::RegexString(const string& exp_ref) :
_ref_max(1),
_ok(false), _reg_ok(false), _ref_ok(false)
{
	parse(exp_ref);
}

//! @fn    RegexString(const string& exp, const string& ref)
//! @brief Constructor for RegexString class
RegexString::RegexString(const string& exp, const string& ref) :
_ref_max(1),
_ok(false), _reg_ok(false), _ref_ok(false)
{
	parseReg(exp);
	parseRef(ref);
}

//! @fn    ~RegexString()
//! @brief Destructor for RegexString class
RegexString::~RegexString()
{
	freeReg();
}

//! @fn    bool parse(const string& exp_reg)
//! @brief Parses the exp_reg string
bool
RegexString::parse(const std::string& exp_reg)
{
	_ok = false;

	if (exp_reg.size() < 3) // impossible to have a pattern shorter than 3
		return false;

	char delimeter = exp_reg[0];
	if (exp_reg[exp_reg.size() - 1] != delimeter)
		return false;

	string::size_type pos = exp_reg.find_first_of(delimeter, 1);

	if (pos == string::npos)
		return false;

	parseReg(exp_reg.substr(1, pos - 1));
	parseRef(exp_reg.substr(pos + 1, exp_reg.size() - pos - 2));

	if (_reg_ok && _ref_ok)
		_ok = true;

	return _ok;
}

//! @fn    void parseReg(const std::string& exp)
//! @brief Compiles the regexp exp
void
RegexString::parseReg(const std::string& exp)
{
	if (_reg_ok)
		freeReg();
	_reg_ok = !regcomp(&_regex, (const char*) exp.c_str(), REG_EXTENDED);
}

//! @fn    void parseRef(const string& ref);
//! @brief Parse a reference string ( My: $1 )
void
RegexString::parseRef(const string& ref)
{
	// reset values
	_ref_ok = false;
	_ref_list.clear();

	string buf(ref);
	unsigned int num, length;
	string::size_type beg = 0, last = 0, end;

	while ((end = buf.find_first_of("\\$", beg)) != string::npos) {
		if (buf[end] == '$') { // reference
			if (last < end) // store string between
				_ref_list.push_back(RegListItem(buf.substr(last, end - last), -1));

			// get the number
			beg = buf.find_first_not_of(NUMBERS, end + 1);
			length = (beg == string::npos) ? buf.size() - end - 1 : beg - end - 1;
			num = atoi(buf.substr(end + 1, length).c_str());

			_ref_list.push_back(RegListItem("", num));
			
			last = beg;
		} else {
			buf.erase(end); // remove excape
			beg++; // skip the character after
		}
	}

	// get the end
	if (beg != string::npos)
		_ref_list.push_back(RegListItem(buf.substr(beg, buf.size() - beg), -1));

	// calc max number of refs
	list<RegListItem>::iterator it = _ref_list.begin();
	for (_ref_max = 0; it != _ref_list.end(); ++it) {
		if ((it->getRef() != -1) && (it->getRef() > signed(_ref_max)))
			_ref_max = it->getRef();
	}

	// make sure it's more than 0 and we need an extra for the whole match
	++_ref_max;

	_ref_ok = true;
}

//! @fn    void freeReg(void)
//! @brief Frees resources used by compiled regex
void
RegexString::freeReg(void)
{
	if (_reg_ok)
		regfree(&_regex);
	_reg_ok = false;
}

//! @fn    bool replace(string& rhs)
//! @brief 
bool
RegexString::replace(string& rhs)
{
	if (!_ok)
		return false;

	const char *data = rhs.c_str();
	string result;

	unsigned int ref, length;
	regmatch_t matches[_ref_max];

	if (regexec(&_regex, data, _ref_max, matches, 0) == 0) {
		list<RegListItem>::iterator it = _ref_list.begin();
		for (; it != _ref_list.end(); ++it) {
			if (it->getRef() != -1) { // this item is a reference
				ref = it->getRef();

				if (matches[ref].rm_so != -1) {
					length = matches[ref].rm_eo - matches[ref].rm_so;
					result.append(string(data + matches[ref].rm_so, length));
				}
			} else { // this item is a string, just append it
				result.append(it->getString());
			}
		}
	}

	if (rhs != result)
		rhs = result;

	return true;
}
