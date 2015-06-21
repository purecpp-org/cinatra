

#include <cinatra/http_server.hpp>

int main()
{
	boost::asio::io_service service;
	cinatra::HTTPServer s(service);
	s.listen("0.0.0.0", "http");

	service.run();
	return 0;
}
