#pragma once

#include <cinatra/utils.hpp>
#include <cinatra/request.hpp>

#include <boost/algorithm/string/trim.hpp>

#include <algorithm>
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
			: state_(METHOD_START)
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
						query_ = query_parser(quert_str);
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

		bool is_version10() const
		{
			return version_major_ == 1 && version_minor_ == 0;
		}

		bool is_version11() const
		{
			return version_major_ == 1 && version_minor_ == 1;
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
			case METHOD_START:
				if (!is_char(input) || is_ctl(input) || is_tspecial(input))
				{
					return bad;
				}
				else
				{
					state_ = METHOD;
					method_.push_back(input);
					return indeterminate;
				}
			case METHOD:
				if (input == ' ')
				{
					state_ = URI;
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
			case URI:
				if (input == ' ')
				{
					state_ = HTTP_VERSION_H;
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
			case HTTP_VERSION_H:
				if (input == 'H')
				{
					state_ = HTTP_VERSUIN_T_1;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HTTP_VERSUIN_T_1:
				if (input == 'T')
				{
					state_ = HTTP_VERSION_T_2;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HTTP_VERSION_T_2:
				if (input == 'T')
				{
					state_ = HTTP_VERSION_P;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HTTP_VERSION_P:
				if (input == 'P')
				{
					state_ = HTTP_VERSION_SLASH;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HTTP_VERSION_SLASH:
				if (input == '/')
				{
					version_major_ = 0;
					version_minor_ = 0;
					state_ = HTTP_VERSION_MAJOR_START;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HTTP_VERSION_MAJOR_START:
				if (is_digit(input))
				{
					version_major_ = version_major_ * 10 + input - '0';
					state_ = HTTP_VERSION_MAJOR;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HTTP_VERSION_MAJOR:
				if (input == '.')
				{
					state_ = HTTP_VERSION_MINOR_START;
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
			case HTTP_VERSION_MINOR_START:
				if (is_digit(input))
				{
					version_minor_ = version_minor_ * 10 + input - '0';
					state_ = HTTP_VERSION_MINOR;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HTTP_VERSION_MINOR:
				if (input == '\r')
				{
					state_ = EXPECTING_NEWLINE_1;
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
			case EXPECTING_NEWLINE_1:
				if (input == '\n')
				{
					state_ = HEADER_LINE_START;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case HEADER_LINE_START:
				if (input == '\r')
				{
					state_ = EXPECTING_NEWLINE_3;
					return indeterminate;
				}
				else if (header_.size() != 0 && (input == ' ' || input == '\t'))
				{
					state_ = HEADER_LWS;
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
					state_ = HEADER_NAME;
					return indeterminate;
				}
			case HEADER_LWS:
				if (input == '\r')
				{
					state_ = EXPECTING_NEWLINE_2;
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
					state_ = HEADER_VALUE;
					current_header_val_.push_back(input);
					return indeterminate;
				}
			case HEADER_NAME:
				if (input == ':')
				{
					state_ = SPACE_BEFORE_HEADER_VALUE;
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
			case SPACE_BEFORE_HEADER_VALUE:
				if (input == ' ')
					return indeterminate;
				else if(is_ctl(input))
					return bad;
				current_header_val_.push_back(input);
				state_ = HEADER_VALUE;
				return indeterminate;
			case HEADER_VALUE:
				if (input == '\r')
				{
					auto tmp = boost::trim_right_copy_if(current_header_val_, [](const char c)->bool{return c == ' ';});
					header_.add(current_header_key_, std::move(tmp));
					state_ = EXPECTING_NEWLINE_2;
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
			case EXPECTING_NEWLINE_2:
				if (input == '\n')
				{
					state_ = HEADER_LINE_START;
					return indeterminate;
				}
				else
				{
					return bad;
				}
			case EXPECTING_NEWLINE_3:
			{
				if (input != '\n')
				{
					return bad;
				}

				if (header_.get_count("content-length") !=0 )
				{
					content_length_ = boost::lexical_cast<unsigned int>(header_.get_val("content-length"));
					if (content_length_ > 0)
					{
						state_ = REQUEST_BODY;
						return indeterminate;
					}

					return good;
				}

				content_length_ = 0;
				return good;
			}
			case REQUEST_BODY:
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
			METHOD_START,
			METHOD,
			URI,
			HTTP_VERSION_H,
			HTTP_VERSUIN_T_1,
			HTTP_VERSION_T_2,
			HTTP_VERSION_P,
			HTTP_VERSION_SLASH,
			HTTP_VERSION_MAJOR_START,
			HTTP_VERSION_MAJOR,
			HTTP_VERSION_MINOR_START,
			HTTP_VERSION_MINOR,
			EXPECTING_NEWLINE_1,
			HEADER_LINE_START,
			HEADER_LWS,
			HEADER_NAME,
			SPACE_BEFORE_HEADER_VALUE,
			HEADER_VALUE,
			EXPECTING_NEWLINE_2,
			EXPECTING_NEWLINE_3,
			REQUEST_BODY
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
		std::string body_;
		NcaseMultiMap header_;
	};
}


