

#include <cinatra/http_server.hpp>
#include <thread>

int main()
{
	cinatra::HTTPServer s(std::thread::hardware_concurrency());
	s.listen("0.0.0.0", "http").run();

	return 0;
}
