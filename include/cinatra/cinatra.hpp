
#pragma once

#include <cinatra/http_server/http_server.hpp>
#include <cinatra/http_router.hpp>
#include <cinatra/http_server/request.hpp>
#include <cinatra/http_server/response.hpp>
#include <cinatra/utils/logging.hpp>
#include <cinatra/utils/aop.hpp>
#include <cinatra/body_parser.hpp>
#include <cinatra/context_container.hpp>

#include <boost/lexical_cast.hpp>

#include <string>
#include <vector>


namespace cinatra
{
	template<typename... Aspect>
	class Cinatra
	{
	public:
		template<typename F>
		typename std::enable_if<(function_traits<F>::arity>0), Cinatra&>::type route(const std::string& name,const F& f)
		{
			router_.route(name, f);
			return *this;
		}

		template<typename F, typename Self>
		typename std::enable_if<(function_traits<F>::arity>0), Cinatra&>::type route(const std::string& name, const F& f, Self* self)
		{
			router_.route(name, f, self);
			return *this;
		}

		template<typename F>
		typename std::enable_if<function_traits<F>::arity==0, Cinatra&>::type route(const std::string& name,const F& f)
		{
			route(name, [f](cinatra::Request& req, cinatra::Response& res)
			{
				auto r = f();
				res.write(boost::lexical_cast<std::string>(r));
			});

			return *this;
		}

		template<typename F, typename Self>
		typename std::enable_if<function_traits<F>::arity == 0, Cinatra&>::type route(const std::string& name, const F& f, Self* self)
		{
			route(name, [f, self](cinatra::Request& req, cinatra::Response& res)
			{
				auto r = (*self.*f)();				
				res.write(boost::lexical_cast<std::string>(r));
			});

			return *this;
		}

		Cinatra& threads(int num)
		{
			num_threads_ = num < 1 ? 1 : num;
			return *this;
		}


		Cinatra& error_handler(error_handler_t error_handler)
		{
			error_handler_ = error_handler;
			return *this;
		}

		Cinatra& listen(const std::string& address, const std::string& port)
		{
			http_listen_infos_.push_back(HttpListenInfo{ address, port });
			return *this;
		}

		Cinatra& listen(const std::string& port)
		{
			return listen("0.0.0.0", port);
		}

		Cinatra& listen(const std::string& address, unsigned short port)
		{
			return listen(address, boost::lexical_cast<std::string>(port));
		}

#ifdef CINATRA_ENABLE_HTTPS
		Cinatra& listen(const std::string& address, const std::string& port, const HttpsConfig& cfg)
		{
			https_listen_infos_.push_back(HttpsListenInfo{ address, port, cfg });
			return *this;
		}

		Cinatra& listen(const std::string& port, const HttpsConfig& cfg)
		{
			return listen("0.0.0.0", port, cfg);
		}

		Cinatra& listen(const std::string& address, unsigned short port, const HttpsConfig& cfg)
		{
			return listen(address, boost::lexical_cast<std::string>(port), cfg);
		}
#endif // CINATRA_ENABLE_HTTPS


		Cinatra& static_dir(const std::string& dir)
		{
			static_dir_ = dir;
			return *this;
		}

		template<typename T>
		T& get_middleware()
		{
			return aop_.template get_aspect<T>();
		}


		void run()
		{
#ifndef CINATRA_SINGLE_THREAD
			HTTPServer s(num_threads_);
#else
			HTTPServer s;
#endif
			aop_.set_func(std::bind(&HTTPRouter::dispatch, router_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
			s.set_request_handler([this](Request& req, Response& res)
			{
				ContextContainer ctx(app_container_);
				return aop_.invoke(req, res, ctx);
			})
				.set_error_handler(error_handler_)
				.static_dir(static_dir_);

			for (auto const & info : http_listen_infos_)
			{
				s.listen(info.address, info.port);
			}
#ifdef CINATRA_ENABLE_HTTPS
			for (auto const & info : https_listen_infos_)
			{
				s.listen(info.address, info.port, info.cfg);
			}
#endif // CINATRA_ENABLE_HTTPS
			
			s.run();
		}

	private:
		AOP<Aspect...> aop_;

#ifndef CINATRA_SINGLE_THREAD
		int num_threads_ = 1;
#else
		int num_threads_ = std::thread::hardware_concurrency();
#endif // CINATRA_SINGLE_THREAD

		struct HttpListenInfo
		{
			std::string address;
			std::string port;
		};
		std::vector<HttpListenInfo> http_listen_infos_;
		
#ifdef CINATRA_ENABLE_HTTPS
		struct HttpsListenInfo
		{
			std::string address;
			std::string port;
			HttpsConfig cfg;
		};
		std::vector<HttpsListenInfo> https_listen_infos_;
#endif // CINATRA_ENABLE_HTTPS

		std::string static_dir_;

		error_handler_t error_handler_;

		HTTPRouter router_;

		app_ctx_container_t app_container_;
	};

	using SimpleApp = Cinatra<>;
}
