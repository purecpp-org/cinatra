

#define DISABLE_LOGGER

#include <cinatra/cinatra.hpp>
#include <fstream>

struct CheckLoginAspect
{
	void before(Request& req, Response& res)
	{
//  		if (!req.session().exists("uid")	//如果session没有uid
//  			&& req.path() != "/login.html"	//且访问的不是login
//  			&& req.path() != "/test_post"	//和test_post页面
//  			&& req.path().compare(0, 7, "/public"))	//也不是 public文件夹下的东西
//  		{
//  			// 跳转到登陆页面
//  			res.redirect("/login.html");
//  		}
	}

	void after(Request& req, Response& res)
	{

	}
};

struct MyStruct
{
	void hello(cinatra::Request& req, cinatra::Response& res)
	{
		res.end("Hello " + req.session().get<std::string>("uid") + "!");
	}
};

int main()
{
	cinatra::Cinatra<CheckLoginAspect> app;
	app.route("/", [](cinatra::Request& req, cinatra::Response& res)
	{
		res.end("Hello Cinatra");
	});

	// 访问/login.html进行登录.
	app.route("/test_post", [](cinatra::Request& req, cinatra::Response& res)
	{
		if (req.method() != Request::method_t::POST)
		{
			res.set_status_code(404);
			res.end("404 Not found");
			return;
		}

		auto body = cinatra::body_parser(req.body());
		req.session().set("uid", body.get_val("uid"));
		res.end("Hello " + body.get_val("uid") + "! Your password is " + body.get_val("pwd") + "...hahahahaha...");
	});


	MyStruct t;
	// 访问/hello
	app.route("/hello", &MyStruct::hello, &t);
	// 访问类似于/hello/jone/10/xxx
	// joen、10和xxx会分别作为a、b和c三个参数传入handler
	app.route("/hello/:name/:age/:test", [](cinatra::Request& req, cinatra::Response& res, const std::string& a, int b, double c)
	{
		res.end("Name: " + a + " Age: " + lexical_cast<std::string>(b)+"Test: " + lexical_cast<std::string>(c));
	});
	// 
	app.route("/hello/:name/:age", [](cinatra::Request& req, cinatra::Response& res, const std::string& a, int b)
	{
		res.end("Name: " + a + " Age: " + lexical_cast<std::string>(b));
	});

	// example: /test_query?a=asdf&b=sdfg
	app.route("/test_query", [](cinatra::Request& req, cinatra::Response& res)
	{
		res.write("<html><head><title>test query</title ></head><body>");
		res.write("Total " + lexical_cast<std::string>(req.query().size()) + "queries<br />");
		for (auto it : req.query())
		{
			res.write(it.first + ": " + it.second + "<br />");
		}

		res.end("</body></html>");
	});

	//设置cookie
	app.route("/set_cookies", [](cinatra::Request& req, cinatra::Response& res)
	{
		res.cookies().new_cookie() // 会话cookie
			.add("foo", "bar")
			.new_cookie().max_age(24 * 60 * 60) //这个cookie会保存一天的时间
			.add("longtime", "test");
		res.write("<html><head><title>Set cookies</title ></head><body>");
		res.write("Please visit <a href='/show_cookies'>show cookies page</a>");
		res.end("</body></html>");
	});
	//列出所有的cookie
	app.route("/show_cookies", [](cinatra::Request& req, cinatra::Response& res)
	{
		res.write("<html><head><title>Show cookies</title ></head><body>");
		res.write("Total " + lexical_cast<std::string>(req.cookie().size()) + "cookies<br />");
		for (auto it : req.cookie())
		{
			res.write(it.first + ": " + it.second + "<br />");
		}

		res.end("</body></html>");
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
			<img src="/public/img/404.jpg" width="100%" height="100%" />
			</body>
			</html>)"
		);

		return true;
	});

	app.static_dir("./static").threads(std::thread::hardware_concurrency()).listen("0.0.0.0", "http").run();

	return 0;
}
