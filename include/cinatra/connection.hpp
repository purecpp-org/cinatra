
#pragma once

#include <cinatra/response.hpp>
#include <cinatra/request_parser.hpp>
#include <cinatra/logging.hpp>
#include <cinatra/http_router.hpp>

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#ifdef CINATRA_ENABLE_HTTPS
#include <boost/asio/ssl.hpp>
#endif // CINATRA_ENABLE_HTTPS


#include <memory>
#include <string>
#include <functional>
#include <iostream>
#include <fstream>


// 默认一个request的大小限制.
#ifndef CINATRA_REQ_MAX_SIZE
#define CINATRA_REQ_MAX_SIZE 2 * 1024 * 1024
#endif // !REQ_MAX_SIZE


namespace cinatra
{
	using request_handler_t = std::function<bool(Request&, Response&)>;
	using error_handler_t = std::function<bool(int, const std::string&, Request&, Response&)>;

	class ConnectionBase
		: public std::enable_shared_from_this<ConnectionBase>
	{
	public:
		ConnectionBase(boost::asio::io_service& service,
			const request_handler_t& request_handler,
			const error_handler_t& error_handler,
			const std::string& static_dir)
			:service_(service), timer_(service),
			error_handler_(error_handler), static_dir_(static_dir),
			request_handler_(request_handler)
		{
		}
		~ConnectionBase()
		{
			LOG_DBG << "Connection closed.";
		}

		virtual boost::asio::ip::tcp::socket& socket() = 0;

		void start()
		{
			boost::asio::spawn(service_,
				std::bind(&ConnectionBase::do_work,
				shared_from_this(), std::placeholders::_1));
		}

	protected:
		virtual void do_work(boost::asio::yield_context yield) = 0;

		virtual std::size_t async_write(
			const boost::asio::const_buffers_1& buffer,
			const boost::asio::yield_context& yield) = 0;
		virtual std::size_t async_write(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) = 0;
		virtual std::size_t async_write(
			boost::asio::streambuf& buffer,
			const boost::asio::yield_context& yield) = 0;
		virtual std::size_t async_read(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) = 0;
		virtual std::size_t async_read_some(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) = 0;

		bool init_req_res(Request& req, Response& res,
			const RequestParser& parser, boost::asio::yield_context& yield)
		{
// 			// 获取session id
// 			std::string session_id = req.cookie().get_val("CSESSIONID");
// 			if (session_id.empty())
// 			{
// 				// ID为空则新建一个session
// 				session_id = session_container_.new_session();
// 			}
// 			// 设置session到req上
// 			req.set_session(session_container_.get_container(session_id));
// 
// 			// 把session id写到cookie中
// 			res.cookies().new_cookie().add("CSESSIONID", session_id);
			init_response(res, yield);
			bool has_error = check_request(parser, req, res);
			add_version(parser, req, res);
			add_keepalive(parser, req, res);
			add_conten_type(res);

			return !has_error;
		}

		void init_response(Response& res, const boost::asio::yield_context& yield)
		{
			res.direct_write_func_ = [&yield, this](const char* data, std::size_t len)
			{
				boost::system::error_code ec;
				async_write(boost::asio::buffer(data, len), yield[ec]);
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
			if (!res.is_complete())
			{
				res.end();
			}

			if (!res.is_chunked_encoding_)
			{
				// 如果是chunked编码数据应该都发完了.
				std::string header_str = res.get_header_str();
				async_write(boost::asio::buffer(header_str), yield);
				async_write(res.buffer_, yield);
			}
		}

		bool response_file(Request& req, bool keep_alive, const boost::asio::yield_context& yield)
		{
			if (static_dir_.empty())
			{
				return false;
			}
			std::string path = static_dir_ + req.path();
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

			async_write(boost::asio::buffer(header), yield);
			std::vector<char> data(1024 * 1024);
			while (!in.eof())
			{
				in.read(&data[0], data.size());
				async_write(boost::asio::buffer(data, size_t(in.gcount())), yield);
			}

			return true;
		}

		void response_5xx(const std::string& msg, const boost::asio::yield_context& yield)
		{
			Request req;
			Response res;
			error_handler_(500, msg, req, res);
			boost::system::error_code ignored_ec;
			async_write(boost::asio::buffer(res.get_header_str()), yield[ignored_ec]);
			async_write(res.buffer_, yield[ignored_ec]);

			close();
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
				if (!socket().is_open())
				{
					return;
				}
				close();
			});
		}

		void cancel_timer()
		{
			timer_.cancel();
		}

		void close()
		{
			LOG_DBG << "Close Http connection";

			boost::system::error_code ignored_ec;
			socket().close(ignored_ec);
		}

		boost::asio::io_service& service_;
		boost::asio::deadline_timer timer_;	// 长连接超时使用的timer.
		const error_handler_t& error_handler_;
		const std::string& static_dir_;
		const request_handler_t& request_handler_;
	};

	//HTTP链接.
	class TCPConnection
		: public ConnectionBase
	{
	public:
		TCPConnection(boost::asio::io_service& service,
			const request_handler_t& request_handler,
			const error_handler_t& error_handler,
			const std::string& static_dir)
			:ConnectionBase(service,request_handler,
			error_handler,static_dir),
			socket_(service)
		{
			LOG_DBG << "New http connection";
		}

		boost::asio::ip::tcp::socket& socket() override { return socket_; }

	protected:
		virtual std::size_t async_write(
			const boost::asio::const_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_write(socket_, buffer, yield);
		}

		virtual std::size_t async_write(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_write(socket_, buffer, yield);
		}

		virtual std::size_t async_write(
			boost::asio::streambuf& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_write(socket_, buffer,
				boost::asio::transfer_exactly(buffer.size()), yield);
		}

		virtual std::size_t async_read(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_read(socket_, buffer, yield);
		}

		virtual std::size_t async_read_some(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return socket_.async_read_some(buffer, yield);
		}
		virtual void do_work(boost::asio::yield_context yield) override
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
						std::size_t n = async_read_some(boost::asio::buffer(buffer), yield[ec]);
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
						if (total_size > CINATRA_REQ_MAX_SIZE)
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
					Response res;

					if (init_req_res(req, res, parser, yield))
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


		void shutdown(bool both = false)
		{
			LOG_DBG << "Shutdown Http connection";

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

	private:
		boost::asio::ip::tcp::socket socket_;
	};

#ifdef CINATRA_ENABLE_HTTPS
	//HTTPS链接.
	class SSLConnection
		: public ConnectionBase
	{
	public:
		SSLConnection(boost::asio::io_service& service,
			boost::asio::ssl::context& ctx,
			const request_handler_t& request_handler,
			const error_handler_t& error_handler,
			const std::string& static_dir)
			:ConnectionBase(service, request_handler,
			error_handler, static_dir),
			socket_(service,ctx)
		{
			LOG_DBG << "New https connection";
		}

		boost::asio::ip::tcp::socket& socket() override { return socket_.next_layer(); }

	protected:
		virtual std::size_t async_write(
			const boost::asio::const_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_write(socket_, buffer, yield);
		}

		virtual std::size_t async_write(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_write(socket_, buffer, yield);
		}

		virtual std::size_t async_write(
			boost::asio::streambuf& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_write(socket_, buffer,
				boost::asio::transfer_exactly(buffer.size()), yield);
		}

		virtual std::size_t async_read(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return boost::asio::async_read(socket_, buffer, yield);
		}

		virtual std::size_t async_read_some(
			const boost::asio::mutable_buffers_1& buffer,
			const boost::asio::yield_context& yield) override
		{
			return socket_.async_read_some(buffer, yield);
		}
		virtual void do_work(boost::asio::yield_context yield) override
		{
			boost::system::error_code ec;
			socket_.async_handshake(boost::asio::ssl::stream_base::server, yield[ec]);
			if (ec)
			{
				LOG_ERR << "SSL handshake failed: " << ec.message();
				return;
			}
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
						std::size_t n = async_read_some(boost::asio::buffer(buffer), yield);
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
						if (total_size > CINATRA_REQ_MAX_SIZE)
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
					Response res;

					if (init_req_res(req,res,parser,yield))
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

					if (parser.is_version10())
					{
						if (!req.header().val_ncase_equal("Connetion", "Keep-Alive"))
						{
							shutdown(yield);
							close();
							return;
						}
					}
					else if (parser.is_version11())
					{
						if (req.header().val_ncase_equal("Connetion", "close"))
						{
							shutdown(yield);
							close();
							return;
						}
					}
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
					shutdown(yield);
					close();
					return;
				}
				catch (...)
				{
					response_5xx("", yield);
					shutdown(yield);
					close();
					return;
				}
			}
		}


		void shutdown(boost::asio::yield_context& yield)
		{
			LOG_DBG << "Shutdown Https connection";
			boost::system::error_code ignored_ec;
			socket_.async_shutdown(yield[ignored_ec]);
		}

	private:
		boost::asio::ssl::stream<
			boost::asio::ip::tcp::socket
		> socket_;
	};
#endif // CINATRA_ENABLE_HTTPS
}
