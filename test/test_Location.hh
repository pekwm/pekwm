//
// test_Location.hh for pekwm
// Copyright (C) 2025 Claes Nästén <pekdon@gmail.com>
//
// This program is licensed under the GNU GPL.
// See the LICENSE file for more information.
//

#include "test.hh"
#include "Location.hh"

const char *LOOKUP_V2_8_8_8_8 =
	"{\"query\":\"8.8.8.8\",\"host\":\"dns.google\",\"ip\":\"8.8.8.8\","
	"\"ip_type\":4,\"summary\":\"United States, NA\",\"city\":\"City\","
	"\"subdivision\":\"\",\"country\":\"United States\","
	"\"country_abbr\":\"US\",\"continent\":\"North America\","
	"\"continent_abbr\":\"NA\",\"latitude\":37.751,\"longitude\":-97.822,"
	"\"timezone\":\"America/Chicago\",\"postal_code\":\"\","
	"\"accuracy_radius_km\":1000,\"network\":\"8.8.8.0/24\","
	"\"asn\":\"AS15169\",\"asn_org\":\"GOOGLE\"}";

class MockHttpClient : public HttpClient {
public:
	MockHttpClient() : HttpClient() { }
	virtual ~MockHttpClient() { }

	virtual int GET(const std::string &url, const string_map &headers,
			std::ostream &os)
	{
		if (url == "https://geoip.pw/api/v2/lookup/self?pretty=false") {
			os << LOOKUP_V2_8_8_8_8;
			return 200;
		}
		return 404;
	}
};

class TestLocation : public TestSuite {
public:
	TestLocation();
	virtual ~TestLocation();

	virtual bool run_test(TestSpec, bool status);

private:
	static void testGet();
};

TestLocation::TestLocation()
	: TestSuite("Location")
{
}

TestLocation::~TestLocation()
{
}

bool
TestLocation::run_test(TestSpec spec, bool status)
{
	TEST_FN(spec, "get", testGet());
	return status;
}

void
TestLocation::testGet()
{
	MockHttpClient *client = new MockHttpClient();
	Location location(client);
	double latitude, longitude;
	ASSERT_TRUE("get", location.get(latitude, longitude));
	ASSERT_DOUBLE_EQUAL("get latitude", 37.751, latitude);
	ASSERT_DOUBLE_EQUAL("get longitude", -97.822, longitude);
	ASSERT_EQUAL("get country", "United States", location.getCountry());
	ASSERT_EQUAL("get city", "City", location.getCity());
}
