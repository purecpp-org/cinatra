
#pragma once

#include <cinatra_http/request.h>
#include <cinatra_http/response.h>
#include <cinatra/context_container.hpp>
#include <cinatra/middleware/cookies.hpp>

#include <boost/any.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <unordered_map>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <ctime>


namespace cinatra
{
	class session
	{
	public:
		void before(request const& req, response& res, context_container& ctx)
		{
			{
				auto now = std::time(nullptr);
				std::lock_guard<std::mutex> _(mtx_);
				for (auto it = sessions_.begin(); it != sessions_.end();)
				{
					if (now - it->second.last_used_time > timeout_sec_)
					{
						it = sessions_.erase(it);
					}
					else
					{
						++it;
					}
				}
			}

			ctx.add_req_ctx(context_t(sessions_, mtx_, ctx));
		}

		void after(request const& req, response& res, context_container& ctx)
		{
		}

		void set_timeout(std::time_t seconds)
		{
			timeout_sec_ = seconds;
		}

		struct session_map_t
		{
			session_map_t()
				:last_used_time(std::time(nullptr))
			{}
			std::time_t last_used_time;
			std::unordered_map<std::string, boost::any> m;
		};

		class context_t
		{
		public:
			context_t(std::unordered_map<std::string, session_map_t>& sessions, std::mutex& mtx, context_container& ctx)
				:sessions_(sessions), mtx_(mtx), ctx_(ctx)
			{

			}

			template<typename T>
			T& get(std::string const& key)
			{
				auto sp = get_session_map_with_session_id();
				if (!sp)
				{
					throw std::invalid_argument("key " + key + " not found");
				}

				auto it = sp->m.find(key);
				if (it == sp->m.end())
				{
					throw std::invalid_argument("key " + key + " not found");
				}

				return boost::any_cast<T&>(it->second);
			}

			template<typename T>
			void add(std::string const& key, T const& val)
			{
				auto sp = get_session_map_with_add();
				sp->m.emplace(key, val);
			}

			bool has(std::string const& key)
			{
				auto sp = get_session_map_with_session_id();
				if (!sp)
				{
					return false;
				}

				return sp->m.find(key) != sp->m.end();
			}

			bool del(std::string const& key)
			{
				auto sp = get_session_map_with_session_id();
				if (!sp)
				{
					return false;
				}

				auto it = sp->m.find(key);
				if (it == sp->m.end())
				{
					return false;
				}

				sp->m.erase(it);
				return true;
			}
		private:
			std::string get_session_id()
			{
				auto& cookie = ctx_.get_req_ctx<cookies>();
				return cookie.get("CSESSIONID");
			}

			session_map_t* get_session_map()
			{
				return session_map_ptr_;
			}

			session_map_t* get_session_map_with_session_id()
			{
				if (session_map_ptr_)
				{
					session_map_ptr_->last_used_time = std::time(nullptr);
					return session_map_ptr_;
				}
				if (session_map_ptr_not_found_)
				{
					return nullptr;
				}

				auto& cookie = ctx_.get_req_ctx<cookies>();
				auto session_id = cookie.get("CSESSIONID");
				if (session_id.empty())
				{
					session_map_ptr_not_found_ = true;
					return nullptr;
				}

				std::lock_guard<std::mutex> _(mtx_);
				auto it = sessions_.find(session_id);
				if (it == sessions_.end())
				{
					session_map_ptr_not_found_ = true;
					return nullptr;
				}

				session_map_ptr_ = &it->second;

				session_map_ptr_->last_used_time = std::time(nullptr);
				return session_map_ptr_;
			}

			inline void itoh(int c, char& out1, char& out2)
			{
				unsigned char hexchars[] = "0123456789ABCDEF";
				out1 = hexchars[c >> 4];
				out2 = hexchars[c & 15];
			}

			session_map_t* get_session_map_with_add()
			{
				if (session_map_ptr_)
				{
					session_map_ptr_->last_used_time = std::time(nullptr);
					return session_map_ptr_;
				}

				auto& cookie = ctx_.get_req_ctx<cookies>();
				auto session_id = cookie.get("CSESSIONID");
				if (session_id.empty())
				{
					boost::uuids::uuid u = boost::uuids::random_generator()();
					for (auto c : u)
					{
						char out1, out2;
						itoh(c, out1, out2);
						session_id.push_back(out1);
						session_id.push_back(out2);
					}

					cookies::cookie_t c;
					c.add("CSESSIONID", session_id);
					cookie.add_cookie(c);
				}

				std::lock_guard<std::mutex> _(mtx_);
// 				sessions_.emplace(session_id_, session_map_t());
				session_map_ptr_ = &sessions_[session_id];
				return session_map_ptr_;
			}


			context_container& ctx_;
			session_map_t* session_map_ptr_ = nullptr;
			bool session_map_ptr_not_found_ = false;

			std::mutex& mtx_;
			std::unordered_map<std::string, session_map_t>& sessions_;
		};
	private:
		std::mutex mtx_;
		std::unordered_map<std::string, session_map_t> sessions_;
		std::time_t timeout_sec_ = 10 * 60;
	};







}