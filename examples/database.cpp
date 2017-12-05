//
// Created by qiyu on 12/5/17.
//
#include <iostream>
#include <cinatra/cinatra.h>
#include <ormpp/mysql.hpp>
#include <ormpp/dbng.hpp>

using namespace cinatra;

struct person{
    int id;
    std::string name;
    int age;
};
REFLECTION(person, id, name, age)

int main(){

    using namespace ormpp;
    //create table
    dbng<mysql> mysql;
    bool r = mysql.connect("127.0.0.1", "root", "12345", "testdb");
    r = mysql.create_datatable<person>(ormpp_key{"id"});

    cinatra::cinatra app;
    app.register_handler<GET, POST>("/", [&mysql](const request& req, response& res){
        res.response_text("hello");
    });

    app.register_handler<GET, POST>("/add_person", [&mysql](const request& req, response& res){
        person s{};
        iguana::json::from_json(s, req.body().data());
        int r = mysql.insert(s);
        if(r<0){
            res.set_status(response::status_type::internal_server_error);
            res.response_text("failed");
        }else{
            res.response_text("ok");
        }
    });

    app.register_handler<GET, POST>("/get_person", [&mysql](const request& req, response& res){
        auto v = mysql.query<person>(); //query all
        if(v.empty()){
            res.set_status(response::status_type::internal_server_error);
            res.response_text("failed");
        }else{
            iguana::string_stream ss;
            iguana::json::to_json(ss, v[0]); //return the first person
            res.response_text(ss.data());
        }
    });

    app.register_handler<GET, POST>("/update_person", [&mysql](const request& req, response& res){
        person s{};
        iguana::json::from_json(s, req.body().data());

        auto r = mysql.update(s);
        if(r<0){
            res.set_status(response::status_type::internal_server_error);
            res.response_text("failed");
        }else{
            res.response_text("ok");
        }
    });

    app.register_handler<GET, POST>("/delete_person", [&mysql](const request& req, response& res){
        person s{};
        iguana::json::from_json(s, req.body().data());

        auto r = mysql.delete_records<person>(); //delete all
        if(!r){
            res.set_status(response::status_type::internal_server_error);
            res.response_text("failed");
        }else{
            res.response_text("ok");
        }
    });

    app.set_static_path("./static");
    app.listen("0.0.0.0", "8080");
    app.run();

    return 0;
}