
#pragma once

#include <cinatra/session_container.hpp>
#include <cinatra/utils.hpp>

#include <string>
#include <vector>

// 也不知道M$定义DELETE这个宏作甚...
#ifdef DELETE
#undef DELETE
#endif

namespace cinatra
{
	inline CaseMap query_parser(const std::string& data)
	{
		return kv_parser<std::string::const_iterator, CaseMap, '=', '&'>(data.begin(), data.end(), false);
	}

	inline CaseMap cookie_parser(const std::string& data)
	{
		return kv_parser<std::string::const_iterator, CaseMap, '=', ';'>(data.begin(), data.end(), true);
	}


	class Request
	{
	public:
		Request()
			:method_(method_t::UNKNOWN)
		{

		}

		Request(const std::string& url, const std::string& body,
			const std::string& method, const std::string& path,
			const CaseMap& query_map, const NcaseMultiMap& header)
			:url_(url), body_(body), path_(path),
			query_(query_map), header_(header), cookie_(cookie_parser(raw_cookie()))
		{
			if (boost::iequals(method, "GET"))
			{
				method_ = method_t::GET;
			}
			else if (boost::iequals(method, "PUT"))
			{
				method_ = method_t::PUT;
			}
			else if (boost::iequals(method, "POST"))
			{
				method_ = method_t::POST;
			}
			else if (boost::iequals(method, "DELETE"))
			{
				method_ = method_t::DELETE;
			}
			else if (boost::iequals(method, "TRACE"))
			{
				method_ = method_t::TRACE;
			}
			else if (boost::iequals(method, "CONNECT"))
			{
				method_ = method_t::CONNECT;
			}
			else if (boost::iequals(method, "HEAD"))
			{
				method_ = method_t::HEAD;
			}
			else if (boost::iequals(method, "OPTIONS"))
			{
				method_ = method_t::OPTIONS;
			}
			else
			{
				method_ = method_t::UNKNOWN;
			}
		}

		enum class method_t
		{
			UNKNOWN,
			GET,
			PUT,
			POST,
			DELETE,
			TRACE,
			CONNECT,
			HEAD,
			OPTIONS
		};

		const std::string& url() const { return url_; }
		method_t method() const { return method_; }

		const std::string& host() const
		{
			return header_.get_val("host");
		}
		const std::string& body() const
		{
			return body_;
		}

		const std::string& path() const { return path_; }
		const CaseMap& query() const { return query_; }
		const NcaseMultiMap&  header() const { return header_; }

		int content_length() const 
		{
			if (header_.get_count("Content-Length") == 0)
			{
				return 0;
			}

			// 这里可以不用担心cast抛异常，要抛异常在解析http header的时候就抛了.
			return boost::lexical_cast<int>(header_.get_val("Content-Length"));
		}

		const std::string& raw_cookie() const
		{
			return header_.get_val("Cookie");
		}

		const CaseMap& cookie() const
		{
			return cookie_;
		}

		Session& session()
		{
			return *session_;
		}
	private:
		friend class Connection;
		void set_session(session_ptr_t session)
		{
			session_ = session;
		}

		std::string url_;
		std::string body_;
		method_t method_;
		std::string path_;
		CaseMap query_;
		NcaseMultiMap header_;
		CaseMap cookie_;

		session_ptr_t session_;
	};
}
