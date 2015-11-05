
#pragma once

#include <boost/any.hpp>

#include <unordered_map>
#include <string>
#include <type_traits>
#include <typeindex>
#include <mutex>

namespace cinatra
{
	using app_ctx_container_t = std::unordered_map<std::string, boost::any>;

	class ContextContainer : boost::noncopyable
    {
	public:
		ContextContainer(app_ctx_container_t& app_container)
			:app_container_(app_container)
		{

		}

		template<typename T>
		void add_req_ctx(T const & val)
		{
			req_ctx_.emplace(std::type_index(typeid(typename std::decay<T>::type)), val);
		}

		template<typename T>
		bool has_req_ctx()
		{
			return req_ctx_.find(std::type_index(typeid(typename T::Context))) != req_ctx_.end();
		}

		template<typename T>
		typename T::Context& get_req_ctx()
		{
			auto it = req_ctx_.find(std::type_index(typeid(typename T::Context)));
			if (it == req_ctx_.end())
			{
				throw std::runtime_error("No such key.");
			}

			return boost::any_cast<typename T::Context&>(it->second);
		}

		template<typename T>
		T& get_app_ctx(const std::string& key)
		{
			auto it = app_container_.find(key);
			if (it == app_container_.end())
			{
				throw std::runtime_error("No such key.");
			}

			return boost::any_cast<T&>(it->second);
		}

		bool has_app_ctx(const std::string& key)
		{
			auto it = app_container_.find(key);
			return it != app_container_.end();
		}
		template<typename T>
		void set_app_ctx(const std::string& key, T const & val)
		{
#ifndef CINATRA_SINGLE_THREAD
			std::lock_guard<std::mutex> guard(app_con_mtx_);
#endif
			app_container_.emplace(key, val);
		}

	private:
		app_ctx_container_t& app_container_;
		std::unordered_map<std::type_index, boost::any>
			req_ctx_;
#ifndef CINATRA_SINGLE_THREAD
		std::mutex app_con_mtx_;
#endif
	};
}
