

// #define CINATRA_DISABLE_LOGGER

// set max request size 10m,default is 2m
#define CINATRA_REQ_MAX_SIZE 10 * 1024 * 1024

#include <cinatra/cinatra.hpp>
#include <cinatra/middleware/cookie.hpp>
#include <cinatra/middleware/session.hpp>

#include <cinatra/html_template.hpp>

#include <fstream>

struct CheckLoginAspect
{
	void before(cinatra::Request& req, cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		auto & session = ctx.get_req_ctx<cinatra::Session>();
		if (!session.has("uid")&&req.path()!="/login.html"&&
			req.path() != "/test_post"&&req.path().compare(0, 7, "/public"))	//如果session没有uid且访问的不是login和test_post页面.
 		{
 			// 跳转到登陆页面.
 			res.redirect("/login.html");
 		}
	}

	void after(cinatra::Request& /* req */, cinatra::Response& /* res */, cinatra::ContextContainer& /*ctx*/)
	{

	}
};

struct MyStruct
{
	void hello(cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		auto& session = ctx.get_req_ctx<cinatra::Session>();
		res.end("Hello " + session.get<std::string>("uid") + "!");
	}
};

int main()
{
	cinatra::Cinatra<
		cinatra::RequestCookie,
		cinatra::ResponseCookie,
		cinatra::Session,
		CheckLoginAspect
	> app;

	app.get_middleware<cinatra::Session>().set_session_life_circle(5 * 60);
	app.route("/", [](cinatra::Request& /* req */, cinatra::Response& res)
	{
		res.end("Hello Cinatra");
	});

	// 访问/login.html进行登录.
	app.route("/test_post", [](cinatra::Request& req, cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		if (req.method() != cinatra::Request::method_t::POST)
		{
			res.set_status_code(404);
			res.end("404 Not found");
			return;
		}

		auto body = cinatra::urlencoded_body_parser(req.body());
		auto& session = ctx.get_req_ctx<cinatra::Session>();
		session.set("uid", body.get_val("uid"));
		res.end("Hello " + body.get_val("uid") + "! Your password is " + body.get_val("pwd") + "...hahahahaha...");
	});


	MyStruct t;
	// 访问/hello
	app.route("/hello", &MyStruct::hello, &t);
	// 访问类似于/hello/jone/10/xxx
	// joen、10和xxx会分别作为a、b和c三个参数传入handler
	app.route("/hello/:name/:age/:test", [](cinatra::Response& res, const std::string& a, int b, double c)
	{
		res.end("Name: " + a + " Age: " + boost::lexical_cast<std::string>(b)+"Test: " + boost::lexical_cast<std::string>(c));
	});
	// 
	app.route("/hello/:name/:age", [](cinatra::Response& res, const std::string& a, int b)
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
	app.route("/set_cookies", [](cinatra::ContextContainer& ctx, cinatra::Response& res)
	{
		auto& cookie = ctx.get_req_ctx<cinatra::ResponseCookie>();
		cookie.new_cookie() // 会话cookie
			.add("foo", "bar")
			.new_cookie().max_age(24 * 60 * 60) //这个cookie会保存一天的时间.
			.add("longtime", "test");
		res.write("<html><head><title>Set cookies</title ></head><body>");
		res.write("Please visit <a href='/show_cookies'>show cookies page</a>");
		res.end("</body></html>");
	});
	//列出所有的cookie
	app.route("/show_cookies", [](cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		auto& cookie = ctx.get_req_ctx<cinatra::RequestCookie>();
		res.write("<html><head><title>Show cookies</title ></head><body>");
		res.write("Total " + boost::lexical_cast<std::string>(cookie.get_all().size()) + "cookies<br />");
		for (auto it : cookie.get_all())
		{
			res.write(it.first + ": " + it.second + "<br />");
		}

		res.end("</body></html>");
	});

	//测试html template engine和json
	app.route("/template", [](cinatra::Response& res)
	{
		//html模板修改自RedZone
		//具体使用方式和模板语法请参考官方文档
		//https://github.com/jcfromsiberia/RedZone
		//Thanks Ivan Il'in!
		cinatra::render(res, "./view/test.tpl", cinatra::Json::object{
			{ "title", "hello" },
			{ "foo", "bar" },
			{ "test_loop", cinatra::Json::array{"aaa", "sssssssssss", "ddddddd"} },
		});
	});

	app.static_dir("./static")
		.listen("0.0.0.0", "https", cinatra::HttpsConfig(
		true,
		cinatra::HttpsConfig::none,
		[](std::size_t, int)->std::string{ return "123456"; },
		"server.crt",
		"server.key",
		"dh2048.pem",
		""
		))
		.listen("http").run();

	return 0;
}
