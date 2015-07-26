#include <cinatra/cinatra.hpp>

int main()
{
	cinatra::Cinatra<> app;
	app.route("/", [](cinatra::Request& req, cinatra::Response& res)
	{
		res.write("url: " + req.url() + "\n");
		res.write("header: \n");
		for(auto&& it : req.header())
			res.write(it.first + ": " + it.second + "\n");
		res.write("body: \n");
		res.write(std::string(req.body().begin(), req.body().end()) + "\n");
		res.end();
	}).listen("0.0.0.0", "http").run();
	return 0;
}
