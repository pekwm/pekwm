//
// Location.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#ifndef _PEKWM_LOCATION_HH_
#define _PEKWM_LOCATION_HH_

#include "HttpClient.hh"

class Location {
public:
	Location(HttpClient *client);
	virtual ~Location();

	bool get(double &latitude, double &longitude);
	const std::string &getCountry() const { return _country; }
	const std::string &getCity() const { return _city; }

protected:
	bool lookup();

private:
	HttpClient *_client;
	bool _looked_up;
	double _latitude;
	double _longitude;

	std::string _country;
	std::string _city;
};

#endif // _PEKWM_LOCATION_HH_
