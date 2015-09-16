#include "Login.h"

int main()
{
	cinatra::Cinatra<
		cinatra::RequestCookie,
		cinatra::ResponseCookie,
		cinatra::Session,
		LoginControl
	> app;
	app.route("/", [](cinatra::Request& /* req */, cinatra::Response& res)
	{
		res.redirect("index.html");
		return;
	});
	app.route("/login", [](cinatra::Request& req, cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		auto body = cinatra::urlencoded_body_parser(req.body());
		auto& session = ctx.get_req_ctx<cinatra::Session>();
		if (!session.has(SessionData::loginin_)) { //第一次登陆.
			if (body.get_val(SessionData::uid_).compare(SessionData::username_) != 0
				|| body.get_val(SessionData::pwd_).compare(SessionData::password_) != 0)
			{
				//登陆失败.
				res.end("{\"result\":-3}");
				return;
			}
		}

		session.set(SessionData::uid_, body.get_val(SessionData::uid_));
		session.set<bool>(SessionData::loginin_, true);
		session.set(SessionData::pwd_, SessionData::password_);

		std::string json = "{\"result\":0,\"uid\":\"";
		json += SessionData::username_ + "\"}";
		res.end(json);
	});
	app.route("/loginOut", [](cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		auto& session = ctx.get_req_ctx<cinatra::Session>();
		if (session.has(SessionData::loginin_))
		{
			session.del(SessionData::loginin_);
		}
		res.redirect("/index.html");
	});
	app.route("/change", [](cinatra::Request& req, cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		auto body = cinatra::urlencoded_body_parser(req.body());
		auto& session = ctx.get_req_ctx<cinatra::Session>();

		if (!session.has(SessionData::loginin_)
			|| body.get_val(SessionData::uid_).compare(session.get<std::string>(SessionData::uid_)) != 0
			|| !session.get<bool>(SessionData::loginin_))
		{
			res.end("{\"result\":-2}");
			return;
		}
		session.set(SessionData::nick_, body.get_val(SessionData::nick_));
		session.set(SessionData::pwd_, body.get_val(SessionData::pwd_));
		res.end("{\"result\":0}");
	});
	app.route("/queryInfo", [](cinatra::Request& req, cinatra::Response& res, cinatra::ContextContainer& ctx)
	{
		auto body = cinatra::urlencoded_body_parser(req.body());
		auto& session = ctx.get_req_ctx<cinatra::Session>();

		if (!session.has(SessionData::loginin_)
			|| body.get_val(SessionData::uid_).compare(session.get<std::string>(SessionData::uid_)) != 0
			|| !session.get<bool>(SessionData::loginin_))
		{
			res.end("{\"result\":-2}");
			return;
		}
		std::string json = "{\"result\":0,\"uid\":\"";
		json += SessionData::username_ + "\",\"nick\":\"" 
			+ (session.has(SessionData::nick_) ? session.get<std::string>(SessionData::nick_) : "not set")
			+ "\",\"pwd\":\"" 
			+ session.get<std::string>(SessionData::pwd_)
			+ "\"}";
		res.end(json);
		return;
	});

	app.static_dir("./static").listen("http").run();
	return 0;
}
