
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

	app.route("/test_post").method(cinatra::Request::method_t::GET)
		([](const cinatra::Request&, cinatra::Response& res)
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
		([](const cinatra::Request& req, cinatra::Response& res)
	{
		auto body = cinatra::body_parser(req.body());
		res.write("Hello " + body.get_val("uid") + "! Your password is " + body.get_val("pwd") + "...hahahahaha...");
	});

	app.route("/test_query").method(cinatra::Request::method_t::GET)
		([](const cinatra::Request& req, cinatra::Response& res)
	{
		res.write("<html><body>");
		res.write("total " + boost::lexical_cast<std::string>(req.query().size()) + " queries</br>");
		for (auto it : req.query().get_all())
		{
			res.write(it.first + ":" + it.second + "</br>");
		}
		res.end("</body></html>");
	});

	app.route("/test_cookie")
		([](const cinatra::Request& req, cinatra::Response& res)
	{
		auto cookies = cinatra::cookie_parser(req.cookie());
		res.header.add("Set-Cookie", "foo=bar;");
		res.write("<html><body>");
		res.write("total " + boost::lexical_cast<std::string>(cookies.size()) + " cookies in request</br>");
		for (auto it : cookies.get_all())
		{
			res.write(it.first + ":" + it.second + "</br>");
		}
		res.end("</body></html>");

	});

	app.error_handler(
		[](int code, const std::string&, const cinatra::Request&, cinatra::Response& res)
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
