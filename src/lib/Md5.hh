//
// Md5.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_MD5_HH_
#define _PEKWM_MD5_HH_

#include "String.hh"

extern "C" {
#include <stdio.h>
}

namespace pekwm
{
	struct md5_context {
		unsigned int a;
		unsigned int b;
		unsigned int c;
		unsigned int d;
		unsigned int count[2];
		unsigned char input[64];
		unsigned int block[16];
	};
}

class Md5 {
public:
	Md5();
	Md5(const std::string &data);
	Md5(const StringView &data);

	std::string hexDigest() const {
		char hex[33];
		for (size_t i = 0; i < sizeof(_digest); i++) {
			sprintf(&hex[i*2], "%02x", _digest[i]);
		}
		hex[32] = '\0';
		return hex;
	}

	void update(const std::string &data, bool do_finalze=false);
	void finalize();

private:
	void oneGo(const char *data, size_t len);

	pekwm::md5_context _ctx;
	uint8_t _digest[16];
};

#endif // _PEKWM_MD5_HH_
