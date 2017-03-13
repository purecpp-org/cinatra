
#include <iostream>

#include <cinatra/cinatra.h>
#include <cinatra/middleware/cookies.hpp>
#include <cinatra/middleware/session.hpp>
#include <cinatra_http/websocket.h>


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

	app.get_middleware<cinatra::session>().set_timeout(50);

	app.route("/", [](cinatra::request const& req, cinatra::response& res)
	{
		res.response_text("Your IP address is " + req.remote_endpoint().address().to_string());
	});
	app.route("/test/:name/:age/:a1/a2", [](std::string name, std::string age, int a1, cinatra::response& res)
	{
		res.response_text(name + ":" + age + ":" + std::to_string(a1));
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

    app.route("/ws_echo", [](cinatra::request const& req, cinatra::response& res)
    {
        auto client_key = cinatra::websocket::websocket_connection::is_websocket_handshake(req);
        if (client_key.empty())
        {
            res.response_text("Not a websocket handshake");
            return;
        }

        cinatra::websocket::ws_config_t cfg =
        {
                // on_start
                [](cinatra::websocket::ws_conn_ptr_t conn)
                {
                    static const std::string hello = "Hello websocket!";
                    conn->async_send_msg(hello, cinatra::websocket::TEXT,
                         [](boost::system::error_code const& ec)
                         {
                             if (ec)
                             {
                                 std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
                             }
                         });
                },
                // on_message
                [](cinatra::websocket::ws_conn_ptr_t conn, boost::string_ref msg, cinatra::websocket::opcode_t opc)
                {
                    auto back_msg = std::make_shared<std::string>(msg.to_string());
                    conn->async_send_msg(*back_msg, opc, [back_msg](boost::system::error_code const& ec)
                    {
                        if (ec)
                        {
                            std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
                        }
                    });
                },
                nullptr, nullptr, nullptr,
                // on_error
                [](boost::system::error_code const& ec)
                {
                    std::cout << "upgrade to websocket failed:" << ec.message() << std::endl;
                }
        };
        cinatra::websocket::websocket_connection::upgrade_to_websocket(req, res, client_key, cfg);
    });

	app.set_static_path("./static");
	app.listen("127.0.0.1", "8080");
	app.run();


	return 0;
}