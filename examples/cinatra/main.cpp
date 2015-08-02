

#define DISABLE_LOGGER
// 单线程的话禁用一些锁提高效率.

#include <cinatra/cinatra.hpp>
#include <fstream>

struct CheckLoginAspect
{
	void before(cinatra::Request& req, cinatra::Response& res)
	{
		if (!req.session().exists("uid")&&req.path()!="/login.html"&&
			req.path() != "/test_post"&&req.path().compare(0, 7, "/public"))	//如果session没有uid且访问的不是login和test_post页面.
 		{
 			// 跳转到登陆页面.
 			res.redirect("/login.html");
 		}
	}

	void after(cinatra::Request& /* req */, cinatra::Response& /* res */)
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
	app.route("/", [](cinatra::Request& /* req */, cinatra::Response& res)
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

		auto body = cinatra::urlencoded_body_parser(req.body());
		req.session().set("uid", body.get_val("uid"));
		res.end("Hello " + body.get_val("uid") + "! Your password is " + body.get_val("pwd") + "...hahahahaha...");
	});


	MyStruct t;
	// 访问/hello
	app.route("/hello", &MyStruct::hello, &t);
	// 访问类似于/hello/jone/10/xxx
	// joen、10和xxx会分别作为a、b和c三个参数传入handler
	app.route("/hello/:name/:age/:test", [](cinatra::Request& /*req */, cinatra::Response& res, const std::string& a, int b, double c)
	{
		res.end("Name: " + a + " Age: " + boost::lexical_cast<std::string>(b)+"Test: " + boost::lexical_cast<std::string>(c));
	});
	// 
	app.route("/hello/:name/:age", [](cinatra::Request& /* req */, cinatra::Response& res, const std::string& a, int b)
	{
		res.end("Name: " + a + " Age: " + boost::lexical_cast<std::string>(b));
	});

	// example: /test_query?a=asdf&b=sdfg
	app.route("/test_query", [](cinatra::Request& req, cinatra::Response& res)
	{
		res.write("<html><head><title>test query</title ></head><body>");
		res.write("Total " + boost::lexical_cast<std::string>(req.query().size()) + "queries<br />");
		for (auto it : req.query())
		{
			res.write(it.first + ": " + it.second + "<br />");
		}

		res.end("</body></html>");
	});

	//设置cookie
	app.route("/set_cookies", [](cinatra::Request& /* req */, cinatra::Response& res)
	{
		res.cookies().new_cookie() // 会话cookie
			.add("foo", "bar")
			.new_cookie().max_age(24 * 60 * 60) //这个cookie会保存一天的时间.
			.add("longtime", "test");
		res.write("<html><head><title>Set cookies</title ></head><body>");
		res.write("Please visit <a href='/show_cookies'>show cookies page</a>");
		res.end("</body></html>");
	});
	//列出所有的cookie
	app.route("/show_cookies", [](cinatra::Request& req, cinatra::Response& res)
	{
		res.write("<html><head><title>Show cookies</title ></head><body>");
		res.write("Total " + boost::lexical_cast<std::string>(req.cookie().size()) + "cookies<br />");
		for (auto it : req.cookie())
		{
			res.write(it.first + ": " + it.second + "<br />");
		}

		res.end("</body></html>");
	});

	app.listen("http").run();

	return 0;
}
