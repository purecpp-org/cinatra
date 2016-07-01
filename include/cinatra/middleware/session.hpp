
#pragma once

#include <cinatra/utils/logging.hpp>
#include <cinatra/utils/utils.hpp>
#include <cinatra/middleware/cookie.hpp>
#include <cinatra/http_server/request.hpp>
#include <cinatra/http_server/response.hpp>
#include <cinatra/context_container.hpp>

#include <boost/any.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/asio.hpp>

#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <atomic>
#include <sstream>
#include <ctime>

#ifdef CINATRA_SINGLE_THREAD
#define CINATRA_UNIQUE_LOCK(mtx)  
#else
#define CINATRA_UNIQUE_LOCK(mtx) std::unique_lock<std::mutex> lock(mtx)
#endif // CINATRA_SINGLE_THREAD


namespace cinatra
{
	class Session
	{
	public:
		Session() = default;
		Session(const Session&) = delete;
		Session& operator=(const Session&) = delete;


		void before(Request& /*req*/, Response& /*res*/,ContextContainer& ctx)
		{
			{
				CINATRA_UNIQUE_LOCK(mutex_);
				//检查所有session有没有超时的
				for (auto it = sessions_.begin(); it != sessions_.end();)
				{
					if (std::time(nullptr) - it->second->last_used_time >= life_cycle_)
					{
						it = sessions_.erase(it);
					}
					else
					{
						++it;
					}
				}
			}

			// 如果这里抛异常请检查是否添加了RequestCookie和ResponseCookie中间件,并且在session中间件的前面
			auto& req_cookie = ctx.get_req_ctx<RequestCookie>();

  			// 获取session id
			std::string session_id = req_cookie.get("CSESSIONID");
			if (session_id.empty() || sessions_.find(session_id) == sessions_.end())
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

		void set_session_life_circle(int seconds)
		{
			life_cycle_ = seconds;
		}

		struct SessionMap
		{
			SessionMap()
				:last_used_time(std::time(nullptr))
			{}
#ifndef CINATRA_SINGLE_THREAD
			std::mutex mutex;
#endif //CINATRA_SINGLE_THREAD
			std::atomic<std::time_t> last_used_time;
			std::unordered_map<std::string, boost::any> m;
		};

		class Context
		{
		public:
			Context(std::shared_ptr<SessionMap> sm)
				:sm_(sm)
			{}

			template<typename T>
			void set(const std::string& key, T const & val)
			{
				CINATRA_UNIQUE_LOCK(sm_->mutex);
				sm_->last_used_time = std::time(nullptr);
				sm_->m[key] = val;
			}
			bool has(const std::string& key)
			{
				sm_->last_used_time = std::time(nullptr);
				return sm_->m.find(key) != sm_->m.end();
			}
			template<typename T>
			T& get(const std::string& key)
			{
				CINATRA_UNIQUE_LOCK(sm_->mutex);
				sm_->last_used_time = std::time(nullptr);
				auto it = sm_->m.find(key);
				if (it == sm_->m.end())
				{
					throw std::invalid_argument("key \"" + key + "\" not found.");
				}

				return boost::any_cast<typename std::decay<T>::type&>(it->second);
			}
			void del(const std::string& key)
			{
				CINATRA_UNIQUE_LOCK(sm_->mutex);
				sm_->last_used_time = std::time(nullptr);
				auto it = sm_->m.find(key);
				if (it == sm_->m.end())
				{
					throw std::invalid_argument("key \"" + key + "\" not found.");
				}

				sm_->m.erase(it);
			}
		private:
			std::shared_ptr<SessionMap> sm_;
		};
	private:
#ifndef CINATRA_SINGLE_THREAD
		std::mutex mutex_;
#endif //CINATRA_SINGLE_THREAD

		std::string new_sessionid()
		{
			boost::uuids::uuid u = boost::uuids::random_generator()();
			const std::string u_str = boost::uuids::to_string(u);
			/*std::string u_str;
			for (auto c : u)
			{
				char out1, out2;
				itoh(c, out1, out2);

				u_str.push_back(out1);
				u_str.push_back(out2);
			}*/
			
			CINATRA_UNIQUE_LOCK(mutex_);
			sessions_.emplace(u_str, std::make_shared<SessionMap>());
			return u_str;
		}

		std::unordered_map<std::string, std::shared_ptr<SessionMap>> sessions_;

		int life_cycle_ = 10 * 60;
 	};
}