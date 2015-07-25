#include <cinatra/cinatra.hpp>

int main()
{
	cinatra::Cinatra<> app;
	app.route("/", [](cinatra::Request& /* req */, cinatra::Response& res)
	{
		res.end("hello world");
	}).listen("0.0.0.0", "http").run();
	return 0;
}
