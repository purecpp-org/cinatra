//
// Created by qiyu on 12/5/17.
//

#include <cinatra/cinatra.h>
#include <cinatra_http/websocket.h>

using namespace cinatra;

int main() {

    cinatra::cinatra app;
    app.register_handler<GET, POST>("/", [](const request &req, response &res) {
        res.response_text("hello");
    });

    app.register_handler<GET>("/ws_echo", [](cinatra::request const &req, cinatra::response &res) {
        auto client_key = cinatra::websocket::websocket_connection::is_websocket_handshake(req);
        if (client_key.empty()) {
            res.response_text("Not a websocket handshake");
            return;
        }

        cinatra::websocket::ws_config_t cfg =
                {
                        // on_start
                        [](cinatra::websocket::ws_conn_ptr_t conn) {
                            static const std::string hello = "Hello websocket!";
                            conn->async_send_msg(hello, cinatra::websocket::TEXT,
                                                 [](boost::system::error_code const &ec) {
                                                     if (ec) {
                                                         std::cout << "Send websocket hello failed: " << ec.message()
                                                                   << std::endl;
                                                     }
                                                 });
                        },
                        // on_message
                        [](cinatra::websocket::ws_conn_ptr_t conn, boost::string_ref msg,
                           cinatra::websocket::opcode_t opc) {
                            auto back_msg = std::make_shared<std::string>(msg.to_string());
                            conn->async_send_msg(*back_msg, opc, [back_msg](boost::system::error_code const &ec) {
                                if (ec) {
                                    std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
                                }
                            });
                        },
                        nullptr, nullptr, nullptr,
                        // on_error
                        [](boost::system::error_code const &ec) {
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