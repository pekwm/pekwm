//
// Charset.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CHARSET_HH_
#define _PEKWM_CHARSET_HH_

#include "Compat.hh"
#include "String.hh"

extern "C" {
#ifdef PEKWM_HAVE_STDINT_H
#include <stdint.h>
#endif // PEKWM_HAVE_STDINT_H
}

namespace Charset
{
	class WithCharset
	{
	public:
		WithCharset(void);
		~WithCharset(void);
	};

	/**
	 * Class for iterating a std::string one UTF-8 character at a time.
	 */
	class Utf8Iterator
	{
	public:
		Utf8Iterator(const StringView& str);
		~Utf8Iterator();

		/**
		 * Seek to logical character.
		 */
		void seek(size_t pos) {
			_pos = 0;
			for (; ok() && pos > 0; --pos) {
				++(*this);
			}
		}

		/**
		 * Hard set of position, does not take multi-byte sequences
		 * into account.
		 */
		void setPos(size_t pos) {
			_pos = pos > _str.size() ? _str.size() : pos;
		}

		bool begin() const { return _begin; }
		bool end() const { return _pos == _str.size(); }
		bool ok() const { return ! end(); }

		size_t pos() const { return _pos; }
		const char* str() const { return ok() ? *_str + _pos : ""; }

		bool operator==(char chr);
		bool operator==(const char* chr);
		bool operator==(const std::string& chr);

		const char* operator*();
		size_t charLen() const;

		Utf8Iterator &operator++();
		Utf8Iterator &operator--();

	private:
		bool decPos(void);
		bool incPos(void);
		size_t len(size_t pos) const;

		bool _begin;
		StringView _str;
		size_t _pos;

		char _deref_buf[7];
	};

	void init(void);
	void destruct(void);

	bool isUtf8Locale(void);

	std::string toSystem(const StringView &str);
	std::string fromSystem(const StringView &str);

	void toUtf8(uint32_t chr, std::string &str);
}

#endif // _PEKWM_CHARSET_HH_
