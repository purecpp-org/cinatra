#include <boost/test/unit_test.hpp>
#include <cinatra/response.hpp>
#include <iostream>
#include <cstring>

using namespace cinatra;

namespace cinatra {
class Connection {
public:
	static void basic_test() {
		std::stringstream ss;
		Response res;
		res.write("basic test");
		ss << &res.buffer_;
		BOOST_CHECK(ss.str() == "basic test");	
	}

	static void chunked_test() {
		std::stringstream ss;
		Response res;
		res.direct_write_func_ = [](const char* data, std::size_t len) {
			static int step = 0;
			switch(step) {
				case 0:
					++step;
					break;
				case 1:
					BOOST_CHECK(strncmp(data, "5\r\nfirst\r\n", len) == 0);
					++step;
					break;
				case 2:
					BOOST_CHECK(strncmp(data, "6\r\nsecond\r\n", len) == 0);
					++step;
					break;
				case 3:
					BOOST_CHECK(strncmp(data, "5\r\nthird\r\n", len) == 0);
					++step;
					break;
				case 4:
					BOOST_CHECK(strncmp(data, "5\r\nfouth\r\n", len) == 0);
					++step;
					break;
				default:
					BOOST_CHECK(false);
			}
			return true;
		};
		res.direct_write("");
		res.direct_write("first");
		res.direct_write("second");
		res.direct_write("third");
		res.direct_write("fouth");
	}

	static void status_test() {
		Response res;
		std::stringstream ss;
		std::string first_line;
		res.set_status_code(404);
		ss << res.get_header_str();
		getline(ss, first_line);
		BOOST_CHECK(first_line == "HTTP/1.0 404 Not Found\r");
	}
};
}

BOOST_AUTO_TEST_CASE(response_basic_test)
{
	Connection::basic_test();
}

BOOST_AUTO_TEST_CASE(response_chunked_test)
{
	Connection::chunked_test();
}

BOOST_AUTO_TEST_CASE(response_status_test)
{
	Connection::status_test();
}
