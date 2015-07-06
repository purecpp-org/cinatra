#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <boost/algorithm/cxx11/copy_if.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include "utils.hpp"

namespace cinatra 
{
	class cookies
	{
	public:
		explicit cookies(const std::string& domain = _EMPTY_STRING
			, const std::string& path = _EMPTY_STRING
			, bool httponly = false
			, bool secure = false)
			: domain_(domain)
			, path_(path)
			, httponly_(httponly)
			, secure_(secure)
		{}

		cookies(const cookies&& cookie)
			: cookies(std::move(cookie))
		{}

		// 解析cookie字符串生成cookie键值对
		void from_string(const std::string& cookie)
		{
			parse_cookie_string(cookie);
		}

		void set_domain(const std::string& domain)
		{
			domain_ = domain;
		}

		void set_path(const std::string& path)
		{
			path_ = path;
		}

		void set_secure(bool secure)
		{
			secure_ = secure;
		}

		void set_httponly(bool httponly)
		{
			httponly_ = httponly;
		}

		void set_expire(time_t t)
		{
			expires_ = t;
		}

		// 添加或更新一个（name， value）对
		void add(const std::string& name, const std::string& value)
		{
			if (name.empty()) return;//what are you弄啥咧 
			http_cookies_.add(name, value);
		}

		bool has_name(const std::string& name)
		{
			return http_cookies_.has_key(name);
		}

		// 获取指定name的value
		const std::string& get_value(const std::string& name) const
		{
			return http_cookies_.get_val(name);
		}

		const std::string& operator[](const std::string& name) const
		{
			return http_cookies_.get_val(name);
		}

		// 移除指定的name value
		void remove_name(const std::string& name)
		{
			http_cookies_.remove_key(name);
		}

		// 清除所有cookie
		void clear() {
			http_cookies_.clear();
		}

		//返回cookie串
		const std::string operator()() const
		{
			return (*this)();
		}

		std::string operator()()
		{
			std::string cookie_string = _EMPTY_STRING;
			auto vec_temp = http_cookies_.get_all();
			for (auto iter = vec_temp.cbegin()
				; iter != vec_temp.cend(); ++iter)
			{
				cookie_string.append(iter->first + "=" + iter->second);
				if (iter != vec_temp.cend())
					cookie_string.append("; ");
			}

			if (!domain_.empty())
				cookie_string.append("; domain=" + domain_);
			if (!path_.empty())
				cookie_string.append("; path=" + path_);
			if (!http_cookies_.has_key("expires"))
			{
				std::string expires;
				if (expires_ == 0)
					expires = gmtTime(time(NULL));
				else
					expires = gmtTime(expires_);
				if (!expires.empty())
					cookie_string.append("; expires=" + expires);
			}
				
			if (secure_)
				cookie_string.append("; secure");
			if (httponly_)
				cookie_string.append("; HttpOnly");

			return cookie_string;
		}

	private:
		// 解析cookie字符串
		// 比如：logged_in=no; domain=.github.com; path=/; expires=Fri, 06 Jul 2035 00:36:34 -0000; secure; HttpOnly
		void parse_cookie_string(const std::string& cookie)
		{
			CaseMap newMap = kv_parser<std::string::const_iterator, CaseMap, '=', ';'>
				(cookie.cbegin(), cookie.cend(), true);
			if (newMap.has_key("secure")) 
			{
				secure_ = true;
				newMap.remove_key("secure");
			}

			if (newMap.has_key("HttpOnly"))
			{
				httponly_ = true;
				newMap.remove_key("HttpOnly");
			}
			http_cookies_.insert(newMap.cbegin(), newMap.cend());
		}

		// time_t to cookie time 
		inline std::string gmtTime(time_t last_time_t)
		{
			std::string str;
			tm my_tm;
#ifdef _MSC_VER
			gmtime_s(&my_tm, &last_time_t);
#else
			gmtime_r(&last_time_t, &my_tm);
#endif

			str.resize(100);

			size_t date_str_sz = strftime(&str[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &my_tm);
			str.resize(date_str_sz);

			return str;
		}

	private:
		bool httponly_;
		bool secure_;
		std::string domain_;
		std::string path_;
		time_t expires_{ 0 };
		CaseMap http_cookies_;
	};

}