//
// Created by qiyu on 12/5/17.
//

#include <cinatra/cinatra.h>
#include <cinatra_http/websocket.h>
#include <set>
#include <mutex>

using namespace cinatra;

class broadcast{
public:
    void create_group(const request &req, response &res){
        res.response_text("ok");
    }

    void on_open(websocket::ws_conn_ptr_t conn){
        connections_.insert(conn);
        conn->async_send_msg(HELLO, websocket::TEXT, [](boost::system::error_code const &ec){
            if (ec) {
                std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
            }
        });
    }

    void on_close(websocket::ws_conn_ptr_t conn){
        std::cout << "websocket closed " << std::endl;
        if(conn!= nullptr){
            std::unique_lock lock(mtx_);
            connections_.erase(conn);
        }
    }

    void on_message(websocket::ws_conn_ptr_t conn, boost::string_ref msg, websocket::opcode_t opc){
        auto back_msg = std::make_shared<std::string>(msg.to_string());
        std::unique_lock lock(mtx_);
        for(auto conn1 : connections_){
            conn1->async_send_msg(*back_msg, opc, [back_msg](boost::system::error_code const &ec) {
                if (ec) {
                    std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
                }
            });
        }
    }

    void on_error(websocket::ws_conn_ptr_t conn, boost::system::error_code const &ec){
        std::cout << "websocket error:" << ec.message() << std::endl;
        if(conn!= nullptr)
            connections_.erase(conn);
    }

private:
    const std::string HELLO = "Hello websocket!";
    std::set<websocket::ws_conn_ptr_t> connections_;
    std::mutex mtx_;
};

int main() {

    cinatra::cinatra app;
    app.register_handler<GET, POST>("/", [](const request &req, response &res) {
        res.response_text("hello");
    });

    broadcast brod;
    app.register_handler<GET, POST>("/create_group", &broadcast::create_group, brod);

    app.register_handler<GET>("/ws_echo", [&brod](cinatra::request const &req, cinatra::response &res) {
        auto client_key = cinatra::websocket::websocket_connection::is_websocket_handshake(req);
        if (client_key.empty()) {
            res.response_text("Not a websocket handshake");
            return;
        }

        cinatra::websocket::ws_config_t cfg;
        cfg.on_start = std::bind(&broadcast::on_open, &brod, std::placeholders::_1);
        cfg.on_close = std::bind(&broadcast::on_close, &brod, std::placeholders::_1);
        cfg.on_message = std::bind(&broadcast::on_message, &brod, std::placeholders::_1,
                                    std::placeholders::_2,std::placeholders::_3);
        cfg.on_error = std::bind(&broadcast::on_error, &brod, std::placeholders::_1, std::placeholders::_2);

        cinatra::websocket::websocket_connection::upgrade_to_websocket(req, res, client_key, cfg);
    });

    app.set_static_path("./static");
    app.listen("0.0.0.0", "8080");
    app.run();

    return 0;
}