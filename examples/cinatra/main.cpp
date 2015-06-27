
#include <cinatra/cinatra.hpp>
#include <fstream>

int main()
{
	cinatra::Cinatra app;
	app.route("/")
		([](const cinatra::Request&, cinatra::Response& res)
	{
		res.write("Hello Cinatra!");
	});

	app.route("/test_post").method(HTTP_GET)
		([](const cinatra::Request&, cinatra::Response& res)
	{
		std::ifstream in("login.html", std::ios::binary | std::ios::in);
		if (!in)
		{
			res.write("can not open login.html");
			return;
		}

		std::string html;
		in.seekg(0, std::ios::end);
		html.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&html[0], html.size());

		res.write(html);
	});	
	app.route("/test_post").method(HTTP_POST)
		([](const cinatra::Request& req, cinatra::Response& res)
	{
		res.write("Hello " + req.body.get_val("uid") + "! Your password is " + req.body.get_val("pwd") + "...hahahahaha...");
	});

	app.threads(2).listen("0.0.0.0", "http").run();

	return 0;
}