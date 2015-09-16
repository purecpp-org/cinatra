
#pragma once

#include <cinatra/request.hpp>
#include <cinatra/response.hpp>
#include <cinatra/context_container.hpp>
#include <cinatra/logging.hpp>
#include <cinatra/utils.hpp>
#include <cinatra/middleware/cookie.hpp>

#include <boost/any.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/asio.hpp>

#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <sstream>
#include <time.h>

#ifdef CINATRA_SINGLE_THREAD
#define CINATRA_UNIQUE_LOCK()  
#else
#define CINATRA_UNIQUE_LOCK() std::unique_lock<std::mutex> lock(mutex_)
#endif // CINATRA_SINGLE_THREAD


namespace cinatra
{
	class Session
	{
	public:
		void before(Request& req, Response& res,ContextContainer& ctx)
		{
			// 如果这里抛异常请检查是否添加了RequestCookie和ResponseCookie中间件
			auto& req_cookie = ctx.get_req_ctx<RequestCookie>();

  			// 获取session id
			std::string session_id = req_cookie.get("CSESSIONID");
 			if (session_id.empty())
 			{
 			 	// ID为空则新建一个session
				session_id = new_sessionid();
				auto& res_cookie = ctx.get_req_ctx<ResponseCookie>();
				res_cookie.new_cookie().add("CSESSIONID", session_id);
 			}

			auto it = sessions_.find(session_id);
			ctx.add_req_ctx(Context(it->second));
		}

		void after(Request& /*req*/, Response& /*res*/, ContextContainer& /*ctx*/)
		{
		}

		using session_map_t = std::unordered_map<std::string, boost::any>;

		class Context
		{
		public:
			Context(session_map_t& m)
				:m_(m)
			{}

			template<typename T>
			void add(const std::string& key, T const & val)
			{
				//CINATRA_UNIQUE_LOCK();
				m_.emplace(key, val);
			}
			bool has(const std::string& key)
			{
				return m_.find(key) != m_.end();
			}

			template<typename T>
			T& get(const std::string& key)
			{
				//CINATRA_UNIQUE_LOCK();
				auto it = m_.find(key);
				if (it == m_.end())
				{
					throw std::invalid_argument("key \"" + key + "\" not found.");
				}

				return boost::any_cast<typename std::decay<T>::type&>(it->second);
			}
		private:
			session_map_t& m_;
		};
	private:
#ifndef CINATRA_SINGLE_THREAD
		std::mutex mutex_;
#endif //CINATRA_SINGLE_THREAD

		std::string new_sessionid()
		{
			boost::uuids::uuid u = boost::uuids::string_generator()("{0123456789abcdef0123456789abcdef}");

			std::string u_str;
			for (auto c : u)
			{
				char out1, out2;
				itoh(c, out1, out2);

				u_str.push_back(out1);
				u_str.push_back(out2);
			}
			
			sessions_.emplace(u_str, session_map_t());
			return u_str;
		}

		std::unordered_map<std::string, session_map_t> sessions_;
 	};
}