
#pragma once

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
				boost::asio::streambuf buf;
				boost::asio::async_read_until(socket_, buf, "\r\n\r\n", yield);

				//先输出一下request看看
				std::string req_header;
				req_header.resize(buf.size());
				buf.sgetn(&req_header[0], buf.size());
				std::cout << "received request:\n" << req_header << std::endl;

				std::string res_str =
					"HTTP/1.0 200 OK\r\n"
					"Content-Length: 23\r\n"
					"Content-Type: text/html\r\n\r\n"
					"Hello,world!";
				boost::asio::async_write(socket_, boost::asio::buffer(res_str), yield);
			}
			catch (boost::system::system_error& /*e*/)
			{
				// on err
			}

			boost::system::error_code ignored_ec;
			socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
		}
	private:
		boost::asio::io_service& service_;
		boost::asio::ip::tcp::socket socket_;
	};
}