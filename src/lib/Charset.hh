//
// Charset.cc for pekwm
// Copyright (C) 2021 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CHARSET_HH_
#define _PEKWM_CHARSET_HH_

#include "Compat.hh"
#include <string>

namespace Charset
{
	class WithCharset
	{
	public:
		WithCharset(void);
		~WithCharset(void);
	};

	class Utf8Iterator
	{
	public:
		Utf8Iterator(const std::string& str, size_t pos);

		bool begin(void) const { return _begin; }
		bool end(void) const { return _pos == _str.size(); }
		bool ok(void) const { return ! end(); }

		size_t pos(void) const { return _pos; }
		const char* str(void) const {
			return ok() ? _str.c_str() + _pos : "";
		}

		bool operator==(char chr) const;
		bool operator==(const char* chr) const;
		bool operator==(const std::string& chr) const;

		const char* operator*(void);

		Utf8Iterator& operator--(void);
		Utf8Iterator& operator++(void);

	private:
		size_t len(size_t pos) const;
		bool decPos(void);
		bool incPos(void);

	private:
		bool _begin;
		const std::string &_str;
		size_t _pos;

		char _deref_buf[7];
	};

	void init(void);
	void destruct(void);

	bool isUtf8Locale(void);

	std::string toSystem(const std::string &str);
	std::string fromSystem(const std::string &str);
}

#endif // _PEKWM_CHARSET_HH_
