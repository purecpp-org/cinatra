
#pragma once

#include "utils.hpp"

#include <boost/noncopyable.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

#include <functional>
#include <cassert>
#include <map>
#include <sstream>
#include <time.h>

namespace cinatra
{
	class Connection;
	class Response : boost::noncopyable
	{
	public:
		Response()
			:status_code_(200),
			version_major_(1),
			version_minor_(0),
			is_complete_(false),
			is_chunked_encoding_(false),
			has_chunked_encoding_header_(false)
		{}

		NcaseMultiMap header;

		bool write(const char* data, std::size_t len)
		{
			assert(!is_chunked_encoding_);
			assert(!is_complete_);
			if (is_chunked_encoding_ && is_complete_)
			{
				return false;
			}
			return size_t(buffer_.sputn(data, len)) == len;
		}

		bool write(const std::string& text)
		{
			return write(text.data(), text.size());
		}

		bool direct_write(const char* data, std::size_t len)
		{
			assert(buffer_.size() == 0);
			assert(!is_complete_);
			if (buffer_.size() != 0 || is_complete_)
			{
				return false;
			}

			is_chunked_encoding_ = true;

			boost::asio::streambuf buf;
			if (!has_chunked_encoding_header_)
			{
				header.add("Transfer-Encoding", "chunked");
				has_chunked_encoding_header_ = true;
				std::string str = get_header_str();
				buf.sputn(str.data(), str.size());
			}

			//要发送数据的长度
			std::stringstream ss;
			ss << std::hex << len;
			std::string str;
			ss >> str;
			str += "\r\n";
			buf.sputn(str.data(), str.size());
			buf.sputn(data, len);
			buf.sputn("\r\n", 2);

			boost::asio::const_buffer b = buf.data();
			const char* p = boost::asio::buffer_cast<const char*>(b);
			return direct_write_func_(p,buf.size());
		}

		bool direct_write(const std::string& text)
		{
			return direct_write(text.data(), text.size());
		}

		bool end(const char* data, std::size_t len)
		{
			return write(data, len) && end();
		}

		bool end(const std::string& text)
		{
			return end(text.data(), text.size());
		}

		bool end()
		{
			assert(!is_complete_);
			if (is_complete_)
			{
				return false;
			}

			if (is_chunked_encoding_)
			{
				if (!direct_write_func_("0\r\n\r\n", 5))
				{
					return false;
				}
			}

			is_complete_ = true;
			return true;
		}

		void set_status_code(int code)
		{
			status_code_ = code;
		}

		void set_version(int major, int minor)
		{
			version_major_ = major;
			version_minor_ = minor;
		}

		std::string get_header_str()
		{
			auto s = status_header(status_code_);
			std::string header_str = boost::str(
				boost::format(
				"HTTP/%d.%d %d %s\r\n"	//http version and status.
				"Server: cinatra/0.1\r\n"
				"Date: %s\r\n"
				)
				% version_major_ %version_minor_ % s.first % s.second
				% date_str(time(NULL))
				);

			if (!is_chunked_encoding_)
			{
				header_str += "Content-Length: ";
				header_str += boost::lexical_cast<std::string>(buffer_.size());
				header_str += "\r\n";
			}

			for (auto iter : header.get_all())
			{
				header_str += iter.first;
				header_str += ": ";
				header_str += iter.second;
				header_str += "\r\n";
			}

			header_str += "\r\n";

			return header_str;
		}
	private:
		friend Connection;
		int status_code_;
		int version_major_;
		int version_minor_;
		std::function < bool(const char*, std::size_t) >
			direct_write_func_;
		bool is_complete_;
		boost::asio::streambuf buffer_;

		// 是否是chunked编码.
		bool is_chunked_encoding_;
		// 是否已经在header中添加了Transfer-Encoding: chunked.
		bool has_chunked_encoding_header_;

	};
}
