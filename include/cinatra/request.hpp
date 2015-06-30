
#pragma once

#include <string>
#include <vector>
#include "utils.hpp"

namespace cinatra
{
	class Request
	{
	public:
		Request()
			:method_(method_t::UNKNOWN),
			is_body_parsed_(false)
		{

		}

		Request(
			const std::string& raw_url,
			const std::vector<char>& raw_body,
			const std::string& method,
			const std::string& path,
			const NcaseMultiMap& header
			)
			:raw_url_(raw_url),
			raw_body_(raw_body),
			path_(path),
			header_(header),
			is_body_parsed_(false)
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

		const std::string& raw_url() const { return raw_url_; }
		const std::vector<char> raw_body() const { return raw_body_; }
		method_t method() const { return method_; }
		const std::string& host() const
		{
			return header_.get_val("host");
		}
		const CaseMap& body()
		{
			if (!is_body_parsed_)
			{
				parsed_body_ = kv_parse(raw_body_.begin(),raw_body_.end());
				is_body_parsed_ = true;
			}

			return parsed_body_;
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

			// 这里可以不用担心cast抛异常，要抛异常在解析http header的时候就抛了
			return boost::lexical_cast<int>(header_.get_val("Content-Length"));
		}

		const std::string& raw_cookie()
		{
			return header_.get_val("Cookie");
		}
	private:
		std::string raw_url_;
		std::vector<char> raw_body_;
		method_t method_;
		std::string path_;
		CaseMap query_;
		NcaseMultiMap header_;
		CaseMap parsed_body_;
		bool is_body_parsed_;
	};
}