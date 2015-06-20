
#pragma once

#include "connection.hpp"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>


namespace cinatra
{
	class HTTPServer : boost::noncopyable
	{
	public:
		HTTPServer(boost::asio::io_service& service)
			:service_(service),acceptor_(service)
		{

		}

		~HTTPServer()
		{}

		HTTPServer& listen(const std::string& address, const std::string& port)
		{
			boost::asio::ip::tcp::resolver resolver(service_);
			boost::asio::ip::tcp::resolver::query query(address, port);
			boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
			acceptor_.open(endpoint.protocol());
			acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor_.bind(endpoint);
			acceptor_.listen();

			boost::asio::spawn(service_,
				std::bind(&HTTPServer::do_accept,
				this, std::placeholders::_1));
			return *this;
		}

		HTTPServer& listen(const std::string& address, unsigned short port)
		{
			return listen(address, boost::lexical_cast<std::string>(port));
		}

	private:
		void do_accept(const boost::asio::yield_context& yield)
		{
			for (;;)
			{
				std::shared_ptr<Connection> conn(std::make_shared<Connection>(service_));
				boost::system::error_code ec;
				acceptor_.async_accept(conn->socket(), yield[ec]);
				if (ec)
				{
					//TODO: log error
					continue;
				}

				conn->start();
			}
		}
	private:
		boost::asio::io_service& service_;
		boost::asio::ip::tcp::acceptor acceptor_;
	};
}