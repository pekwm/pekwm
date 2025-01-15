//
// Charset.cc for pekwm
// Copyright (C) 2021-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "Charset.hh"
#include "Types.hh"
#include "Debug.hh"

#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <stdexcept>

#ifdef PEKWM_HAVE_LOCALE
#include <locale>
#else // ! PEKWM_HAVE_LOCALE
extern "C" {
#include <locale.h>
}
#endif // PEKWM_HAVE_LOCALE

// Lookup table from character value to number of bytes
static const uint8_t UTF8_BYTES[256] = {
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
	2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 1, 1
};

static const uint8_t UTF8_MAX_BYTES = 4;

static int _is_utf8_locale = -1;

template<typename T>
static uint8_t
char_to_utf8(T wc, char *utf8)
{
	if (wc < 0x80) {
		utf8[0] = wc;
		return 1;
	}
	if (wc <= 0x7FF) {
		utf8[0] = (wc >> 6) + 0xC0;
		utf8[1] = (wc & 0x3F) + 0x80;
		return 2;
	}
	if (wc <= 0xFFFF) {
		utf8[0] = (wc >> 12) + 0xE0;
		utf8[1] = ((wc >> 6) & 0x3F) + 0x80;
		utf8[2] = (wc & 0x3F) + 0x80;
		return 3;
	}
	if (wc <= 0x10FFFF) {
		utf8[0] = (wc >> 18) + 0xF0;
		utf8[1] = ((wc >> 12) & 0x3F) + 0x80;
		utf8[2] = ((wc >> 6) & 0x3F) + 0x80;
		utf8[3] = (wc & 0x3F) + 0x80;
		return 4;
	}
	// unsupported character
	return 0;
}

template<typename T>
static uint8_t
utf8_to_char(const char *utf8, T &wc)
{
	uint8_t len = UTF8_BYTES[static_cast<uint8_t>(utf8[0])];
	if (len == 1) {
		wc = utf8[0];
	} else if (len == 2) {
		wc = ((utf8[0] & 0x3f) << 6)
			| (utf8[1] & 0x3f);
	} else if (len == 3) {
		wc = ((utf8[0] & 0x3f) << 12)
			| ((utf8[1] & 0x3f) << 6)
			| (utf8[2] & 0x3f);
	} else if (len == 4) {
		wc = ((utf8[0] & 0x3f) << 18)
			| ((utf8[1] & 0x3f) << 12)
			| ((utf8[1] & 0x3f) << 6)
			| (utf8[2] & 0x3f);
	} else {
		// 5 and 6 character sequences are invalid.
		len = 0;
	}
	return len;
}

#ifdef PEKWM_HAVE_LOCALE_COMBINE
class NoGroupingNumpunct : public std::numpunct<char>
{
protected:
	// cppcheck-suppress unusedFunction
	virtual std::string do_grouping(void) const { return ""; }
};
#endif // PEKWM_HAVE_LOCALE_COMBINE

namespace Charset
{
	WithCharset::WithCharset(void)
	{
		init();
	}

	WithCharset::~WithCharset(void)
	{
		destruct();
	}

	Utf8Iterator::Utf8Iterator(const std::string& str, size_t pos)
		: _begin(false),
		  _str(str),
		  _pos(pos > str.size() ? str.size() : pos)
	{
		_deref_buf[0] = '\0';
	}

	Utf8Iterator::~Utf8Iterator()
	{
	}

	bool
	Utf8Iterator::operator==(char chr)
	{
		const char *c = *(*this);
		return c[0] == chr;
	}

	bool
	Utf8Iterator::operator==(const char* chr)
	{
		const char *c = *(*this);
		size_t len = std::min(sizeof(_deref_buf), strlen(chr));
		return strncmp(c, chr, len) == 0;
	}

	bool
	Utf8Iterator::operator==(const std::string& chr)
	{
		const char *c = *(*this);
		return chr == c;
	}

	/**
	 * Get current UTF-8 character, first byte is NIL if string has been
	 * iterated.
	 */
	const char*
	Utf8Iterator::operator*()
	{
		if (ok()) {
			if (_deref_buf[0] == '\0') {
				size_t size = len(_pos);
				memcpy(_deref_buf, _str.c_str() + _pos, size);
				_deref_buf[size] = '\0';
			}
		} else {
			_deref_buf[0] = '\0';
		}
		return _deref_buf;
	}

	Utf8Iterator&
	Utf8Iterator::operator++()
	{
		incPos();
		return *this;
	}

	Utf8Iterator&
	Utf8Iterator::operator--()
	{
		decPos();
		return *this;
	}

	/**
	 * Return length in bytes of the current character, 0 on eof.
	 */
	size_t
	Utf8Iterator::charLen() const
	{
		return ok() ? len(_pos) : 0;
	}

	size_t
	Utf8Iterator::len(size_t pos) const
	{
		if (pos >= _str.size()) {
			return 0;
		}
		return UTF8_BYTES[static_cast<uint8_t>(_str[pos])];
	}

	/**
	 * Increment position, return true if position was incremented.
	 */
	bool
	Utf8Iterator::incPos(void)
	{
		if (_pos < _str.size()) {
			_pos += len(_pos);
			if (_pos >= _str.size()) {
				_pos = _str.size();
				return false;
			}
			_begin = false;
			_deref_buf[0] = '\0';
		}
		return true;
	}

	/**
	 * Decrement position, return true if position was decremented.
	 */
	bool
	Utf8Iterator::decPos(void)
	{
		if (_pos > 5 && len(_pos - 6) == 6) {
			_pos -= 6;
		} else if (_pos > 4 && len(_pos - 5) == 5) {
			_pos -= 5;
		} else if (_pos > 3 && len(_pos - 4) == 4) {
			_pos -= 4;
		} else if (_pos > 2 && len(_pos - 3) == 3) {
			_pos -= 3;
		} else if (_pos > 1 && len(_pos - 2) == 2) {
			_pos -= 2;
		} else if (_pos > 0) {
			_pos -= 1;
		} else {
			_begin = true;
			return false;
		}
		_deref_buf[0] = '\0';
		return true;
	}

	/**
	 * Init charset conversion resources, must be called before
	 * any other call to functions in the Charset namespace.
	 */
	void
	init(void)
	{
#ifdef PEKWM_HAVE_LOCALE
		try {
			// initial global locale setup works around issues on
			// at least FreeBSD where num_locale setup would cause
			// charset conversion to break.
			std::locale base_locale("");
			std::locale::global(base_locale);

#ifdef PEKWM_HAVE_LOCALE_COMBINE
			std::locale num_locale(std::locale(),
					       new NoGroupingNumpunct());
			std::locale locale = std::locale()
				.combine<std::numpunct<char> >(num_locale);
			std::locale::global(locale);
#endif // PEKWM_HAVE_LOCALE_COMBINE
		} catch (const std::runtime_error&) {
			// a user warning used to occur here  but this fails on
			// too many systems, so skipping the warning.
			setlocale(LC_ALL, "");
		}
#else // ! PEKWM_HAVE_LOCALE
		setlocale(LC_ALL, "");
#endif // PEKWM_HAVE_LOCALE
	}

	/**
	 * Called to free static charset conversion resources.
	 */
	void
	destruct(void)
	{
	}

	bool isUtf8Locale(void)
	{
		if (_is_utf8_locale == -1) {
			const char* lc = getenv("LC_CTYPE");
			if (lc == nullptr) {
				lc = getenv("LC_ALL");
			}
			if (lc != nullptr
			    && (strstr(lc, "utf8")
				|| strstr(lc, "UTF8")
				|| strstr(lc, "UTF-8"))) {
				_is_utf8_locale = 1;
			} else {
				_is_utf8_locale = 0;
			}
		}
		return _is_utf8_locale == 1;
	}

	std::string toSystem(const std::string &str)
	{
		if (isUtf8Locale()) {
			return str;
		}

		wchar_t wc = 0;
		char *mb = new char[MB_CUR_MAX + 1];
		std::string str_sys;

		// reset state of wctomb before starting
		int tmp = wctomb(nullptr, 0);
		(void)tmp;

		Utf8Iterator it(str, 0);
		for (; ! it.end(); ++it) {
			utf8_to_char<wchar_t>(*it, wc);
			int len = wctomb(mb, wc);
			if (len > 0) {
				mb[len] = '\0';
				str_sys += mb;
			}
		}

		delete [] mb;

		return str_sys;
	}

	std::string fromSystem(const std::string &str)
	{
		if (isUtf8Locale()) {
			return str;
		}

		wchar_t wc;
		char utf8[UTF8_MAX_BYTES + 1];
		std::string str_utf8;

		mbtowc(&wc, nullptr, 0);

		const char *mb = str.c_str();
		const char *mb_end = str.c_str() + str.size();
		for (int len;
		     (len = mbtowc(&wc, mb, mb_end - mb)) > 0;
		     mb += len) {
			int utf8_len = char_to_utf8<wchar_t>(wc, utf8);
			utf8[utf8_len] = '\0';
			str_utf8 += utf8;
		}

		return str_utf8;
	}

	void toUtf8(uint32_t chr, std::string &str)
	{
		char utf8[UTF8_MAX_BYTES + 1];
		int utf8_len = char_to_utf8<uint32_t>(chr, utf8);
		utf8[utf8_len] = '\0';
		str += utf8;
	}
}
