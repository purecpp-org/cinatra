
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

	app.route("/test_post").method("GET")
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
	app.route("/test_post").method("POST")
		([](const cinatra::Request& req, cinatra::Response& res)
	{
		res.write("Hello " + req.body.get_val("uid") + "! Your password is " + req.body.get_val("pwd") + "...hahahahaha...");
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
				<meta charset="GB2312">
				<title>悲剧啊</title>
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
