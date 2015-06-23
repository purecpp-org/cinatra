

#include <cinatra/http_server.hpp>

int main()
{
	cinatra::HTTPServer s(boost::thread::hardware_concurrency());
	s.listen("0.0.0.0", "http").run();

	return 0;
}
