
#pragma once

#include <cinatra/utils/utils.hpp>

#include <exception>
#include <string>

#if defined(_MSC_VER) && _MSC_VER < 1900
#define CINATRA_NOEXCEPT
#else
#define CINATRA_NOEXCEPT noexcept
#endif

namespace cinatra
{
	class HttpError : public std::exception
	{
	public:
		HttpError(int code)
		{
			auto h = status_header(code);
			code_ = h.first;
			description_ = h.second;
		}
		HttpError(int code, const std::string& msg)
		{
			auto h = status_header(code);
			code_ = h.first;
			description_ = h.second;
			msg_ = msg;
		}
		HttpError(int code,const std::string& description, const std::string& msg)
			:code_(code),description_(description), msg_(msg)
		{}

		virtual const char * what() const CINATRA_NOEXCEPT override
		{
			std::string str = "HttpError, status code: " + boost::lexical_cast<std::string>(code_)+";";
			if (!description_.empty())
			{
				str += "Description: " + description_ + ";";
			}
			if (!msg_.empty())
			{
				str += "Message: " + msg_ + ";";
			}
			return str.c_str();
		}

		int get_code() const { return code_; }
		std::string get_description() const { return description_; }
		std::string get_msg() const { return msg_; }
	private:
		int code_;
		std::string description_;
		std::string msg_;
	};

}