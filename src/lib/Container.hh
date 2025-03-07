//
// Container.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_CONTAINER_HH_
#define _PEKWM_CONTAINER_HH_

#include <vector>

#include "Compat.hh"

namespace Container {

	/**
	 * front() is can break on non-empty container (behaviour is undefined)
	 * helper that always return, potentially nullptr if empty.
	 *
	 * data() is not allways available on old C++98 variants.
	 */
	template<typename T, typename R>
	R type_data(const std::vector<T> &ctr)
	{
		if (ctr.empty()) {
			return nullptr;
		}
		return reinterpret_cast<R>(&ctr.front());
	}

	template<typename T, typename R>
	R type_data(std::vector<T> &ctr)
	{
		if (ctr.empty()) {
			return nullptr;
		}
		return reinterpret_cast<R>(&ctr.front());
	}

};

#endif // _PEKWM_CONTAINER_HH_
