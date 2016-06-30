#include "UnitTest.hpp"
#include <cinatra/http_server/request_parser.hpp>

using namespace cinatra;

TEST_CASE(http_parser_simple_test)
{
	RequestParser parser;
	std::string content = "GET /test HTTP/1.1\r\n";
	boost::asio::streambuf buf;
	buf.sputn(content.c_str(), content.size());
	TEST_REQUIRE(parser.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) != RequestParser::bad);
	content = "\r\n";
	buf.sputn(content.c_str(), content.size());
	parser = {};
	TEST_REQUIRE(parser.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) == RequestParser::good);
	auto req = parser.get_request();
	TEST_CHECK(req.url() == "/test");
	TEST_CHECK(req.body().size() == 0);
	TEST_CHECK(req.method() == Request::method_t::GET);
	TEST_CHECK(req.path() == "/test");
}

TEST_CASE(http_parser_header_test)
{
	RequestParser parser;
	std::string content = "POST /test HTTP/1.1\r\n";
	boost::asio::streambuf buf;
	buf.sputn(content.c_str(), content.size());
	TEST_REQUIRE(parser.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) != RequestParser::bad);
	parser = {};
	content = "key1:val1\r\nkey1: val11\r\nkey2: val2\r\n\r\n";
	buf.sputn(content.c_str(), content.size());
	TEST_REQUIRE(parser.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) == RequestParser::good);
	auto req = parser.get_request();
	TEST_CHECK(req.url() == "/test");
	TEST_CHECK(req.body().size() == 0);
	TEST_CHECK(req.method() == Request::method_t::POST);
	TEST_CHECK(req.path() == "/test");
	auto header = req.header();
	TEST_CHECK(header.get_count("key1") == 2);
	TEST_CHECK(header.get_count("key2") == 1);
	TEST_CHECK(header.get_val("key1") == "val1" || header.get_val("key1") == "val11");
	TEST_CHECK(header.get_val("key2") == "val2");
	auto r = header.get_vals("key1");
	TEST_REQUIRE(r.size() == 2);
	TEST_CHECK((r[0] == "val1" && r[1] == "val11") || (r[0] == "val11" && r[1] == "val1"));	
}

TEST_CASE(http_parser_body_test)
{
	RequestParser parser;
	std::string content = "PUT /test HTTP/1.1\r\n";
	boost::asio::streambuf buf;
	buf.sputn(content.c_str(), content.size());
	TEST_REQUIRE(parser.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) != RequestParser::bad);
	parser = {};
	content = "Content-Length: 4\r\n\r\nbody";
	buf.sputn(content.c_str(), content.size());
	TEST_REQUIRE(parser.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) == RequestParser::good);
	auto req = parser.get_request();
	auto body = req.body();
	TEST_CHECK(std::string(body.begin(), body.end()) == "body");
}

TEST_CASE(http_parser_map_test)
{
	RequestParser p;
	std::string content = "GET /test?k1=v1&k2=v2&k3=v3 HTTP/1.1\r\nke1: va1\r\nke2: va2\r\n"
		"Content-Length: 9\r\n\r\nI am bod";
	for (auto c : content)
	{
		boost::asio::streambuf buf;
		buf.sputc(c);
		TEST_CHECK(p.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) == RequestParser::indeterminate);
	}

	boost::asio::streambuf buf;
	buf.sputc('y');
	TEST_CHECK(p.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) == RequestParser::good);
	auto req = p.get_request();
	TEST_CHECK(req.method() == Request::method_t::GET);
	TEST_CHECK(req.path() == "/test");
	TEST_CHECK(req.url() == "/test?k1=v1&k2=v2&k3=v3");

	/** FIXME: TEST FAILED */
	TEST_CHECK(req.query().get_val("k1") == "v1");
	TEST_CHECK(req.query().get_val("k2") == "v2");
	TEST_CHECK(req.query().get_val("k3") == "v3");

	TEST_CHECK(req.header().get_val("ke1") == "va1");
	TEST_CHECK(req.header().get_val("ke2") == "va2");

	TEST_CHECK(std::string(req.body().begin(), req.body().end()) == "I am body");
}

TEST_CASE(http_parser_header_blank_test)
{
	RequestParser p;
	std::string content = "GET /test HTTP/1.1\r\nkey1:val1\r\nkey2: val2\r\nkey3:  val3\r\n"
		"key4: val4 \r\nkey5:     val5    \r\nContent-Length: 9\r\n\r\nI am body";
	boost::asio::streambuf buf;
	buf.sputn(content.c_str(), content.size());

	TEST_CHECK(p.parse(boost::asio::buffer_cast<const char*>(buf.data()), buf.size()) == RequestParser::good);

	auto req = p.get_request();
	TEST_CHECK(req.header().get_val("key1") == "val1");
	TEST_CHECK(req.header().get_val("key2") == "val2");
	TEST_CHECK(req.header().get_val("key3") == "val3");
	TEST_CHECK(req.header().get_val("key4") == "val4");
	TEST_CHECK(req.header().get_val("key5") == "val5");
	TEST_CHECK(std::string(req.body().begin(), req.body().end()) == "I am body");
}
