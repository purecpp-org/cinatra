
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
			listen_addr_ = address;
			listen_port_ = port;
			return *this;
		}

		Cinatra& listen(const std::string& port)
		{
			listen_addr_ = "0.0.0.0";
			listen_port_ = port;
			return *this;
		}

		Cinatra& listen(const std::string& address, unsigned short port)
		{
			listen_addr_ = address;
			listen_port_ = boost::lexical_cast<std::string>(port);
			return *this;
		}

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

#ifdef CINATRA_ENABLE_HTTPS
		Cinatra& https_config(const HttpsConfig& cfg)
		{
			config_ = cfg;
			return *this;
		}
#endif // CINATRA_ENABLE_HTTPS

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
				.set_error_handler([this](int code, const std::string& msg, Request& req, Response& res)
			{
				LOG_DBG << "Handle error:" << code << " " << msg << " with path " << req.path();
				if (error_handler_
					&& error_handler_(code,msg,req,res))
				{
					return true;
				}

				LOG_DBG << "In defaule error handler";
				std::string html;
				auto s = status_header(code);
				html = "<html><head><title>" + s.second + "</title></head>";
				html += "<body>";
				html += "<h1>" + boost::lexical_cast<std::string>(s.first) + " " + s.second + " " + "</h1>";
				if (!msg.empty())
				{
					html += "<br> <h2>Message: " + msg + "</h2>";
				}
				html += "</body></html>";

				res.set_status_code(s.first);

				res.write(html);

				return true;
			})
				.static_dir(static_dir_)
#ifdef CINATRA_ENABLE_HTTPS
				.https_config(config_)
#endif // CINATRA_ENABLE_HTTPS
				.listen(listen_addr_, listen_port_)
				.run();
		}

	private:
		AOP<Aspect...> aop_;

#ifndef CINATRA_SINGLE_THREAD
		int num_threads_ = 1;
#else
		int num_threads_ = std::thread::hardware_concurrency();
#endif // CINATRA_SINGLE_THREAD

		std::string listen_addr_;
		std::string listen_port_;
		std::string static_dir_;
#ifdef CINATRA_ENABLE_HTTPS
		HttpsConfig config_;
#endif // CINATRA_ENABLE_HTTPS

		error_handler_t error_handler_;

		HTTPRouter router_;

		app_ctx_container_t app_container_;
	};

	using SimpleApp = Cinatra<>;
}
