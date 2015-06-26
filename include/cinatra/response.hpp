
#pragma once

#include "utils.hpp"

#include <boost/noncopyable.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/lexical_cast.hpp>

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
			return buffer_.sputn(data, len) == len;
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

		std::string get_header_str()
		{
			std::map<int, std::string>codes_detail_ =
			{
				{ 100, "HTTP/1.1 100 Continue\r\n" },
				{ 101, "HTTP/1.1 101 Switching Protocols\r\n" },
				{ 102, "HTTP/1.1 102 Processing\r\n" },
			
				{ 200, "HTTP/1.1 200 OK\r\n" },
				{ 201, "HTTP/1.1 201 Created\r\n" },
				{ 202, "HTTP/1.1 202 Accepted\r\n" },
				{ 203, "HTTP/1.1 203 Non-Authoritative Information\r\n" },
				{ 204, "HTTP/1.1 204 No Content\r\n" },
				{ 205, "HTTP/1.1 205 Reset Content\r\n" },
				{ 206, "HTTP/1.1 206 Partial Content\r\n" },
				{ 207, "HTTP/1.1 207 Muti Status\r\n" },

				{ 300, "HTTP/1.1 300 Multiple Choices\r\n" },
				{ 301, "HTTP/1.1 301 Moved Permanently\r\n" },
				{ 302, "HTTP/1.1 302 Moved Temporarily\r\n" },
				{ 303, "HTTP/1.1 303 See Other\r\n" },
				{ 304, "HTTP/1.1 304 Not Modified\r\n" },

				{ 400, "HTTP/1.1 400 Bad Request\r\n" },
				{ 401, "HTTP/1.1 401 Unauthorized\r\n" },
				{ 402, "HTTP/1.1 402 Payment Required\r\n" },
				{ 403, "HTTP/1.1 403 Forbidden\r\n" },
				{ 404, "HTTP/1.1 404 Not Found\r\n" },

				{ 500, "HTTP/1.1 500 Internal Server Error\r\n" },
				{ 501, "HTTP/1.1 501 Not Implemented\r\n" },
				{ 502, "HTTP/1.1 502 Bad Gateway\r\n" },
				{ 503, "HTTP/1.1 503 Service Unavailable\r\n" },
			};

			std::map<int, std::string>::const_iterator it = codes_detail_.find(status_code_);
			std::string header_str = it->second;
			header_str += "Server: cinatra/0.1\r\n";

			std::string date_str;
			time_t last_time_t = time(0);
			tm my_tm;
#ifdef _MSC_VER
			gmtime_s(&my_tm, &last_time_t);
#else
			gmtime_r(&last_time_t, &my_tm);
#endif

			date_str.resize(100);

			size_t date_str_sz = strftime(&date_str[0], 99, "%a, %d %b %Y %H:%M:%S GMT", &my_tm);
			date_str.resize(date_str_sz);
			header_str += "Date: ";
			header_str += date_str;
			header_str += "\r\n";

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
