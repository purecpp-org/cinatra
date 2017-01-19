
#pragma once

#include <boost/any.hpp>
#include <boost/noncopyable.hpp>	// For boost::noncopyable or Use #include <boost/utility.hpp>

#include <unordered_map>
#include <string>
#include <type_traits>
#include <typeindex>
#include <mutex>

namespace cinatra
{
	class context_container : boost::noncopyable
    {
	public:
		template<typename T>
		void add_req_ctx(T const & val)
		{
			req_ctx_.emplace(std::type_index(typeid(typename std::decay<T>::type)), val);
		}

		template<typename T>
		bool has_req_ctx()
		{
			return req_ctx_.find(std::type_index(typeid(typename T::context_t))) != req_ctx_.end();
		}

		template<typename T>
		typename T::context_t& get_req_ctx()
		{
			auto it = req_ctx_.find(std::type_index(typeid(typename T::context_t)));
			if (it == req_ctx_.end())
			{
				throw std::runtime_error("No such key.");
			}

			return boost::any_cast<typename T::context_t&>(it->second);
		}

	private:
		std::unordered_map<std::type_index, boost::any> req_ctx_;
	};
}
