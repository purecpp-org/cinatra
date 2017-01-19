#pragma once

#include <cinatra_http/request.h>
#include <cinatra_http/response.h>
#include <cinatra/context_container.hpp>
#include <cinatra_http/utils.h>

#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

#include <unordered_map>

namespace cinatra
{

	class cookies
	{
	private:
		static std::string encode(const std::string& str)
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

		static std::string cookie_date_str(time_t t)
		{
			std::string str;
			tm my_tm;
#ifdef _MSC_VER
			gmtime_s(&my_tm, &t);
#else
			gmtime_r(&t, &my_tm);
#endif

			str.resize(100);

			//Tue, 07-Jul-15 13:55:54 GMT
			size_t date_str_sz = strftime(&str[0], 99, "%a, %d-%b-%y %H:%M:%S GMT", &my_tm);
			str.resize(date_str_sz);

			return str;
		}

	public:
		class cookie_t
		{
		public:
			int max_age() const { return max_age_; }
			void set_max_age(int second) { max_age_ = second; }
			bool http_only() const { return http_only_; }
			void set_http_only(bool only) { http_only_ = only; }
			time_t expires() const { return expires_; }
			void set_expires(time_t t) { expires_ = t; }
			std::string const& domain() const { return domain_; }
			// XXX: domain和path是否需要url转义?
			void set_domain(std::string const& d) { domain_ = d; }
			std::string const& path() const { return path_; }
			void set_path(std::string const& p) { path_ = p; }
			bool secure() const { return secure_; }
			void set_secure(bool s) { secure_ = s; }

			void add(std::string const& key, std::string const& val)
			{
				kv_.emplace(encode(key), encode(val));
			}

			std::unordered_map<std::string, std::string> kv() const { return kv_; }
		private:
			int max_age_ = 0;
			bool http_only_ = false;
			time_t expires_ = 0;
			std::string domain_;
			std::string path_;
			bool secure_ = false;
			std::unordered_map<std::string, std::string> kv_;
		};

		void before(request const& req, response& res, context_container& ctx)
		{
			ctx.add_req_ctx(context_t(req));
		}

		void after(request const& req, response& res, context_container& ctx)
		{
			auto& cookies_ctx =  ctx.get_req_ctx<cookies>();
			for (auto const& str: cookies_ctx.get_res_cookies_str())
			{
				res.add_header("Set-Cookie", str);
			}
		}

		class context_t
		{
		public:
			context_t(request const& req)
				:req_(req)
			{

			}

			// XXX: 需要线程安全??
			const std::string& get(const std::string& key)
			{
				do_parse();
				auto it = req_cookies_.find(key);
				if (it == req_cookies_.end())
				{
					static const std::string s;
					return s;
				}

				return it->second;
			}

			std::unordered_map<std::string, std::string> const& get()
			{
				do_parse();
				return req_cookies_;
			}

			bool has_cookie(const std::string& key) const
			{
				return req_cookies_.find(key) != req_cookies_.end();
			}

			void add_cookie(cookie_t const& cookie)
			{
				res_cookies_.push_back(cookie);
			}

			std::vector<std::string> get_res_cookies_str()
			{
				std::vector<std::string> cookies;
				for (auto const& cookie : res_cookies_)
				{
					std::string cookie_str;

					for (auto cookie_pair : cookie.kv())
					{
						cookie_str += cookie_pair.first;
						cookie_str += '=';
						cookie_str += cookie_pair.second;
						cookie_str += ';';
					}

					if (cookie.max_age() != 0)
					{
						cookie_str += "Max-Age=";
						cookie_str += boost::lexical_cast<std::string>(cookie.max_age());
						cookie_str += ';';
					}
					if (!cookie.domain().empty())
					{
						cookie_str += "Domain=";
						cookie_str += cookie.domain();
						cookie_str += ';';
					}
					if (!cookie.path().empty())
					{
						cookie_str += "Path=";
						cookie_str += cookie.path();
						cookie_str += ';';
					}
					if (cookie.expires() != 0)
					{
						cookie_str += "Expires=";
						cookie_str += cookie_date_str(cookie.expires());
						cookie_str += ';';
					}
					if (cookie.http_only())
					{
						cookie_str += "HttpOnly;";
					}
					if (cookie.secure())
					{
						cookie_str += "Secure;";
					}

					cookie_str.pop_back();

					cookies.push_back(cookie_str);
				}

				return cookies;
			}

		private:
			void do_parse()
			{
				if (parsed_)
				{
					return;
				}

				parsed_ = true;
				for (auto val : req_.get_headers("Cookie", 6))
				{
					kv_parser(val.begin(), val.end(), req_cookies_, '=', ';', true, true);
				}
			}

			bool parsed_ = false;
			std::unordered_map<std::string, std::string> req_cookies_;
			std::vector<cookie_t> res_cookies_;


			request const& req_;
		};
	};





}