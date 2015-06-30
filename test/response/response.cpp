#include <boost/test/unit_test.hpp>
#include <cinatra/response.hpp>
#include <iostream>

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
		std::cout << "chunked test..." << std::endl;
		std::cout << "dump response package which contain 'hello' and 'world':" << std::endl;
		std::stringstream ss;
		Response res;
		res.direct_write_func_ = [](const char* data, std::size_t len) {
			std::cout.write(data, len);
			return true;
		};
		res.direct_write("hello");
		res.direct_write("world");
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
