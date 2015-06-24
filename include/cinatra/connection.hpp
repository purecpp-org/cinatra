
#pragma once

#include "http_parser.hpp"
#include "response.hpp"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <memory>
#include <string>
#include <iostream>

namespace cinatra
{
	class Connection
		: public std::enable_shared_from_this<Connection>
	{
	public:
		Connection(boost::asio::io_service& service)
			:service_(service), socket_(service)
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

					bool keep_alive;
					bool close_connection;
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
							return false;
						}
						return true;
					};

					if (keep_alive)
					{
						res.header.add("Connetion", "Keep-Alive");
					}

					//TODO: 添加router.
					//TODO: 如果在router中没有找到匹配的，则查找public dir是否有该文件.
					//TODO: 如果都没有找到，404.

					{
						// test.
						if (req.path == "/")
						{
							res.write("Hello,world");
						}
						else if (req.path == "/123")
						{
							int n = boost::lexical_cast<int>(req.query.get_val("n"));
							for (int i = 0; i < n; ++i)
							{
								res.direct_write("Hello " + boost::lexical_cast<std::string>(i) + "\n");
							}
						}
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
					// TODO: log err and response 500
					std::cout << "error: " << e.what() << std::endl;
					break;
				}
			}

			boost::system::error_code ignored_ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		}
	private:
		boost::asio::io_service& service_;
		boost::asio::ip::tcp::socket socket_;
	};
}
