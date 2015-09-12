
#pragma once

#include <cinatra/http_server.hpp>
#include <cinatra/http_router.hpp>
#include <cinatra/request.hpp>
#include <cinatra/response.hpp>
#include <cinatra/logging.hpp>
#include <cinatra/aop.hpp>
#include <cinatra/body_parser.hpp>

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
		T& get(const std::string& key)
		{
			auto it = app_container_.find(key);
			if (it == app_container_.end())
			{
				throw std::runtime_error("No such key.");
			}

			return boost::any_cast<T>(it->second);
		}

		bool has(const std::string& key)
		{
			auto it = app_container_.find(key);
			return it != app_container_.end();
		}
		template<typename T>
		void set(const std::string& key, T const & val)
		{
#ifndef CINATRA_SINGLE_THREAD
			std::lock_guard<std::mutex> guard(app_con_mtx_);
#endif
			app_container_.emplace(key, val);
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

			s.set_request_handler([this](Request& req, Response& res)
			{
				return Invoke<sizeof...(Aspect)>(res, &Cinatra::dispatch, this, req, res);
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
		template<size_t I, typename Func, typename Self, typename... Args>
		typename std::enable_if<I == 0, bool>::type Invoke(Response& /* res */, Func&&f, Self* self, Args&&... args)
		{
			return (*self.*f)(std::forward<Args>(args)...);
		}

		template<size_t I, typename Func, typename Self, typename... Args>
		typename std::enable_if < (I > 0), bool > ::type Invoke(Response& res, Func&& /* f */, Self* /* self */, Args&&... args)
		{
			return invoke<Aspect...>(res, &Cinatra::dispatch, this, args...);
		}
		
		bool dispatch(Request& req, Response& res)
		{
			return router_.dispatch(req, res);
		}

	private:
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

		std::map<std::string, boost::any> app_container_;
#ifndef CINATRA_SINGLE_THREAD
		std::mutex app_con_mtx_;
#endif
	};

	using SimpleApp = Cinatra<>;
}
