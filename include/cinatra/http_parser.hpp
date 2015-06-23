
#pragma once

#include "request.hpp"
#include "http_parser_merged.h"
#include "utils.hpp"
#include <functional>
#include <cassert>

namespace cinatra
{
	class HTTPParser : public http_parser
	{
	public:
		HTTPParser()
		{
			reset();
			http_parser_init(this, HTTP_REQUEST);
		}

		bool feed(const char* data, size_t len)
		{
			static http_parser_settings settings =
			{
				&HTTPParser::on_message_begin,
				&HTTPParser::on_url,
				nullptr,
				&HTTPParser::on_header_field,
				&HTTPParser::on_header_value,
				&HTTPParser::on_headers_complete,
				&HTTPParser::on_body,
				&HTTPParser::on_message_complete
			};
			return http_parser_execute(this, &settings, data, len) == len;
		}

		bool is_completed(){ return is_completed_; }
		Request get_request()
		{
			assert(is_completed_);
			return Request{ raw_url_, raw_body_, method_, path_, query_, body_, header_ };
		}

	private:
		void reset()
		{
			header_status_ = 0;
			current_key_.clear();
			current_val_.clear();
			is_completed_ = false;
			raw_url_.clear();
			raw_body_.clear();
			method_ = HTTP_GET;
			path_.clear();
			query_.clear();
			body_.clear();
			header_.clear();
		}
		static int on_message_begin(http_parser* p)
		{
			HTTPParser* self = static_cast<HTTPParser*>(p);
			self->reset();
			return 0;
		}
		static int on_message_complete(http_parser* p)
		{
			HTTPParser* self = static_cast<HTTPParser*>(p);
			self->method_ = (http_method)p->method;
			std::string url = urldecode(self->raw_url_);

			std::string::size_type qmak_pos = url.find("?");
			if (qmak_pos == std::string::npos)
			{
				self->path_ = url;
			}
			else
			{
				self->path_ = url.substr(0, qmak_pos);
				std::string quert_str = url.substr(qmak_pos + 1, url.size());
				self->query_ = kv_parse(quert_str);
			}

			self->body_ = kv_parse(self->raw_body_);
			self->is_completed_ = true;
			return 0;
		}
		static int on_headers_complete(http_parser* p)
		{
			HTTPParser* self = static_cast<HTTPParser*>(p);
			self->header_.add(self->current_key_, self->current_val_);
			return 0;
		}

		static int on_url(http_parser* p, const char *at, size_t length)
		{
			HTTPParser* self = static_cast<HTTPParser*>(p);
			self->raw_url_.insert(self->raw_url_.end(), at, at + length);
			return 0;
		}
		static int on_header_field(http_parser* p, const char *at, size_t length)
		{
			HTTPParser* self = static_cast<HTTPParser*>(p);
			if (self->header_status_ == 0)
			{
				self->current_key_.insert(self->current_key_.end(), at, at + length);
			}
			else
			{
				self->header_.add(self->current_key_, self->current_val_);
				self->current_key_ = std::string(at, at + length);
				self->current_val_.clear();
				self->header_status_ = 0;
			}
			return 0;
		}
		static int on_header_value(http_parser* p, const char *at, size_t length)
		{
			HTTPParser* self = static_cast<HTTPParser*>(p);
			self->current_val_.insert(self->current_val_.end(), at, at + length);
			self->header_status_ = 1;
			return 0;
		}
		static int on_body(http_parser* p, const char *at, size_t length)
		{
			HTTPParser* self = static_cast<HTTPParser*>(p);
			self->raw_body_.insert(self->raw_body_.end(), at, at + length);
			return 0;
		}

		static int htoi(int c1, int c2)
		{
			int value;
			int c;

			c = c1;
			if (isupper(c))
				c = tolower(c);
			value = (c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10) * 16;

			c = c2;
			if (isupper(c))
				c = tolower(c);
			value += c >= '0' && c <= '9' ? c - '0' : c - 'a' + 10;

			return (value);
		}

		static std::string urldecode(const std::string &str_source)
		{
			std::string str_dest;
			for (auto iter = str_source.begin();
				iter != str_source.end(); ++iter)
			{
				if (*iter == '+')
				{
					str_dest.push_back(' ');
				}
				else if (*iter == '%'
					&& (str_source.end() - iter) >= 3
					&& isxdigit(*(iter + 1))
					&& isxdigit(*(iter + 2)))
				{
					str_dest.push_back(htoi(*(iter + 1), *(iter + 2)));
					iter += 2;
				}
				else
				{
					str_dest.push_back(*iter);
				}
			}

			return str_dest;
		}

		// 解析a=1&b=2&c=3这样的字符串,如果格式不正确返回空map
		static CaseMap kv_parse(const std::string& ky_str)
		{
			CaseMap result;
			std::string key, val;
			bool is_k = true;
			for (auto c : ky_str)
			{
				if (c == '&')
				{
					is_k = true;
					result.add(key, val);
					key.clear();
				}
				else if (c == '=')
				{
					is_k = false;
					val.clear();
				}
				else
				{
					if (is_k)
					{
						key.push_back(c);
					}
					else
					{
						val.push_back(c);
					}
				}
			}

			if (!is_k)
			{
				result.add(key, val);
			}
			return result;
		}

		//for parsing header fields
		int header_status_;
		std::string current_key_;
		std::string current_val_;
		bool is_completed_;


		std::string raw_url_;
		std::string raw_body_;
		http_method method_;
		std::string path_;
		CaseMap query_;
		CaseMap body_;
		NcaseMultiMap header_;
	};

}
