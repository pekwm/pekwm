//
// String.hh for pekwm
// Copyright (C) 2022-2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_STRING_HH_
#define _PEKWM_STRING_HH_

#include "Compat.hh"

#include <iostream>
#include <string>
extern "C" {
#include <string.h>
}

class StringView {
public:
	StringView(const char *data)
		: _data(data ? data : ""),
		  _size(data ? strlen(data) : 0)
	{
	}

	StringView(const char *data, size_t size)
		: _data(data ? data : ""),
		  _size(data ? size : 0)
	{
	}

	StringView(const char *data, size_t size, size_t off)
	{
		if (data == nullptr) {
			data = "";
		}
		init(data, strlen(data), size, off);
	}


	StringView(const std::string &str)
		: _data(str.c_str()),
		  _size(str.size())
	{
	}

	StringView(const std::string &str, size_t size)
		: _data(str.c_str()),
		  _size((str.size() < size) ? str.size() : size)
	{
	}

	StringView(const std::string &str, size_t size, size_t off)
	{
		init(str.c_str(), str.size(), size, off);
	}

	StringView(const StringView &str, size_t size)
		: _data(*str),
		  _size((str.size() < size) ? str.size() : size)
	{
	}

	StringView(const StringView &str, size_t size, size_t off)
	{
		init(*str, str.size(), size, off);
	}

	virtual ~StringView() { }

	bool empty() const { return _size == 0; }
	std::string str() const { return std::string(_data, _size); }

	const char* operator*() const { return _data; }
	char operator[](size_t pos) const {
		if (_data && pos < _size) {
			return _data[pos];
		}
		return static_cast<char>(0);
	}
	size_t size() const { return _size; }

protected:
	void init(const char *data, size_t data_size, size_t size, size_t off)
	{
		if (off < data_size) {
			_data = data + off;
			if (size == 0) {
				_size = data_size - off;
			} else if ((off + size) > data_size) {
				_size = data_size - off;
			} else {
				_size = size;
			}
		} else {
			_data = "";
			_size = 0;
		}

	}

private:
	const char *_data;
	size_t _size;
};

namespace pekwm {

	bool str_starts_with(const std::string& str, char start,
			     std::string::size_type pos=0);
	bool str_starts_with(const std::string& str, const std::string& start,
			     std::string::size_type pos=0);
	bool str_ends_with(const std::string& str, char end);
	bool str_ends_with(const std::string& str, const std::string& end);

	uint str_hash(const std::string& str);
	uint str_hash(const char* str);

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
