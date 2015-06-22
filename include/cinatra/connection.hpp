
#pragma once

#include "http_parser.hpp"

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
			try
			{
				std::array<char, 8192> buffer;
				HTTPParser parser;

				while (!parser.is_completed())
				{
					std::size_t n = socket_.async_read_some(boost::asio::buffer(buffer), yield);
					if (!parser.feed(buffer.data(), n))
					{
						throw std::exception("bad request");
					}
				}

				Request req = parser.get_request();
				//Êä³öÒ»ÏÂrequest
				{
					std::cout << "request:" << std::endl;
					std::cout << "method: " << req.method << std::endl;
					std::cout << "raw url: " << req.raw_url << std::endl;
					std::cout << "raw body: " << req.raw_body << std::endl;
					std::cout << "path: " << req.path << std::endl;
					
					std::cout << "query:" << std::endl;
					for (auto iter : req.query.get_all())
					{
						std::cout << "\t" << iter.first << ": " << iter.second << std::endl;
					}
					std::cout << "body:" << std::endl;
					for (auto iter : req.body.get_all())
					{
						std::cout << "\t" << iter.first << ": " << iter.second << std::endl;
					}
					std::cout << "header:" << std::endl;
					for (auto iter : req.header.get_all())
					{
						std::cout << "\t" << iter.first << ": " << iter.second << std::endl;
					}

					std::cout << "\n\n\n\n";
				}


				std::string res_str =
					"HTTP/1.0 200 OK\r\n"
					"Content-Length: 23\r\n"
					"Content-Type: text/html\r\n\r\n"
					"Hello,world!";
				boost::asio::async_write(socket_, boost::asio::buffer(res_str), yield);
			}
			catch (std::exception& /*e*/)
			{
				// TODO: log err and response 500
			}

			boost::system::error_code ignored_ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		}
	private:
		boost::asio::io_service& service_;
		boost::asio::ip::tcp::socket socket_;
	};
}