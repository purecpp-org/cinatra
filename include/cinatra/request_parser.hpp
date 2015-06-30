#pragma once

#include "utils.hpp"
#include "request.hpp"

#include <boost/lexical_cast.hpp>

#include <tuple>
#include <string>
#include <vector>

namespace cinatra
{
	/// Parser for incoming requests.
	class RequestParser
	{
	public:
		/// Construct ready to parse the request method.
		RequestParser()
			: state_(method_start)
		{
		}

		/// Result of parse.
		enum result_type { good, bad, indeterminate };

		/// Parse some data. The enum return value is good when a complete request has
		/// been parsed, bad if the data is invalid, indeterminate when more data is
		/// required. The InputIterator return value indicates how much of the input
		/// has been consumed.
		template <typename InputIterator>
		result_type parse(InputIterator begin, InputIterator end)
		{
			while (begin != end)
			{
				result_type result = consume(*begin++);
				if (result == good || result == bad)
				{
					std::string url = urldecode(url_);
					std::string::size_type qmak_pos = url.find("?");
					if (qmak_pos == std::string::npos)
					{
						path_ = url;
					}
					else
					{
						path_ = url.substr(0, qmak_pos);
						std::string quert_str = url.substr(qmak_pos + 1, url.size());
						query_ = kv_parse(quert_str.begin(), quert_str.end());
					}
					return result;
				}
			}
			return indeterminate;
		}

		bool check_version(int major, int minor) const
		{
			return version_major_ == major && version_minor_ == minor;
		}

		Request get_request()
		{
			return Request(url_, body_, method_, path_, query_, header_);
		}
	private:
		/// Handle the next character of input.
		result_type consume(char input)
		{
			switch (state_)
			{
			case method_start:
				if (!is_char(input) || is_ctl(input) || is_tspecial(input))
				{
					return bad;
				}
				else
				{
					state_ = method;
					method_.push_back(input);
					return indeterminate;
				}
			case method:
				if (input == ' ')
				{
					state_ = uri;
					return indeterminate;
				}
				else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
				{
					return bad;
				}
				else
				{
					method_.push_back(input);
					return indeterminate;
				}
			case uri:
				if (input == ' ')
				{
					state_ = http_version_h;
					return indeterminate;
				}
				else if (is_ctl(input))
				{
					return bad;
				}
				else
				{
					url_.push_back(input);
					return indeterminate;
				}
			case http_version_h:
				if (input == 'H')
				{
					state_ = http_version_t_1;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_t_1:
				if (input == 'T')
				{
					state_ = http_version_t_2;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_t_2:
				if (input == 'T')
				{
					state_ = http_version_p;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_p:
				if (input == 'P')
				{
					state_ = http_version_slash;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_slash:
				if (input == '/')
				{
					version_major_ = 0;
					version_minor_ = 0;
					state_ = http_version_major_start;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_major_start:
				if (is_digit(input))
				{
					version_major_ = version_major_ * 10 + input - '0';
					state_ = http_version_major;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_major:
				if (input == '.')
				{
					state_ = http_version_minor_start;
					return indeterminate;
				}
				else if (is_digit(input))
				{
					version_major_ = version_major_ * 10 + input - '0';
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_minor_start:
				if (is_digit(input))
				{
					version_minor_ = version_minor_ * 10 + input - '0';
					state_ = http_version_minor;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case http_version_minor:
				if (input == '\r')
				{
					state_ = expecting_newline_1;
					return indeterminate;
				}
				else if (is_digit(input))
				{
					version_minor_ = version_minor_ * 10 + input - '0';
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case expecting_newline_1:
				if (input == '\n')
				{
					state_ = header_line_start;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case header_line_start:
				if (input == '\r')
				{
					state_ = expecting_newline_3;
					return indeterminate;
				}
				else if (header_.size() != 0 && (input == ' ' || input == '\t'))
				{
					state_ = header_lws;
					return indeterminate;
				}
				else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
				{
					return bad;
				}
				else
				{
					current_header_key_.clear();
					current_header_val_.clear();
					current_header_key_.push_back(input);
					state_ = header_name;
					return indeterminate;
				}
			case header_lws:
				if (input == '\r')
				{
					state_ = expecting_newline_2;
					return indeterminate;
				}
				else if (input == ' ' || input == '\t')
				{
					return indeterminate;
				}
				else if (is_ctl(input))
				{
					return bad;
				}
				else
				{
					state_ = header_value;
					current_header_val_.push_back(input);
					return indeterminate;
				}
			case header_name:
				if (input == ':')
				{
					state_ = space_before_header_value;
					return indeterminate;
				}
				else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
				{
					return bad;
				}
				else
				{
					current_header_key_.push_back(input);
					return indeterminate;
				}
			case space_before_header_value:
				if (input != ' ')
				{
					if (is_ctl(input))
					{
						return bad;
					}
					else
					{
						current_header_val_.push_back(input);
					}
				}
				state_ = header_value;
				return indeterminate;
			case header_value:
				if (input == '\r')
				{
					header_.add(current_header_key_, current_header_val_);
					state_ = expecting_newline_2;
					return indeterminate;
				}
				else if (is_ctl(input))
				{
					return bad;
				}
				else
				{
					current_header_val_.push_back(input);
					return indeterminate;
				}
			case expecting_newline_2:
				if (input == '\n')
				{
					state_ = header_line_start;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case expecting_newline_3:
			{
				if (input != '\n')
				{
					return bad;
				}

				if (header_.get_count("content-length") !=0 )
				{
					content_length_ = boost::lexical_cast<unsigned int>(header_.get_val("content-length"));
					state_ = request_body;
					return indeterminate;
				}

				content_length_ = 0;
				return good;
			}
			case request_body:
			{
				body_.push_back(input);
				if (body_.size() < content_length_)
				{
					return indeterminate;
				}
				else
				{
					return good;
				}
			}
			default:
				return bad;
			}
		}

		/// Check if a byte is an HTTP character.
		static bool is_char(int c)
		{
			return c >= 0 && c <= 127;
		}

		/// Check if a byte is an HTTP control character.
		static bool is_ctl(int c)
		{
			return (c >= 0 && c <= 31) || (c == 127);
		}

		/// Check if a byte is defined as an HTTP tspecial character.
		static bool is_tspecial(int c)
		{
			switch (c)
			{
			case '(': case ')': case '<': case '>': case '@':
			case ',': case ';': case ':': case '\\': case '"':
			case '/': case '[': case ']': case '?': case '=':
			case '{': case '}': case ' ': case '\t':
				return true;
			default:
				return false;
			}
		}

		/// Check if a byte is a digit.
		static bool is_digit(int c)
		{
			return c >= '0' && c <= '9';
		}

		/// The current state of the parser.
		enum state
		{
			method_start,
			method,
			uri,
			http_version_h,
			http_version_t_1,
			http_version_t_2,
			http_version_p,
			http_version_slash,
			http_version_major_start,
			http_version_major,
			http_version_minor_start,
			http_version_minor,
			expecting_newline_1,
			header_line_start,
			header_lws,
			header_name,
			space_before_header_value,
			header_value,
			expecting_newline_2,
			expecting_newline_3,
			request_body
		} state_;

		unsigned int content_length_;

		std::string current_header_key_;
		std::string current_header_val_;

		std::string method_;
		std::string url_;
		std::string path_;
		int version_major_;
		int version_minor_;
		CaseMap query_;
		std::vector<char> body_;
		NcaseMultiMap header_;
	};
}


