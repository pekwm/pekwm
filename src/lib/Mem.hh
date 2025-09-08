//
// Mem.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_MEM_HH_
#define _PEKWM_MEM_HH_

#include "Compat.hh"

extern "C" {
#include <stddef.h>
#include <string.h>
}

template<typename T>
class Destruct {
public:
	typedef void (*free_fun)(T*);

	Destruct(T *ptr, bool array=false)
		: _ptr(ptr),
		  _array(array),
		  _free_fun(nullptr)
	{
	}

	Destruct(T *ptr, bool array, free_fun free_fun)
		: _ptr(ptr),
		  _array(array),
		  _free_fun(free_fun)
	{
	}

	~Destruct()
	{
		destruct();
	}

	/**
	 * Return referenced data and drop the reference, caller is
	 * responsible for calling delete on the returned data.
	 */
	T *take()
	{
		T *ptr = _ptr;
		_ptr = nullptr;
		return ptr;
	}

	/**
	 * Immediate destruct of referenced data (if any).
	 */
	void destruct()
	{
		if (_ptr != nullptr) {
			if (_free_fun) {
				_free_fun(_ptr);
			} else if (_array) {
				delete[] _ptr;
			} else {
				delete _ptr;
			}
			_ptr = nullptr;
		}
	}

	T *operator*() const { return _ptr; }
	T *operator->() const { return _ptr; }

private:
	Destruct(const Destruct&);
	Destruct& operator=(const Destruct&);

	T *_ptr;
	bool _array;
	free_fun _free_fun;
};

template<typename T>
class Buf {
public:
	Buf(size_t size)
		: _size(size),
		  _data(new T[size])
	{
	}
	~Buf()
	{
		delete [] _data;
	}

	/**
	 * Grow buffer, optionally preserving the previous content.
	 */
	void grow(bool preserve)
	{
		size_t new_size = _size * 2;
		T *new_data = new T[new_size];
		if (preserve) {
			memcpy(new_data, _data, _size);
		}
		delete [] _data;

		_size = new_size;
		_data = new_data;
	}

	/**
	 * Ensure buffer fits size elements.
	 */
	void ensure(size_t size, bool preserve)
	{
		while (_size < size) {
			grow(preserve);
		}
	}

	T *operator*() { return _data; }
	size_t size() const { return _size; }

private:
	Buf(const Buf&);
	Buf& operator=(const Buf&);

	size_t _size;
	T *_data;
};

#endif // _PEKWM_MEM_HH_
