//
// Location.cc for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include <iostream>

#include "Compat.hh"
#include "Debug.hh"
#include "Json.hh"
#include "Location.hh"

static const char *GEOIP_PW_V2_LOOKUP_SELF =
	"https://geoip.pw/api/v2/lookup/self?pretty=false";

Location::Location(HttpClient *client)
	: _client(client),
	  _looked_up(false),
	  _latitude(0.0),
	  _longitude(0.0)
{
}

Location::~Location()
{
	delete _client;
}

bool
Location::get(double &latitude, double &longitude)
{
	if (! _looked_up && ! lookup()) {
		return false;
	}
	latitude = _latitude;
	longitude = _longitude;
	return true;
}

bool
Location::lookup()
{
	_looked_up = false;

	HttpClient::string_map headers;
	headers["Accept"] = "application/json";
	headers["Content-Type"] = "application/json";

	std::stringstream os;
	int code = _client->GET(GEOIP_PW_V2_LOOKUP_SELF, headers, os);
	if (code != 200) {
		return false;
	}

	JsonParser parser(os.str());
	JsonValueObject *value = parser.parse();
	if (value == nullptr) {
		P_WARN("failed to parse location JSON: " << parser.getError());
		return false;
	}

	JsonValueNumber *json_latitude = jsonGetNumber(value, "latitude");
	JsonValueNumber *json_longitude = jsonGetNumber(value, "longitude");
	if (json_latitude && json_longitude) {
		_latitude = *(*json_latitude);
		_longitude = *(*json_longitude);
		_looked_up = true;
	}

	JsonValueString *json_country = jsonGetString(value, "country");
	if (json_country) {
		_country = *(*json_country);
	}
	JsonValueString *json_city = jsonGetString(value, "city");
	if (json_city) {
		_city = *(*json_city);
	}

	delete value;
	return _looked_up;
}
