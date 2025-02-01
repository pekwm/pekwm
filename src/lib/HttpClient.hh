//
// HttpClient.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_HTTP_CLIENT_
#define _PEKWM_HTTP_CLIENT_

#include <map>
#include <iostream>
#include <string>

class HttpClient {
public:
	typedef std::map<std::string, std::string> string_map;

	HttpClient() { }
	virtual ~HttpClient() { }

	const std::string &getError() const { return _error; }

	virtual int GET(const std::string &url, const string_map &headers,
			std::ostream &os)
	{
		return 0;
	}

	virtual int POST(const std::string &url, const string_map &headers,
			 const std::string &body, std::ostream &os)
	{
		return 0;
	}

protected:
	std::string _error;
};

HttpClient *mkHttpClient();

#endif // _PEKWM_HTTP_CLIENT_
