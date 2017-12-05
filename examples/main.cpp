
#include <iostream>

#include <cinatra/cinatra.h>
#include <cinatra/middleware/cookies.hpp>
#include <cinatra/middleware/session.hpp>
#include <cinatra_http/websocket.h>
using namespace cinatra;

struct mylog{
    bool before(cinatra::request const& req, cinatra::response& res)
    {
        std::cout << "log before" << std::endl;
        return true;
    }

    bool after(const std::string& result, cinatra::request const& req, cinatra::response& res)
    {
        std::cout << "log after " <<result<< std::endl;
        res.response_text(result);
        return true;
    }
};

struct validate{
    bool before(cinatra::request const& req, cinatra::response& res)
    {
        std::cout << "validate before" << std::endl;
        return true;
    }

    bool after(const std::string& result, cinatra::request const& req, cinatra::response& res)
    {
        std::cout << "validate after" << std::endl;
        res.response_text(result);
        return true;
    }
};

struct student{
    int id;
    std::string name;
    int age;
};
REFLECTION(student, id, name, age)

struct person{
    int get_id(const request& req, response& res){
        return 20;
    }

    std::string get_ip(const request& req, response& res){
        return "Your IP address is " + req.remote_endpoint().address().to_string();
    }

    void get_json(const request& req, response& res){
        student s = {1, "tom", 19};
        iguana::string_stream ss;
        iguana::json::to_json(ss, s);
        res.response_text(ss.str());
    }
};

int main()
{
    cinatra::cinatra app;
    app.register_handler<GET>("/", [](const request& req, response& res){
        return "hello";
    }, mylog{});

    app.register_handler<POST>("/", [](const request& req, response& res){
        return 20;
    }, mylog{}, validate{});

    app.register_handler<GET>("/ip", [](const request& req, response& res){
        res.response_text("Your IP address is " + req.remote_endpoint().address().to_string());
    });

    person p;
    app.register_handler<GET>("/get_id", &person::get_id, person{}, mylog{});
    app.register_handler<GET>("/get_ip", &person::get_ip, p, mylog{});
    app.register_handler<GET>("/test_json", &person::get_json, p);

    app.register_handler<GET>("/ws_echo", [](cinatra::request const& req, cinatra::response& res)
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
                conn->async_send_msg(hello, cinatra::websocket::TEXT, [](boost::system::error_code const& ec)
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
	app.listen("0.0.0.0", "8080");
	app.run();


	return 0;
}