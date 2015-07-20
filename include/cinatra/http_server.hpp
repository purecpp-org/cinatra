
#pragma once

#ifdef _MSC_VER
#pragma warning(disable:4819)
#endif // _MSC_VER

#include "connection.hpp"
#include "io_service_pool.hpp"
#include "logging.hpp"

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include "lexical_cast.hpp"
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>


namespace cinatra
{
	template<typename... Aspect>
	class HTTPServer : boost::noncopyable
	{
	public:
		HTTPServer(std::size_t io_service_pool_size, HTTPRouter& router)
			:io_service_pool_(io_service_pool_size), router_(router),
			acceptor_(io_service_pool_.get_io_service())
		{

		}

		~HTTPServer()
		{}

		HTTPServer& set_request_handler(request_handler_t request_handler)
		{
			request_handler_ = request_handler;
			return *this;
		}

		HTTPServer& set_error_handler(error_handler_t error_handler)
		{
			error_handler_ = error_handler;
			return *this;
		}

		HTTPServer& listen(const std::string& address, const std::string& port)
		{
			LOG_DBG << "Listen on " << address << ":" << port;
			boost::asio::ip::tcp::resolver resolver(acceptor_.get_io_service());
			boost::asio::ip::tcp::resolver::query query(address, port);
			boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);
			acceptor_.open(endpoint.protocol());
			acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			acceptor_.bind(endpoint);
			acceptor_.listen();

			boost::asio::spawn(acceptor_.get_io_service(),
				std::bind(&HTTPServer::do_accept,
				this, std::placeholders::_1));
			return *this;
		}

		HTTPServer& listen(const std::string& address, unsigned short port)
		{
			return listen(address, lexical_cast<std::string>(port));
		}

		HTTPServer& public_dir(const std::string& dir)
		{
			public_dir_ = dir;
			return *this;
		}

		void run()
		{
			LOG_DBG << "Starting HTTP Server";
			io_service_pool_.run();
		}

	private:
		void do_accept(const boost::asio::yield_context& yield)
		{
			for (;;)
			{
				auto conn(
					std::make_shared<Connection<Aspect...>>(
					io_service_pool_.get_io_service(),
					request_handler_, error_handler_, router_, public_dir_));

				boost::system::error_code ec;
				acceptor_.async_accept(conn->socket(), yield[ec]);
				if (ec)
				{
					LOG_DBG << "Accept new connection failed: " << ec.message();
					continue;
				}

				conn->start();
			}
		}
	private:
		IOServicePool io_service_pool_;
		boost::asio::ip::tcp::acceptor acceptor_;

		request_handler_t request_handler_;
		error_handler_t error_handler_;
		HTTPRouter& router_;

		std::string public_dir_;
	};
}
