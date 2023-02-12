//
// Tokenizer.hh for pekwm
// Copyright (C) 2023 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_TOKENIZER_HH_
#define _PEKWM_TOKENIZER_HH_

#include <string>

class Tokenizer {
public:
	Tokenizer(const std::string& str);

	bool next();
	bool isBreak();
	const std::string& operator*() const;

private:
	void setPos(std::string::size_type pos);

private:
	std::string _str;
	std::string _token;
	std::string::size_type _pos;
};

#endif // _PEKWM_TOKENIZER_HH_
