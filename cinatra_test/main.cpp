
#include <iostream>

#include <cinatra/cinatra.h>
#include <cinatra/middleware/cookies.hpp>
#include <cinatra/middleware/session.hpp>


struct CheckLoginAspect
{
	void before(cinatra::request const& req, cinatra::response& res, cinatra::context_container& ctx)
	{
		std::cout << "before" << std::endl;
	}

	void after(cinatra::request const& req, cinatra::response& res, cinatra::context_container& ctx)
	{
		std::cout << "after" << std::endl;
	}
};


int main()
{
	cinatra::cinatra<CheckLoginAspect, cinatra::cookies, cinatra::session> app;

	app.route("/", [](cinatra::response& res)
	{
		res.response_text("hhhhhhhhhhhhhhhhhh");
	});
	app.route("/test/:name", [](std::string name, cinatra::response& res)
	{
		res.response_text(name);
	});


	app.route("/show_cookie", [](cinatra::response& res, cinatra::context_container& ctx)
	{
		std::string cookie_str;
		auto& cookie_ctx = ctx.get_req_ctx<cinatra::cookies>();
		for (auto it : cookie_ctx.get())
		{
			cookie_str += it.first + ": " + it.second + "\n";
		}
		res.response_text(cookie_str);
	});

	app.route("/add_cookie", [](cinatra::response& res, cinatra::context_container& ctx)
	{
		auto& cookie_ctx = ctx.get_req_ctx<cinatra::cookies>();
		cinatra::cookies::cookie_t cookie;
		cookie.add("foo", "bar");
		cookie.set_http_only(true);
		cookie_ctx.add_cookie(cookie);

		res.response_text("over");
	});

	app.route("/set_session", [](cinatra::response& res, cinatra::context_container& ctx)
	{
		auto s = ctx.get_req_ctx<cinatra::session>();
		s.add("test", std::string("hello"));
		res.response_text("OK");
	});

	app.route("/show_session", [](cinatra::response& res, cinatra::context_container& ctx)
	{
		auto s = ctx.get_req_ctx<cinatra::session>();
		if (s.has("test"))
		{
			res.response_text(s.get<std::string>("test"));
		}
		else
		{
			res.response_text("no session");
		}
	});

	app.set_static_path("./static");
	app.listen("127.0.0.1", "8080");
	app.run();


	return 0;
}