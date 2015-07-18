
#include <cinatra/cinatra.hpp>

#include <fstream>

int main()
{
	cinatra::Cinatra app;
	//支持函数指针和成员函数.
// 	app.route("/", []
// 		(const cinatra::Request& req, cinatra::Response& res)
// 	{
// 		res.write("Hello Cinatra!");
// 	});

// 	app.route("/test_post", []
// 		(const cinatra::Request& req, cinatra::Response& res)
// 	{
// 		std::ifstream in("./view/login.html", std::ios::binary | std::ios::in);
// 		if (!in)
// 		{
// 			res.write("can not open login.html");
// 			return;
// 		}
// 
// 		std::string html;
// 		in.seekg(0, std::ios::end);
// 		html.resize(size_t(in.tellg()));
// 		in.seekg(0, std::ios::beg);
// 		in.read(&html[0], html.size());
// 
// 		res.write(html);
// 	});
// 
// 	app.route("/test_post", []
// 		(const cinatra::Request& req, cinatra::Response& res)
// 	{
// 		auto body = cinatra::body_parser(req.body());
// 		res.write("Hello " + body.get_val("uid") + "! Your password is " + body.get_val("pwd") + "...hahahahaha...");
// 	});
// 
// 	app.route("/test_query", []
// 		(const cinatra::Request& req, cinatra::Response& res)
// 	{
// 		res.write("<html><body>");
// 		res.write("total " + boost::lexical_cast<std::string>(req.query().size()) + " queries</br>");
// 		for (auto it : req.query())
// 		{
// 			res.write(it.first + ":" + it.second + "</br>");
// 		}
// 		res.end("</body></html>");
// 	});
// 
// 	app.route("/test_ajax", []
// 		(const cinatra::Request& req, cinatra::Response& res)
// 	{
// 		res.end("Hello ajax!");
// 	});
// 
	app.route("/add/:name/:age", []
		(const std::string& a, int b)
	{
		std::cout << a << /*" " << req.url() << */std::endl;
	});
// 
// 	app.route("/hello", []
// 		(const cinatra::Request& req, cinatra::Response& res, const std::string& a, int b)
// 	{
// 		std::cout << a << b << std::endl;
// 		res.write("t");
// 		std::cout << req.path() << std::endl;
// 	});
// 
// 	app.route("/test_cookie",
// 		[](const cinatra::Request& req, cinatra::Response& res)
// 	{
// 		auto cookies = cinatra::cookie_parser(req.cookie());
// 		res.cookies()
// 			.new_cookie()
// 			.add("session1", "1")
// 			.add("session2", "foo")	// 这个cookie应该在会话结束之后就没了.
// 			.new_cookie()	// 添加一个新cookie，这个cookie应该会保存一天的时间.
// 			.add("longtime", "bar")
// 			.expires(time(NULL) + 24 * 60 * 60)
// 			.new_cookie()
// 			.add("a=b", "c%=;d")
// 			.max_age(1000);
// 
// 		res.write("<html><body>");
// 		res.write("total " + boost::lexical_cast<std::string>(cookies.size()) + " cookies in request</br>");
// 		for (auto it : cookies)
// 		{
// 			res.write(it.first + ":" + it.second + "</br>");
// 		}
// 		res.end("</body></html>");
// 
// 	});
// 
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
