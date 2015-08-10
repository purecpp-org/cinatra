
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
	struct HttpsConfig 
	{
		using pwd_callback_t = std::function < std::string(std::size_t, int) > ;
		enum verify_mode_t
		{
			none,
			optional,
			required,
		};

		HttpsConfig(bool ssl_enable_v3,
			verify_mode_t verify_mode,
			pwd_callback_t pwd_callback,
			const std::string& certificate_chain_file,
			const std::string& private_key_file,
			const std::string& tmp_dh_file,
			const std::string& verify_file)
			:use_https(true),
			ssl_enable_v3(ssl_enable_v3),
			verify_mode(verify_mode),
			pwd_callback(pwd_callback),
			certificate_chain_file(certificate_chain_file),
			private_key_file(private_key_file),
			tmp_dh_file(tmp_dh_file),
			verify_file(verify_file)
		{}
		HttpsConfig()
			:use_https(false)
		{}
		bool use_https;
		bool ssl_enable_v3;
		verify_mode_t verify_mode;

		pwd_callback_t pwd_callback;
		std::string certificate_chain_file;
		std::string private_key_file;
		std::string tmp_dh_file;
		std::string verify_file;
	};

	class HTTPServer : boost::noncopyable
	{
	public:
#ifndef CINATRA_SINGLE_THREAD
		HTTPServer(std::size_t io_service_pool_size)
			:io_service_pool_(io_service_pool_size),
#else
		HTTPServer()
			: io_service_pool_(1),
#endif // CINATRA_SINGLE_THREAD
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

#ifdef CINATRA_ENABLE_HTTPS
		HTTPServer& https_config(const HttpsConfig& cfg)
		{
			config_ = cfg;
			return *this;
		}
#endif // CINATRA_ENABLE_HTTPS

		void run()
		{
			LOG_DBG << "Starting HTTP Server";
			io_service_pool_.run();
		}

	private:
		void do_accept(const boost::asio::yield_context& yield)
		{
			std::shared_ptr<ConnectionBase> conn;
#ifdef CINATRA_ENABLE_HTTPS
			std::unique_ptr<boost::asio::ssl::context> ctx;
			if (config_.use_https)
			{
				ctx.reset(new boost::asio::ssl::context(boost::asio::ssl::context::sslv23));
				unsigned long ssl_options = boost::asio::ssl::context::default_workarounds
					| boost::asio::ssl::context::no_sslv2
					| boost::asio::ssl::context::single_dh_use;

				if (!config_.ssl_enable_v3)
					ssl_options |= boost::asio::ssl::context::no_sslv3;
				ctx->set_options(ssl_options);

				if (config_.pwd_callback)
				{
					ctx->set_password_callback(config_.pwd_callback);
				}

				if (config_.verify_mode == HttpsConfig::none)
				{
					ctx->set_verify_mode(boost::asio::ssl::context::verify_none);
				}
				else if (config_.verify_mode == HttpsConfig::optional)
				{
					ctx->set_verify_mode(boost::asio::ssl::context::verify_peer);
					ctx->load_verify_file(config_.verify_file);
				}
				else
				{
					// required
					ctx->set_verify_mode(boost::asio::ssl::context::verify_peer |
						boost::asio::ssl::context::verify_fail_if_no_peer_cert);
					ctx->load_verify_file(config_.verify_file);
				}

				ctx->use_certificate_chain_file(config_.certificate_chain_file);
				ctx->use_private_key_file(config_.private_key_file,
					boost::asio::ssl::context::pem);
				ctx->use_tmp_dh_file(config_.tmp_dh_file);
			}
#endif // CINATRA_ENABLE_HTTPS

			for (;;)
			{
#ifdef CINATRA_ENABLE_HTTPS
				if (ctx)
				{
					conn = std::make_shared<SSLConnection>(
						io_service_pool_.get_io_service(),
						*ctx, session_container_,
						request_handler_, error_handler_, static_dir_);
				}
				else
#endif // CINATRA_ENABLE_HTTPS
				{
					conn = std::make_shared<TCPConnection>(
						io_service_pool_.get_io_service(),session_container_,
						request_handler_, error_handler_, static_dir_);
				}

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
#ifdef CINATRA_ENABLE_HTTPS
		HttpsConfig config_;
#endif // CINATRA_ENABLE_HTTPS


		request_handler_t request_handler_;
		error_handler_t error_handler_;

		std::string static_dir_;

		SessionContainer session_container_;
	};
}
