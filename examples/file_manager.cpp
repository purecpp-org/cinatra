//
// Created by qiyu on 12/5/17.
//
#include <iostream>
#include <fstream>
#include <string>
#include <boost/filesystem.hpp>
#include <cinatra/cinatra.h>
using namespace cinatra;

int main(){

    cinatra::cinatra app;
    app.register_handler<GET, POST>("/", [](const request& req, response& res){
        res.response_text("hello");
    });

    //single file up/down, the size must be less than 2M
    app.register_handler<GET, POST>("/upload", [](const request& req, response& res){
        auto name = req.get_header("filename");
        if(name.empty()){
            res.set_status(response::status_type::not_found);
            res.response_text("failed");
            return;
        }

        std::string filename = std::string("./")+std::string(name.data(),name.length());
        std::ofstream out(filename, std::ios::binary | std::ios::out);
        if (!out || !out.write(req.body().data(), req.body_len())){
            res.set_status(response::status_type::internal_server_error);
            res.response_text("failed");
        }else{
            res.response_text("ok");
        }
    });

    app.register_handler<GET, POST>("/download", [](const request& req, response& res){
        namespace fs = boost::filesystem;
        auto name = req.get_header("filename");
        if(name.empty()){
            res.set_status(response::status_type::not_found);
            res.response_text("failed");
            return;
        }

        std::string filename = std::string("./")+std::string(name.data(),name.length());
        if(!fs::exists(filename)){
            res.set_status(response::status_type::not_found);
            res.response_text("failed");
        }else{
            std::string buffer;

            std::ifstream f(filename, std::ios::binary | std::ios::in);
            f.seekg(0, std::ios::end);
            buffer.resize(f.tellg());
            f.seekg(0);
            f.read(buffer.data(), buffer.size());

            res.response_text(buffer);
        }

    });

    app.set_static_path("./static");
    app.listen("0.0.0.0", "8080");
    app.run();

    return 0;
}