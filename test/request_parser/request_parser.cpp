#include <boost/test/unit_test.hpp>
#include <cinatra/request_parser.hpp>

using namespace cinatra;

BOOST_AUTO_TEST_CASE(http_parser_simple_test)
{
	RequestParser parser;
	std::string content = "GET /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	content = "\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_url == "/test");
	BOOST_CHECK(req.raw_body.size() == 0);
	BOOST_CHECK(req.method == "GET");
	BOOST_CHECK(req.path == "/test");
}

BOOST_AUTO_TEST_CASE(http_parser_header_test)
{
	RequestParser parser;
	std::string content = "POST /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	content = "key1: val1\r\nkey1:val11\r\nkey2:val2\r\n\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(req.raw_url == "/test");
	BOOST_CHECK(req.raw_body.size() == 0);
	BOOST_CHECK(req.method == "POST");
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
	RequestParser parser;
	std::string content = "PUT /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	content = "Content-Length: 4\r\n\r\nbody";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(std::string(req.raw_body.begin(), req.raw_body.end()) == "body");
	BOOST_CHECK(req.body.size() == 0);
}

BOOST_AUTO_TEST_CASE(http_parser_kv_body_test)
{
	RequestParser parser;
	std::string content = "POST /test HTTP/1.1\r\n";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) != RequestParser::bad);
	content = "Content-Length: 3\r\n\r\nk=v";
	BOOST_REQUIRE(parser.parse(content.begin(), content.end()) == RequestParser::good);
	auto req = parser.get_request();
	BOOST_CHECK(std::string(req.raw_body.begin(), req.raw_body.end()) == "k=v");
	BOOST_REQUIRE(req.body.has_key("k"));
	BOOST_CHECK(req.body.get_val("k") == "v");
}
