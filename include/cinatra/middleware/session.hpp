
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
			auto& res_cookie = ctx.get_req_ctx<ResponseCookie>();

			ctx.add_req_ctx(Context{ req_cookie , res_cookie, sessions_, mutex_});
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
			Context(RequestCookie::Context& req_cookie,
				ResponseCookie::Context& res_cooike,
				std::unordered_map<std::string, std::shared_ptr<SessionMap>>& sm2, std::mutex& mutex)
				:req_cookie_(req_cookie), res_cooike_(res_cooike), sm2_(sm2), mutex_(mutex)
			{

			}

			template<typename T>
			void set(const std::string& key, T const & val)
			{
				get_session_map()->last_used_time = std::time(nullptr);
				get_session_map()->m[key] = val;
			}
			bool has(const std::string& key)
			{
				if (!get_session_map(false))
				{
					return false;
				}

				get_session_map()->last_used_time = std::time(nullptr);
				return get_session_map()->m.find(key) != get_session_map()->m.end();
			}
			template<typename T>
			T& get(const std::string& key)
			{
				if (!get_session_map(false))
				{
					//TODO: 修改接口..
					static T empty;
					return empty;
				}

				get_session_map()->last_used_time = std::time(nullptr);
				auto it = get_session_map()->m.find(key);
				if (it == get_session_map()->m.end())
				{
					throw std::invalid_argument("key \"" + key + "\" not found.");
				}

				return boost::any_cast<typename std::decay<T>::type&>(it->second);
			}
			void del(const std::string& key)
			{
				if (!get_session_map(false))
				{
					return;
				}
				get_session_map()->last_used_time = std::time(nullptr);
				auto it = get_session_map()->m.find(key);
				if (it == get_session_map()->m.end())
				{
					throw std::invalid_argument("key \"" + key + "\" not found.");
				}

				get_session_map()->m.erase(it);
			}
		private:
			std::shared_ptr<SessionMap> get_session_map(bool create_if_no = true)
			{
				if (sm_)
				{
					std::cout << "1" << std::endl;
					return sm_;
				}


				std::cout << "2" << std::endl;
				// 获取session id
				std::string session_id = req_cookie_.get("CSESSIONID");
				if (session_id.empty() || sm2_.find(session_id) == sm2_.end())
				{
					if (!create_if_no)
					{
						return nullptr;
					}
					// ID为空则新建一个session
					session_id = new_sessionid();
					res_cooike_.new_cookie().add("CSESSIONID", session_id);
				}

				sm_ = sm2_.find(session_id)->second;
				return sm_;
			}

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
				sm2_.emplace(u_str, std::make_shared<SessionMap>());
				return u_str;
			}


			std::shared_ptr<SessionMap> sm_;
			RequestCookie::Context& req_cookie_;
			ResponseCookie::Context& res_cooike_;
			std::unordered_map<std::string, std::shared_ptr<SessionMap>>& sm2_;
			std::mutex& mutex_;
		};
	private:
#ifndef CINATRA_SINGLE_THREAD
		std::mutex mutex_;
#endif //CINATRA_SINGLE_THREAD
		std::unordered_map<std::string, std::shared_ptr<SessionMap>> sessions_;

		int life_cycle_ = 10 * 60;
 	};
}