
#pragma once

#include <cinatra/utils.hpp>
#include <boost/lexical_cast.hpp>
#include <string>

namespace cinatra
{
	class cookie_builder
	{
	public:
		cookie_builder& new_cookie()
		{
			jar_.push_back(cookie_t());
			return *this;
		}
		cookie_builder& add(const std::string& key, const std::string& val)
		{
			auto tmp_k = encode(key);
			auto tmp_v = encode(val);
			jar_.back().kv.add(tmp_k, tmp_v);
			return *this;
		}
		cookie_builder& http_only(bool only = true)
		{
			jar_.back().http_only = only;
			return *this;
		}
		cookie_builder& expires(time_t t)
		{
			jar_.back().expires = t;
			return *this;
		}
		cookie_builder& domain(const std::string& domain_str)
		{
			jar_.back().domain = domain_str;
			return *this;
		}
		cookie_builder& path(const std::string& path_str)
		{
			jar_.back().path = path_str;
			return *this;
		}
		cookie_builder& secure(bool s = true)
		{
			jar_.back().secure = s;
			return *this;
		}
		cookie_builder& max_age(int seconds)
		{
			jar_.back().max_age = seconds;
			return *this;
		}
	private:
		std::string encode(const std::string& str)
		{
			std::string tmp;
			for (auto c : str)
			{
				if (c == ';')
				{
					tmp += "%3B";
				}
				else if (c == '%')
				{
					tmp += "%25";
				}
				else if (c == '=')
				{
					tmp += "%3D";
				}
				else
				{
					tmp.push_back(c);
				}
			}

			return tmp;
		}

		friend class Response;
		std::string to_string()
		{
			std::string cookie_str;

			for (auto& cookie : jar_)
			{
				cookie_str += "Set-Cookie: ";

				for (auto cookie_pair : cookie.kv)
				{
					cookie_str += cookie_pair.first;
					cookie_str += '=';
					cookie_str += cookie_pair.second;
					cookie_str += ';';
				}

				if (cookie.max_age != 0)
				{
					cookie_str += "Max-Age=";
					cookie_str += boost::lexical_cast<std::string>(cookie.max_age);
					cookie_str += ';';
				}
				if (!cookie.domain.empty())
				{
					cookie_str += "Domain=";
					cookie_str += cookie.domain;
					cookie_str += ';';
				}
				if (!cookie.path.empty())
				{
					cookie_str += "Path=";
					cookie_str += cookie.path;
					cookie_str += ';';
				}
				if (cookie.expires != 0)
				{
					cookie_str += "Expires=";
					cookie_str += cookie_date_str(cookie.expires);
					cookie_str += ';';
				}
				if (cookie.http_only)
				{
					cookie_str += "HttpOnly;";
				}
				if (cookie.secure)
				{
					cookie_str += "Secure;";
				}

				cookie_str.pop_back();
				cookie_str += "\r\n";
			}

			return cookie_str;
		}

		struct cookie_t
		{
			cookie_t()
				:max_age(0), http_only(false),
				expires(0), secure(false)
			{}
			int max_age;
			bool http_only;
			time_t expires;
			std::string domain;
			std::string path;
			bool secure;
			CaseMap kv;
		};
		std::vector<cookie_t> jar_;
	};
}

