
#pragma once

#ifdef _MSC_VER
#pragma warning(disable:4819)
#endif // _MSC_VER

#include <cinatra/io_service_pool.hpp>
#include <cinatra/connection.hpp>
#include <cinatra/logging.hpp>
#include <cinatra/session_container.hpp>

#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>


namespace cinatra
{
	class HTTPServer : boost::noncopyable
	{
	public:
#ifndef SINGLE_THREAD
		HTTPServer(std::size_t io_service_pool_size)
			:io_service_pool_(io_service_pool_size),
#else
		HTTPServer()
			: io_service_pool_(1),
#endif // SINGLE_THREAD
			acceptor_(io_service_pool_.get_io_service()),
			session_container_(io_service_pool_.get_io_service())
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
			return listen(address, boost::lexical_cast<std::string>(port));
		}

		HTTPServer& static_dir(const std::string& dir)
		{
			static_dir_ = dir;
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
					std::make_shared<Connection>(
					io_service_pool_.get_io_service(),session_container_,
					request_handler_, error_handler_, static_dir_));

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

		std::string static_dir_;

		SessionContainer session_container_;
	};
}
