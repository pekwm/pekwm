//
// Tokenizer.cc for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "String.hh"
#include "Tokenizer.hh"

Tokenizer::Tokenizer(const std::string& str)
	: _str(str),
	  _pos(str.empty() ? std::string::npos : 0)
{
}

bool
Tokenizer::next()
{
	if (_pos == std::string::npos) {
		return false;
	} else if (pekwm::str_starts_with(_str, ' ', _pos)
		   || pekwm::str_starts_with(_str, '\n', _pos)
		   || pekwm::str_starts_with(_str, '\t', _pos)) {
		_token = "";
		_token += _str[_pos];
		setPos(_pos + 1);
		return true;
	} else if (pekwm::str_starts_with(_str, "\\n", _pos)) {
		_token = "\n";
		setPos(_pos + 2);
		return true;
	}

	bool done = false;
	std::string::size_type pos = _pos;
	while (! done) {
		done = true;
		pos = _str.find_first_of(" \t\n\\", pos);
		if (pos == std::string::npos) {
			_token = _str.substr(_pos, pos - _pos);
			setPos(pos);
			continue;
		}

		switch (_str[pos]) {
		case ' ':
		case '\t':
		case '\n':
			_token = _str.substr(_pos, pos - _pos);
			setPos(pos);
			break;
		case '\\':
			if (pekwm::str_starts_with(_str, "\\n", pos)) {
				_token = _str.substr(_pos, pos - _pos);
				setPos(pos);
			} else {
				done = false;
			}
		}
	}
	return true;
}

bool
Tokenizer::isBreak()
{
	return _token == "\n";
}

const std::string&
Tokenizer::operator*() const
{
	return _token;
}

/**
 * Set position, if at the end replace with npos
 */
void
Tokenizer::setPos(std::string::size_type pos)
{
	if (pos < _str.size()) {
		_pos = pos;
	} else {
		_pos = std::string::npos;
	}
}
