
#pragma once

#include <cinatra/response.hpp>
#include <cinatra/request_parser.hpp>
#include <cinatra/logging.hpp>
#include <cinatra/http_router.hpp>
#include <cinatra/session_container.hpp>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/format.hpp>

#include <memory>
#include <string>
#include <functional>
#include <iostream>
#include <fstream>

namespace cinatra
{
	using request_handler_t = std::function<bool(Request&, Response&)>;
	using error_handler_t = std::function<bool(int, const std::string&, Request&, Response&)>;

	class Connection
		: public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(boost::asio::io_service& service,
			SessionContainer& session_container,
			const request_handler_t& request_handler,
			const error_handler_t& error_handler,
			const std::string& public_dir)
			:service_(service), socket_(service), timer_(service),
			error_handler_(error_handler), public_dir_(public_dir), 
			request_handler_(request_handler), session_container_(session_container)
		{
			LOG_DBG << "New connection";
		}
		~Connection()
		{
			boost::system::error_code ec;
			socket_.close(ec);
			LOG_DBG << "Connection closed.";
		}

		boost::asio::ip::tcp::socket& socket(){ return socket_; }

		void start()
		{
			boost::asio::spawn(service_,
				std::bind(&Connection::do_work,
				shared_from_this(), std::placeholders::_1));
		}

	private:
		void do_work(const boost::asio::yield_context& yield)
		{
			for (;;)
			{
				try
				{
					reset_timer();

					std::array<char, 8192> buffer;
					RequestParser parser;

					std::size_t total_size = 0;
					for (;;)
					{
						boost::system::error_code ec;
						std::size_t n = socket_.async_read_some(boost::asio::buffer(buffer), yield[ec]);
						if (ec)
						{
							if (ec == boost::asio::error::eof)
							{
								LOG_DBG << "Socket shutdown";
							}
							else
							{
								LOG_DBG << "Network exception: " << ec.message();
							}
							close();
							return;
						}

						cancel_timer();	//读取到了数据之后就取消关闭连接的timer
						total_size += n;
						if (total_size > 2 * 1024 * 1024)
						{
							throw std::runtime_error("Request toooooooo large");
						}

						auto ret = parser.parse(buffer.data(), buffer.data() + n);
						if (ret == RequestParser::good)
						{
							break;
						}
						if (ret == RequestParser::bad)
						{
							throw std::runtime_error("HTTP Parser error");
						}
					}

					Request req = parser.get_request();
					LOG_DBG << "New request,path:" << req.path();
					// 获取session id
					std::string session_id = req.cookie().get_val("CSESSIONID");
					if (session_id.empty())
					{
						// ID为空则新建一个session
						session_id = session_container_.new_session();
					}
					// 设置session到req上
					req.set_session(session_container_.get_container(session_id));

					Response res;
					// 把session id写到cookie中
					res.cookies().new_cookie().add("CSESSIONID", session_id);
					init_response(res, yield);
					bool hasError = check_request(parser, req, res);		
					add_version(parser, req, res);
					add_keepalive(parser, req, res);
					add_conten_type(res);		

					if (!hasError)
					{
						bool r = request_handler_(req, res);
						if (!res.is_complete() && !r)
						{
							if (response_file(req, res.header.hasKeepalive(), yield))
							{
								continue;
							}

							error_handler_(404, "", req, res);
						}
					}

					response(res, yield);

					close_connection(parser, req);
				}
				catch (boost::system::system_error& e)
				{
					//网络通信异常，关socket.
					LOG_DBG << "Network exception: " << e.code().message();
					close();
					return;
				}
				catch (std::exception& e)
				{
					LOG_ERR << "Error occurs,response 500: " << e.what();
					response_5xx(e.what(), yield);
					shutdown();
				}
				catch (...)
				{
					response_5xx("", yield);
					shutdown();
				}
			}
		}


		void init_response(Response& res, const boost::asio::yield_context& yield)
		{
			res.direct_write_func_ =[&yield, this](const char* data, std::size_t len)
			{
				boost::system::error_code ec;
				boost::asio::async_write(socket_, boost::asio::buffer(data, len), yield[ec]);
				if (ec)
				{
					LOG_WARN << "direct_write_func error" << ec.message();
					return false;
				}
				return true;
			};
		}

		bool check_request(const RequestParser& parser, Request& req, Response& res)
		{
			bool hasError = false;
			//check request
			if (parser.is_version10())
			{

			}
			else if (parser.is_version11())
			{
				if (req.header().get_count("host") == 0)
				{
					hasError = error_handler_(400, "Bad Request", req, res);
				}
			}
			else
			{
				hasError = error_handler_(400, "Unsupported HTTP version.", req, res);
			}

			return hasError;
		}

		void add_version(const RequestParser& parser, Request& /* req */, Response& res)
		{
			if (parser.is_version10())
			{
				res.set_version(1, 0);
			}
			else if (parser.is_version11())
			{
				res.set_version(1, 1);
			}
		}

		/*
		如果是http1.0，规则是这样的：
		如果request里面的connection是keep-alive，那就说明浏览器想要长连接，服务器如果也同意长连接，
		那么返回的response的connection也应该有keep-alive通知浏览器，如果不想长链接，response里面就不应该有keep-alive;
		如果是1.1的，规则是这样的：
		如果request里面的connection是close，那就说明浏览器不希望长连接，如果没有close，就是默认保持长链接，
		本来是跟keep-alive没关系，但是如果浏览器发了keep-alive，那你返回的时候也应该返回keep-alive;
		惯例是根据有没有close判断是否长链接，但是如果没有close但是有keep-alive，你response也得加keep-alive;
		如果没有close也没有keep-alive
		那就是长链接但是不用返回keep-alive
		*/
		void add_keepalive(const RequestParser& parser, Request& req, Response& res)
		{
			if (parser.is_version10())
			{
				if (req.header().val_ncase_equal("Connetion", "Keep-Alive"))
				{
					res.header.add("Connetion", "Keep-Alive");
				}
			}
			else if (parser.is_version11())
			{
				if (req.header().val_ncase_equal("Connetion", "Keep-Alive"))
				{
					res.header.add("Connetion", "Keep-Alive");
				}
			}
		}

		void add_conten_type(Response& res)
		{
			if (res.header.get_count("Content-Type") == 0)
			{
				res.header.add("Content-Type", "text/html");
			}
		}

		void response(Response& res, const boost::asio::yield_context& yield)
		{
			if (!res.is_complete_)
			{
				res.end();
			}

			if (!res.is_chunked_encoding_)
			{
				// 如果是chunked编码数据应该都发完了.
				std::string header_str = res.get_header_str();
				boost::asio::async_write(socket_, boost::asio::buffer(header_str), yield);
				boost::asio::async_write(socket_, res.buffer_,
					boost::asio::transfer_exactly(res.buffer_.size()), yield);
			}
		}

		void close_connection(const RequestParser& parser, Request& req)
		{
			if (parser.is_version10())
			{
				if (!req.header().val_ncase_equal("Connetion", "Keep-Alive"))
				{
					shutdown();
				}
			}
			else if (parser.is_version11())
			{
				if (req.header().val_ncase_equal("Connetion", "close"))
				{
					shutdown();
				}
			}
		}

		bool response_file(Request& req, bool keep_alive, const boost::asio::yield_context& yield)
		{
			std::string path = public_dir_ + req.path();
			std::fstream in(path, std::ios::binary | std::ios::in);
			if (!in)
			{
				return false;
			}

			LOG_DBG << "Response a file";
			in.seekg(0, std::ios::end);

			std::string header =
				"HTTP/1.1 200 OK\r\n"
				"Server: cinatra/0.1\r\n"
				"Date: " + header_date_str() + "\r\n"
				"Content-Type: " + content_type(path) + "\r\n"
				"Content-Length: " + boost::lexical_cast<std::string>(in.tellg()) + "\r\n";

			if (keep_alive)
			{
				header += "Connection: Keep-Alive\r\n";
			}

			header += "\r\n";
			in.seekg(0, std::ios::beg);

			boost::asio::async_write(socket_, boost::asio::buffer(header), yield);
			std::vector<char> data(1024 * 1024);
			while (!in.eof())
			{
				in.read(&data[0], data.size());
				boost::asio::async_write(socket_, boost::asio::buffer(data, size_t(in.gcount())), yield);
			}

			return true;
		}

		void shutdown(bool both = false)
		{
			LOG_DBG << "Shutdown connection";

			boost::asio::ip::tcp::socket::shutdown_type shutdown_type;
			if (both)
			{
				shutdown_type = boost::asio::ip::tcp::socket::shutdown_both;
			}
			else
			{
				shutdown_type = boost::asio::ip::tcp::socket::shutdown_send;
			}
			boost::system::error_code ignored_ec;
			socket_.shutdown(shutdown_type, ignored_ec);
		}

		void response_5xx(const std::string& msg, const boost::asio::yield_context& yield)
		{
			Request req;
			Response res;
			error_handler_(500, msg, req, res);
			boost::system::error_code ignored_ec;
			boost::asio::async_write(socket_,
				boost::asio::buffer(res.get_header_str()),
				yield[ignored_ec]);
			boost::asio::async_write(socket_,
				res.buffer_,
				boost::asio::transfer_exactly(res.buffer_.size()),
				yield[ignored_ec]);

			shutdown();
		}

		void reset_timer()
		{
			//2分钟超时.
			timer_.expires_from_now(boost::posix_time::seconds(2 * 60));
			timer_.async_wait([this](const boost::system::error_code& ec)
			{
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}
				if (ec)
				{
					LOG_DBG << "Connection timer error: " << ec.message();
				}

				LOG_DBG << "Connection timeout.";
				if (!socket_.is_open())
				{
					return;
				}
				shutdown(true);
			});
		}

		void cancel_timer()
		{
			timer_.cancel();
		}

		void close()
		{
			boost::system::error_code ignored_ec;
			socket_.close(ignored_ec);
		}
	private:
		boost::asio::io_service& service_;
		boost::asio::ip::tcp::socket socket_;
		boost::asio::deadline_timer timer_;	// 长连接超时使用的timer.
		const error_handler_t& error_handler_;
		const std::string& public_dir_;
		const request_handler_t& request_handler_;
		SessionContainer& session_container_;
	};
}
