
#pragma once

#include <cinatra/request.hpp>
#include <cinatra/response.hpp>
#include <cinatra/utils.hpp>

#include <boost/lexical_cast.hpp>

#include <string>

namespace cinatra
{
	struct RequestCookie
	{
		class Context
		{
		public:
			Context(const std::string& raw_cookie)
				:cookie_(RequestCookie::cookie_parser(raw_cookie))
			{
			}

			const std::string& get(const std::string& key) const
			{
				return cookie_.get_val(key);
			}

			std::vector<std::pair<std::string, std::string>> 
				get_all() const
			{
				return cookie_.get_all();
			}

			bool has_cookie(const std::string& key) const
			{
				return cookie_.has_key(key);
			}

		private:
			CaseMap cookie_;
		};

		void before(Request& req, Response& res)
		{
			req.add_context(Context(req.header().get_val("Cookie")));
		}

		void after(Request& /*req*/, Response& /*res*/)
		{
		}

	private:
		static inline CaseMap cookie_parser(const std::string& data)
		{
			return kv_parser<std::string::const_iterator, CaseMap, '=', ';'>(data.begin(), data.end(), true);
		}
	};


	struct ResponseCookie 
	{
		class Context
		{
		public:
			Context& new_cookie()
			{
				jar_.push_back(cookie_t());
				return *this;
			}
			Context& add(const std::string& key, const std::string& val)
			{
				auto tmp_k = encode(key);
				auto tmp_v = encode(val);
				jar_.back().kv.add(tmp_k, tmp_v);
				return *this;
			}
			Context& http_only(bool only = true)
			{
				jar_.back().http_only = only;
				return *this;
			}
			Context& expires(time_t t)
			{
				jar_.back().expires = t;
				return *this;
			}
			Context& domain(const std::string& domain_str)
			{
				jar_.back().domain = domain_str;
				return *this;
			}
			Context& path(const std::string& path_str)
			{
				jar_.back().path = path_str;
				return *this;
			}
			Context& secure(bool s = true)
			{
				jar_.back().secure = s;
				return *this;
			}
			Context& max_age(int seconds)
			{
				jar_.back().max_age = seconds;
				return *this;
			}

			std::vector<std::string> to_strings() const
			{
				std::vector<std::string> cookies;
				for (auto& cookie : jar_)
				{
					std::string cookie_str;

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

					cookies.push_back(cookie_str);
				}

				return cookies;
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

		void before(Request& req, Response& res)
		{
			req.add_context(Context());
		}

		void after(Request& req, Response& res)
		{
			auto const & ctx = req.get_context<ResponseCookie>();
 			for (auto const & it : ctx.to_strings())
 			{
 				res.header.add("Set-Cookie", it);
 			}
		}

	};
}