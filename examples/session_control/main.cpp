#include "Login.h"

int main()
{
	cinatra::Cinatra<LoginControl> app;
	app.route("/", [](cinatra::Request& /* req */, cinatra::Response& res)
	{
		res.redirect("index.html");
		return;
	});
	app.route("/login", [](cinatra::Request& req, cinatra::Response& res)
	{
		auto body = cinatra::urlencoded_body_parser(req.body());
		if (!req.session().exists(SessionData::loginin_)) { //第一次登陆.
			if (body.get_val(SessionData::uid_).compare(SessionData::username_) != 0
				|| body.get_val(SessionData::pwd_).compare(SessionData::password_) != 0)
			{
				//登陆失败.
				res.end("{\"result\":-3}");
				return;
			}
		}
		else if (!req.session().get<bool>(SessionData::loginin_)) {
			if (req.session().get<std::string>(SessionData::uid_).compare(body.get_val(SessionData::uid_)) != 0
				|| req.session().get<std::string>(SessionData::pwd_).compare(body.get_val(SessionData::pwd_)) != 0) {
				//登陆失败.
				res.end("{\"result\":-3}");
				return;
			}
		}

		req.session().set(SessionData::uid_, body.get_val(SessionData::uid_));
		req.session().set<bool>(SessionData::loginin_, true);
		req.session().set(SessionData::pwd_, SessionData::password_);
		res.cookies().new_cookie()
			.add("uid", body.get_val(SessionData::uid_))
			.max_age(10 * 60) //10分钟的登陆有效期.
			.new_cookie()
			.add("flag", "1")
			.max_age(10 * 60); 

		std::string json = "{\"result\":0,\"uid\":\"";
		json += SessionData::username_ + "\"}";
		res.end(json);
	});
	app.route("/loginOut", [](cinatra::Request& req, cinatra::Response& res)
	{
		if (req.session().exists(SessionData::loginin_))
		{
			req.session().set<bool>(SessionData::loginin_, false);
		}
		res.redirect("/index.html");
	});
	app.route("/change", [](cinatra::Request& req, cinatra::Response& res)
	{
		auto body = cinatra::urlencoded_body_parser(req.body());
		if (!req.session().exists(SessionData::loginin_) || (body.get_val(SessionData::uid_)
			.compare(req.session().get<std::string>(SessionData::uid_)) != 0
			|| !req.session().get<bool>(SessionData::loginin_)))
		{
			res.end("{\"result\":-2}");
			return;
		}
		req.session().set(SessionData::nick_, body.get_val(SessionData::nick_));
		req.session().set(SessionData::pwd_, body.get_val(SessionData::pwd_));
		res.end("{\"result\":0}");
	});
	app.route("/queryInfo", [](cinatra::Request& req, cinatra::Response& res)
	{
		auto body = cinatra::urlencoded_body_parser(req.body());
		if (!req.session().exists(SessionData::loginin_) || (body.get_val(SessionData::uid_)
			.compare(req.session().get<std::string>(SessionData::uid_)) != 0
			|| !req.session().get<bool>(SessionData::loginin_)))
		{
			res.end("{\"result\":-2}");
			return;
		}
		std::string json = "{\"result\":0,\"uid\":\"";
		json += SessionData::username_ + "\",\"nick\":\"" 
			+ (req.session().exists(SessionData::nick_) ? req.session().get<std::string>(SessionData::nick_) : "not set")
			+ "\",\"pwd\":\"" 
			+ req.session().get<std::string>(SessionData::pwd_)
			+ "\"}";
		res.end(json);
		return;
	});
	app.listen("http").run();
	return 0;
}
