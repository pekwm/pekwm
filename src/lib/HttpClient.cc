//
// HttpClient.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "config.h"
#include "Compat.hh"
#include "HttpClient.hh"

#ifdef PEKWM_HAVE_CURL

extern "C" {
#include <curl/curl.h>
}

class HttpClientCurl : public HttpClient {
public:
	HttpClientCurl();
	virtual ~HttpClientCurl();

	virtual int GET(const std::string &url, const string_map &headers,
			std::ostream &os);
	virtual int POST(const std::string &url, const string_map &headers,
			 const std::string &body, std::ostream &os);

private:
	void setHeaders(const string_map &headers);
	void setWriteFunction(std::ostream &os);

	CURL *_curl;
};

HttpClientCurl::HttpClientCurl()
	: HttpClient(),
	  _curl(curl_easy_init())
{
	static bool init = false;
	if (!init) {
		CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
		if (res == CURLE_OK) {
			atexit(curl_global_cleanup);
		} else {
			throw std::string("curl_global_init failed");
		}
		init = true;
	}
	_curl = curl_easy_init();
	if (_curl == nullptr) {
		throw std::string("curl_easy_init failed");
	}
}

HttpClientCurl::~HttpClientCurl()
{
	curl_easy_cleanup(_curl);
}

int
HttpClientCurl::GET(const std::string &url, const string_map &headers,
		    std::ostream &os)
{
	_error = "";

	curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
	setHeaders(headers);
	setWriteFunction(os);

	long http_code = 0;
	CURLcode res = curl_easy_perform(_curl);
	if (res == CURLE_OK) {
		curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &http_code);
	} else {
		_error = curl_easy_strerror(res);
	}
	curl_easy_reset(_curl);

	return static_cast<int>(http_code);
}

int
HttpClientCurl::POST(const std::string &url, const string_map &headers,
		     const std::string &body, std::ostream &os)
{
	_error = "";

	curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
	setHeaders(headers);
	setWriteFunction(os);

	long http_code = 0;
	CURLcode res = curl_easy_perform(_curl);
	if (res == CURLE_OK) {
		curl_easy_getinfo(_curl, CURLINFO_RESPONSE_CODE, &http_code);
	} else {
		_error = curl_easy_strerror(res);
	}
	curl_easy_reset(_curl);

	return static_cast<int>(http_code);
}

void
HttpClientCurl::setHeaders(const string_map &headers)
{
	struct curl_slist *sl_headers = nullptr;
	struct curl_slist *sl_tmp;

	string_map::const_iterator it(headers.begin());
	for (; it != headers.end(); ++it) {
		std::string header = it->first + ": " + it->second;
		if (sl_headers == nullptr) {
			sl_tmp = curl_slist_append(sl_headers, header.c_str());
		} else {
			sl_tmp = curl_slist_append(sl_tmp, header.c_str());
		}
	}
	if (sl_headers) {
		curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, sl_headers);
		curl_slist_free_all(sl_headers);
	}
}

size_t
_write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	std::ostream *os = static_cast<std::ostream*>(userdata);
	size_t n = size * nmemb;;
	os->write(ptr, size * nmemb);
	return n;
}

void
HttpClientCurl::setWriteFunction(std::ostream &os)
{
	curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &os);
	curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, _write_callback);
}

HttpClient*
mkHttpClient()
{
	return new HttpClientCurl();
}

#else // ! PEKWM_HAVE_CURL

HttpClient*
mkHttpClient()
{
	return new HttpClient();
}

#endif // PEKWM_HAVE_CURL
