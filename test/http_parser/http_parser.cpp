#include <boost/test/unit_test.hpp>
#include <cinatra/http_parser.hpp>
#include <iostream>
using namespace cinatra;

BOOST_AUTO_TEST_CASE(http_parser_test)
{
	HTTPParser parser;
	BOOST_REQUIRE(parser.feed("GET /test HTTP/1.1\r\n", 20));
	BOOST_CHECK(!parser.is_completed());
	BOOST_REQUIRE(parser.feed("\r\n", 2));
	BOOST_CHECK(parser.is_completed());
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_url == "/test");
	BOOST_CHECK(req.raw_body == "");
	BOOST_CHECK(req.method == HTTP_GET);
	BOOST_CHECK(req.path == "/test");
}
