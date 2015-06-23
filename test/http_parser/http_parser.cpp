#include <boost/test/unit_test.hpp>
#include <cinatra/http_parser.hpp>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(http_parser_simple_test)
{
	HTTPParser parser;
	BOOST_REQUIRE(parser.feed("GET /test HTTP/1.1\r\n", 20));
	BOOST_CHECK(!parser.is_completed());
	BOOST_REQUIRE(parser.feed("\r\n", 2));
	BOOST_REQUIRE(parser.is_completed());
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_url == "/test");
	BOOST_CHECK(req.raw_body == "");
	BOOST_CHECK(req.method == HTTP_GET);
	BOOST_CHECK(req.path == "/test");
}

BOOST_AUTO_TEST_CASE(http_parser_header_test)
{
	HTTPParser parser;
	BOOST_REQUIRE(parser.feed("POST /test HTTP/1.1\r\n", 21));
	BOOST_REQUIRE(parser.feed("key1: val1\r\n", 12));
	BOOST_REQUIRE(parser.feed("key1:val11\r\n", 12));
	BOOST_REQUIRE(parser.feed("key2:val2\r\n", 11));
	BOOST_REQUIRE(parser.feed("\r\n", 2));
	BOOST_REQUIRE(parser.is_completed());
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_url == "/test");
	BOOST_CHECK(req.raw_body == "");
	BOOST_CHECK(req.method == HTTP_POST);
	BOOST_CHECK(req.path == "/test");
	auto header = req.header;
	BOOST_CHECK(header.get_count("key1") == 2);
	BOOST_CHECK(header.get_count("key2") == 1);
	BOOST_CHECK(header.get_val("key1") == "val1" || header.get_val("key1") == "val11");
	BOOST_CHECK(header.get_val("key2") == "val2");
	auto r = header.get_vals("key1");
	BOOST_REQUIRE(r.size() == 2);
	BOOST_CHECK((r[0] == "val1" && r[1] == "val11") || (r[0] == "val11" && r[1] == "val1"));	
}

BOOST_AUTO_TEST_CASE(http_parser_body_test)
{
	HTTPParser parser;
	BOOST_REQUIRE(parser.feed("PUT /test HTTP/1.1\r\n", 20));
	BOOST_REQUIRE(parser.feed("Content-Length: 4\r\n", 19));
	BOOST_REQUIRE(parser.feed("\r\n", 2));		
	BOOST_REQUIRE(parser.feed("body", 4));		
	BOOST_REQUIRE(parser.is_completed());
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_body == "body");
	BOOST_CHECK(req.body.size() == 0);
}

BOOST_AUTO_TEST_CASE(http_parser_kv_body_test)
{
	HTTPParser parser;
	BOOST_REQUIRE(parser.feed("POST /test HTTP/1.1\r\n", 21));
	BOOST_REQUIRE(parser.feed("Content-Length: 3\r\n", 19));
	BOOST_REQUIRE(parser.feed("\r\n", 2));
	BOOST_REQUIRE(parser.feed("k=v", 3));
	BOOST_REQUIRE(parser.is_completed());
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_body == "k=v");
	BOOST_REQUIRE(req.body.has_key("k"));
	BOOST_CHECK(req.body.get_val("k") == "v");
}
