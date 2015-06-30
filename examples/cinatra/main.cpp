
#include <cinatra/cinatra.hpp>
#include <fstream>

int main()
{
	cinatra::Cinatra app;
	app.route("/")
		([](cinatra::Request&, cinatra::Response& res)
	{
		res.write("Hello Cinatra!");
	});

	app.route("/test_post").method(cinatra::Request::method_t::GET)
		([](cinatra::Request&, cinatra::Response& res)
	{
		std::ifstream in("./view/login.html", std::ios::binary | std::ios::in);
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
	app.route("/test_post").method(cinatra::Request::method_t::POST)
		([](cinatra::Request& req, cinatra::Response& res)
	{
		auto body = cinatra::kv_parse(req.body());
		res.write("Hello " + body.get_val("uid") + "! Your password is " + body.get_val("pwd") + "...hahahahaha...");
	});

	app.error_handler(
		[](int code, const std::string&, cinatra::Request&, cinatra::Response& res)
	{
		if (code != 404)
		{
			return false;
		}

		res.set_status_code(404);
		res.write(
			R"(<html>
			<head>
				<meta charset="UTF-8">
				<title>404</title>
			</head>
			<body>
			<img src="/img/404.jpg" width="100%" height="100%" />
			</body>
			</html>)"
		);

		return true;
	});

	app.public_dir("./public").threads(2).listen("0.0.0.0", "http").run();

	return 0;
}
