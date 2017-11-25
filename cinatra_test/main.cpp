
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
        return 20;
    }, mylog{});

    app.register_handler<POST>("/", [](const request& req, response& res){
        return 20;
    }, mylog{}, validate{});

    app.register_handler<GET>("/ip", [](const request& req, response& res){
        res.response_text("Your IP address is " + req.remote_endpoint().address().to_string());
    });

    person p;
    app.register_handler<GET>("/get_id", &person::get_id, p, mylog{});
    app.register_handler<GET>("/get_ip", &person::get_ip, p, mylog{});
    app.register_handler<GET>("/test_json", &person::get_json, p);

	app.set_static_path("./static");
	app.listen("0.0.0.0", "8080");
	app.run();


	return 0;
}