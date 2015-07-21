
#pragma once

#include <boost/any.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <map>
#include <string>
#include <memory>
#include <sstream>
#include <mutex>

namespace cinatra
{
	class Session
	{
		std::mutex mutex_;
	public:
		template<typename T>
		void add(const std::string& key, T const & val)
		{
			std::unique_lock<std::mutex> lock(mutex_);
			kv_.emplace(key, val);
		}

		template<typename T>
		void set(const std::string& key, T const & val)
		{
			std::unique_lock<std::mutex> lock(mutex_);
			kv_[key] = val;
		}

		template<typename T>
		T get(const std::string& key)
		{
			auto it = kv_.find(key);
			if (it == kv_.end())
			{
				throw std::runtime_error("No such key.");
			}

			return boost::any_cast<T>(*it);
		}
		bool exists(const std::string& key)
		{
			auto it = kv_.find(key);
			return it != kv_.end();
		}
	private:
		std::map<std::string, boost::any> kv_;
	};

	using session_ptr_t = std::shared_ptr<Session>;
	class SessionContainer
	{
		std::mutex mutex_;
	public:
		std::string new_session()
		{
			std::unique_lock<std::mutex> lock(mutex_);
			boost::uuids::uuid u = boost::uuids::string_generator()("{0123456789abcdef0123456789abcdef}");
			std::stringstream ss;
			ss << u;
			session_container_.emplace(ss.str(), std::make_shared<Session>());
			return ss.str();
		}

		session_ptr_t get_container(const std::string& key)
		{
			std::unique_lock<std::mutex> lock(mutex_);
			auto it = session_container_.find(key);
			if (it == session_container_.end())
			{
				auto ptr(std::make_shared<Session>());
				session_container_.emplace(key, ptr);
				return ptr;
			}
			return it->second;
		}
	private:
		std::map<std::string, session_ptr_t> session_container_;
	};

}