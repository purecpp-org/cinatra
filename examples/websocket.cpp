//
// Created by qiyu on 12/5/17.
//

#include <cinatra/cinatra.h>
#include <cinatra_http/websocket.h>
#include <set>
#include <map>
#include <mutex>
#include <iguana/json.hpp>

using namespace cinatra;
enum MSG_TYPE{
    TALKING,
};

struct message{
    int64_t user_id;
    int64_t topic_id;
    int msg_type;
    std::string data;   //message content
};
REFLECTION(message, user_id, topic_id, msg_type, data)

class broadcast{
public:
    void on_open(websocket::ws_conn_ptr_t conn){
        conn->async_send_msg(HELLO, websocket::TEXT, [](boost::system::error_code const &ec){
            if (ec) {
                std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
            }
        });
    }

    void on_close(websocket::ws_conn_ptr_t conn){
        std::cout << "websocket closed " << std::endl;
//        if(conn!= nullptr){
//            std::unique_lock lock(mtx_);
////            connections_.erase(conn);
//        }
    }

    void on_message(websocket::ws_conn_ptr_t conn, boost::string_ref msg, websocket::opcode_t opc){
        auto back_msg = std::make_shared<std::string>(msg.data(), msg.length());
//        conn->async_send_msg(*back_msg, websocket::TEXT, [back_msg](boost::system::error_code const &ec){
//            if (ec) {
//                std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
//            }
//        });

        message _message{};
        auto r = iguana::json::from_json(_message, (*back_msg).data(), msg.length());
        if(!r){
            char reason[]  = "invalid message";
            conn->close(websocket::opcode_t::CLOSE, reason, strlen(reason));
            return;
        }

        switch(_message.msg_type){
            case MSG_TYPE::TALKING: handle_msg(_message, back_msg, conn);
                break;
            default: close(conn); break;
        }
    }

    void on_error(websocket::ws_conn_ptr_t conn, boost::system::error_code const &ec){
        std::cout << "websocket error:" << ec.message() << std::endl;
//        if(conn!= nullptr)
//            connections_.erase(conn);
    }

    void create_group(const request &req, response &res){
        auto ids = get_ids(req, res);
        if(!std::get<0>(ids))
            return;

        std::unique_lock lock(mtx_);
        auto it = groups_.find(std::get<1>(ids));
        if(it==groups_.end()){
            groups_.emplace(std::get<1>(ids), std::set{std::get<2>(ids)});
            lock.unlock();

            res.response_text("success");
        }else{
            lock.unlock();
            //the group has exist
            res.set_status(response::status_type::bad_request);
            res.response_text("the group has exist");
        }
    }

    void join_group(const request &req, response &res){
        auto ids = get_ids(req, res);
        if(!std::get<0>(ids))
            return;

        std::unique_lock lock(mtx_);
        auto it = groups_.find(std::get<1>(ids));
        if(it!=groups_.end()){
            it->second.insert(std::get<2>(ids));
            lock.unlock();
            res.response_text("success");
        }else{
            lock.unlock();
            res.set_status(response::status_type::bad_request);
            res.response_text("the group not exist");
        }
    }

private:
    void handle_msg(const message& msg, std::shared_ptr<std::string> back_msg, websocket::ws_conn_ptr_t conn){
//        conn->async_send_msg(*back_msg, websocket::opcode_t::TEXT, [back_msg](boost::system::error_code const& ec)
//        {
//            if (ec)
//            {
//                std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
//            }
//        });

        std::unique_lock lock(mtx_);
        auto it = groups_.find(msg.topic_id);
        if(it == groups_.end()){
            char reason[]  = "can't find the group";
            conn->close(websocket::opcode_t::CLOSE, reason, strlen(reason));
            return;
        }

        auto cnit = conn_groups.find(msg.topic_id);
        if(cnit==conn_groups.end()){
            //can't find the conn group, push to the third platform
            //push_all();
            std::cout<<"third platform push"<<std::endl;
            return;
        }

        std::multimap<int64_t, int64_t> clear_mp;
        for(auto uid : it->second){
            if(uid==msg.user_id) //don't send to self
                continue;

            auto i = cnit->second.find(uid);
            if(i!=cnit->second.end()){
                i->second->async_send_msg(*back_msg, websocket::opcode_t::TEXT, [back_msg, topic_id = msg.topic_id, uid, &clear_mp](boost::system::error_code const &ec) {
                    if (ec) {
                        clear_mp.emplace(topic_id, uid);
                        std::cout << "Send websocket hello failed: " << ec.message() << std::endl;
                    }
                });
            }else{
                //push
                std::cout<<"third platform push"<<std::endl;
            }
        }

        //clear the failed connection
        erase_connection(clear_mp);
    }

    void erase_connection(const std::multimap<int64_t, int64_t>& clear_mp){
        for(auto it = clear_mp.begin(), end = clear_mp.end(); it != end; it = clear_mp.upper_bound(it->first))
        {
            auto it2 = conn_groups.find(it->first);
            if(it2==conn_groups.end())
                continue;

            auto p = clear_mp.equal_range(it->first);
            for (auto it3 = p.first; it3 != p.second; ++it3) {
                if(auto cnit = it2->second.find(it3->second); cnit!=it2->second.end()){
                    it2->second.erase(it3->second);
                }
            }
        }
    }

    void close(websocket::ws_conn_ptr_t conn){
        char reason[] = "invalid message";
        conn->close(websocket::opcode_t::CLOSE, reason, strlen(reason));
        return;
    }

    std::tuple<bool, int64_t, int64_t> get_ids(const request &req, response &res){
        auto str_topic_id = req.get_header("topic_id");
        if(str_topic_id.empty()){
            res.set_status(response::status_type::bad_request);
            res.response_text("failed");
            return {};
        }

        auto str_user_id = req.get_header("user_id");
        if(str_user_id.empty()){
            res.set_status(response::status_type::bad_request);
            res.response_text("failed");
            return {};
        }

        int64_t topic_id = atoll(str_topic_id.data());
        int64_t user_id = atoll(str_user_id.data());

        return std::make_tuple(true, topic_id, user_id);
    }

private:
    const std::string HELLO = "Hello websocket!";

    std::mutex mtx_;
    std::map<int64_t, std::set<int64_t>> groups_;  //topic_id, set{uid, uid, uid,....}
    std::map<int64_t, std::map<int64_t, websocket::ws_conn_ptr_t>> conn_groups; //topic_id, map{{uid, conn}, {uid, conn},...}
};

int main() {
    cinatra::cinatra app;
    app.register_handler<GET, POST>("/", [](const request &req, response &res) {
        res.response_text("hello");
    });

    broadcast brod;

    app.register_handler<GET, POST>("/create_group", &broadcast::create_group, brod);

    app.register_handler<GET, POST>("/join_group", &broadcast::join_group, brod);

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