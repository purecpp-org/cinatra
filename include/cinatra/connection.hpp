
#pragma once

#include "http_parser.hpp"
#include "response.hpp"

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
	typedef std::function<bool(const Request&, Response&)> handler_t;

	class Connection
		: public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(boost::asio::io_service& service,
			const handler_t& handler,
			const std::string& public_dir)
			:service_(service),
			socket_(service),
			request_handler_(handler),
			public_dir_(public_dir)
		{}
		~Connection()
		{}

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
					std::array<char, 8192> buffer;
					HTTPParser parser;

					while (!parser.is_completed())
					{
						std::size_t n = socket_.async_read_some(boost::asio::buffer(buffer), yield);
						if (!parser.feed(buffer.data(), n))
						{
							// TODO:添加专用的异常类型.
							throw std::invalid_argument("bad request");
						}
					}

					Request req = parser.get_request();

					bool keep_alive{};
					bool close_connection{};
					if (parser.check_version(1, 0))
					{
						// HTTP/1.0
						if (req.header.val_ncase_equal("Connetion", "Keep-Alive"))
						{
							keep_alive = true;
							close_connection = false;
						}
						else
						{
							keep_alive = false;
							close_connection = true;
						}
					}
					else if (parser.check_version(1, 1))
					{
						// HTTP/1.1
						if (req.header.val_ncase_equal("Connetion", "close"))
						{
							keep_alive = false;
							close_connection = true;
						}
						else if (req.header.val_ncase_equal("Connetion", "Keep-Alive"))
						{
							keep_alive = true;
							close_connection = false;
						}
						else
						{
							keep_alive = false;
							close_connection = false;
						}

						if (req.header.get_count("host") == 0)
						{
							// TODO:添加专用的异常类型.
							throw std::invalid_argument("bad request");
						}
					}
					else
					{
						throw std::invalid_argument("bad request");
					}
					Response res;
					auto self = shared_from_this();
					res.direct_write_func_ = 
						[&yield, self, this]
					(const char* data, std::size_t len)->bool
					{
						boost::system::error_code ec;
						boost::asio::async_write(socket_, boost::asio::buffer(data, len), yield[ec]);
						if (ec)
						{
							// TODO: log ec.message().
							std::cout << "direct_write_func error" << ec.message() << std::endl;
							return false;
						}
						return true;
					};

					if (keep_alive)
					{
						res.header.add("Connetion", "Keep-Alive");
					}

					bool found = false;
					if (request_handler_)
					{
						found = request_handler_(req, res);
					}
					if (!found && response_file(req, keep_alive, yield))
					{
						continue;
					}
					//TODO: 如果都没有找到，404.

					//用户没有指定Content-Type，默认设置成text/html
					if (res.header.get_count("Content-Type") == 0)
					{
						res.header.add("Content-Type", "text/html");
					}

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

					if (close_connection)
					{
						break;
					}
				}
				catch (std::exception& e)
				{
					// FIXME: 单独处理网络错误，在read得到eof的时候close连接
					// TODO: log err and response 500
					std::cout << "error: " << e.what() << std::endl;
					break;
				}
			}

			boost::system::error_code ignored_ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		}

		bool response_file(const Request& req, bool keep_alive, const boost::asio::yield_context& yield)
		{
			std::string path = public_dir_ + req.path;
			std::fstream in(path, std::ios::binary | std::ios::in);
			if (!in)
			{
				return false;
			}

			in.seekg(0, std::ios::end);

			std::string header =
				boost::str(boost::format(
				"HTTP/1.1 200 OK\r\n"
				"Server: cinatra/0.1\r\n"
				"Date: %1%\r\n"
				"Content-Type: %2%\r\n"
				"Content-Length: %3%\r\n"
				)
				% date_str()
				% content_type(path)
				% in.tellg());

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
				//FIXME: warning C4244: “参数”: 从“std::streamoff”转换到“unsigned int”，可能丢失数据.
				boost::asio::async_write(socket_, boost::asio::buffer(data, in.gcount()), yield);
			}

			return true;
		}
	private:
		boost::asio::io_service& service_;
		boost::asio::ip::tcp::socket socket_;
		const handler_t& request_handler_;
		const std::string& public_dir_;
	};
}
