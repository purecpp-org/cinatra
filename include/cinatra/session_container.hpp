
#pragma once

#include <cinatra/logging.hpp>
#include <cinatra/utils.hpp>

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

#ifdef SINGLE_THREAD
#define UNIQUE_LOCK()  
#else
#define UNIQUE_LOCK() std::unique_lock<std::mutex> lock(mutex_)
#endif // SINGLE_THREAD


namespace cinatra
{
	class Session
	{
		std::mutex mutex_;
	public:
		Session()
			:last_used_time_(time(NULL))
		{}
		template<typename T>
		void add(const std::string& key, T const & val)
		{
			update_time();
			UNIQUE_LOCK();
			kv_.emplace(key, val);
		}

		template<typename T>
		void set(const std::string& key, T const & val)
		{
			update_time();
			UNIQUE_LOCK();
			kv_[key] = val;
		}

		template<typename T>
		T get(const std::string& key)
		{
			update_time();
			auto it = kv_.find(key);
			if (it == kv_.end())
			{
				throw std::runtime_error("No such key.");
			}

			return boost::any_cast<T>(it->second);
		}
		bool exists(const std::string& key)
		{
			update_time();
			auto it = kv_.find(key);
			return it != kv_.end();
		}

		void update_time()
		{
			last_used_time_ = time(NULL);
		}
		time_t get_last_used_time()
		{
			return last_used_time_;
		}
	private:
		std::map<std::string, boost::any> kv_;
		time_t last_used_time_;
	};

	using session_ptr_t = std::shared_ptr<Session>;
	class SessionContainer
	{
		std::string u_str_;
		std::mutex mutex_;
	public:
		SessionContainer(boost::asio::io_service& service)
			:timer_(service)
		{
			start_timer();
			u_str_.reserve(36);
		}
		std::string new_session()
		{
			UNIQUE_LOCK();
			boost::uuids::uuid u = boost::uuids::string_generator()("{0123456789abcdef0123456789abcdef}");

			//std::stringstream ss;
			//ss << u;
			//session_container_.emplace(ss.str(), std::make_shared<Session>());
			//return ss.str();
			u_str_.clear();
 			for (auto c : u)
 			{
 				char out1, out2;
 				itoh(c, out1, out2);
 
 				u_str_.push_back(out1);
 				u_str_.push_back(out2);
 			}
 			session_container_.emplace(u_str_, std::make_shared<Session>());
 			return u_str_;
		}

		session_ptr_t get_container(const std::string& key)
		{
			auto it = session_container_.find(key);
			if (it == session_container_.end())
			{
				auto ptr(std::make_shared<Session>());
				UNIQUE_LOCK();
				session_container_.emplace(key, ptr);
				return ptr;
			}
			it->second->update_time();
			return it->second;
		}
	private:
		std::map<std::string, session_ptr_t> session_container_;

		boost::asio::deadline_timer timer_;

		void start_timer()
		{
			//2分钟检测一次.
			timer_.expires_from_now(boost::posix_time::seconds(2*60));
			timer_.async_wait([this](const boost::system::error_code& ec)
			{
				if (ec == boost::asio::error::operation_aborted)
				{
					return;
				}
				if (ec)
				{
					LOG_DBG << "Session timer error: " << ec.message();
				}

				time_t now = time(NULL);
				UNIQUE_LOCK();

				for (auto it = session_container_.begin();
					it != session_container_.end();)
				{
					if (now - it->second->get_last_used_time() >= 5 * 60)	//超过5分钟没有使用过就超时删除
					{
						it = session_container_.erase(it);
						continue;
					}

					++it;
				}

				start_timer();
			});
		}
	};

}
