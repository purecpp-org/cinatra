
#include <cinatra_http/request.h>
#include <cinatra_http/utils.h>

#include <boost/lexical_cast/try_lexical_convert.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/regex.hpp>
#include <cstdlib>

namespace cinatra
{

	request::request()
		:buffer_{static_cast<char*>(std::malloc(8192)), 0, 8192}
	{

	}

	request::~request()
	{
		if (multipart_parser_)
		{
			multipart_parser_free(multipart_parser_);
		}
		std::free(buffer_.buffer);
	}

	int request::parse_header(std::size_t last_len)
    {
        num_headers_ = sizeof(headers_) / sizeof(headers_[0]);
        header_size_ = phr_parse_request(buffer_.buffer, buffer_.size, &method_,
                                         &method_len_, &url_, &url_len_,
                                         &minor_version_, headers_, &num_headers_, last_len);

        auto content_length = get_header("content-length", 14);

        if (content_length.empty()
            || !boost::conversion::try_lexical_convert<size_t>(content_length.data(), content_length.size(), body_len_))
        {
            body_len_ = 0;
        }

		if (header_size_ > 0)
		{
			http_parser_url_init(&url_info_);
			if (http_parser_parse_url(url_, url_len_, false, &url_info_) != 0)
			{
				return -1;
			}

			if (url_info_.field_set & (1 << UF_QUERY))
			{
				kv_parser(url_ + url_info_.field_data[UF_QUERY].off,
					url_ + url_info_.field_data[UF_QUERY].off + url_info_.field_data[UF_QUERY].len,
					queries_, '=', '&', true, false);
			}
		}

        return header_size_;
    }

	namespace parser
	{
		std::string get_content_type(std::string::iterator& begin, std::string::iterator end)
		{
			std::string content_type;
			for (; begin != end; ++begin)
			{
				char c = *begin;
				if (c == ';')
				{
					boost::algorithm::trim(content_type);
					++begin;
					return content_type;
				}
				content_type.push_back(c);
			}

			return content_type;
		}

		std::string get_pair(std::string::iterator& begin, std::string::iterator end)
		{
			return get_content_type(begin, end);
		}

		std::string get_pair_name(std::string::iterator& begin, std::string::iterator end)
		{
			std::string name;
			for (; begin != end; ++begin)
			{
				char c = *begin;
				if (c == '=')
				{
					boost::algorithm::trim(name);
					++begin;
					return name;
				}
				name.push_back(c);
			}

			return name;
		}
		std::string get_pair_value(std::string::iterator& begin, std::string::iterator end)
		{
			std::string value;
			bool has_first_quo = false;
			for (; begin != end; ++begin)
			{
				char c = *begin;
				if (c == '"')
				{
					if (!has_first_quo)
					{
						has_first_quo = true;
						continue;
					}
					boost::algorithm::trim(value);
					++begin;
					return value;
				}

				value.push_back(c);
			}

			return value;
		}

		request::form_parts_t::content_disposition_t parse_content_disposition(std::string str)
		{
			auto start = str.begin();
			auto end = str.end();
			request::form_parts_t::content_disposition_t content_disposition;
			content_disposition.content_type = get_content_type(start, end);
			while (start != end)
			{
				auto pair = get_pair(start, end);
				auto pair_start = pair.begin();
				auto pair_end = pair.end();
				auto name = get_pair_name(pair_start, pair_end);
				auto value = get_pair_value(pair_start, pair_end);
				content_disposition.pairs.emplace(name, value);
			}

			return content_disposition;
		}
	}

	bool request::parse_form_multipart()
	{
		if (!multipart_parser_)
		{
			// 获取boundary
			auto content_type = get_header("Content-Type", 12).to_string();
			if (content_type.empty())
			{
				return false;
			}

			auto pos = content_type.find(';');
			if (pos == std::string::npos)
			{
				return false;
			}

			pos = content_type.find("boundary", pos);
			pos += 8;
			if (pos == std::string::npos || pos >= content_type.size())
			{
				return false;
			}

			bool equ_found = false;
			for (;pos < content_type.size(); ++pos)
			{
				if (content_type[pos] == '=')
				{
					if (equ_found)
					{
						return false;
					}
					equ_found = true;
					continue;
				}

				if (content_type[pos] != ' ')
				{
					break;
				}
			}
			if (!equ_found)
			{
				return false;
			}

			auto boundary = "--" + content_type.substr(pos, content_type.size() - pos);

			// TODO:改为static const
			multipart_parser_settings_.on_part_data_begin = [](multipart_parser* p)
			{
				auto self = static_cast<request*>(multipart_parser_get_data(p));
				self->multipart_form_data_.emplace_back(form_parts_t{});
				return 0;
			};
			multipart_parser_settings_.on_header_field = [](multipart_parser* p, const char *at, size_t length)
			{
				auto self = static_cast<request*>(multipart_parser_get_data(p));
				auto& part = self->multipart_form_data_.back();
				if (part.state_ == 1)
				{
					if (iequal(part.curr_field_.data(), part.curr_field_.size(), "Content-Disposition", 19))
					{
						part.content_disposition_ = parser::parse_content_disposition(part.curr_value_);
					}

					part.meta_.emplace_back(part.curr_field_, part.curr_value_);
					part.state_ = 0;

					part.curr_field_.clear();
					part.curr_value_.clear();
				}
				
				part.curr_field_ += std::string(at, length);
				return 0;
			};
			multipart_parser_settings_.on_header_value = [](multipart_parser* p, const char *at, size_t length)
			{
				auto self = static_cast<request*>(multipart_parser_get_data(p));
				auto& part = self->multipart_form_data_.back();
				part.state_ = 1;
				part.curr_value_ += std::string(at, length);
				return 0;
			};
			multipart_parser_settings_.on_headers_complete = [](multipart_parser* p)
			{
				auto self = static_cast<request*>(multipart_parser_get_data(p));
				auto& part = self->multipart_form_data_.back();
				assert(part.state_ == 1);
				part.meta_.emplace_back(part.curr_field_, part.curr_value_);
				return 0;
			};
			multipart_parser_settings_.on_part_data = [](multipart_parser* p, const char *at, size_t length)
			{
				auto self = static_cast<request*>(multipart_parser_get_data(p));
				auto& part = self->multipart_form_data_.back();
				part.data_ += std::string(at, length);

				return 0;
			};

// 			multipart_parser_settings_.on_part_data_end = [](multipart_parser*)
// 			{
// 				std::cout << "*****************on_part_data_end********************************" << std::endl;
// 				return 0;
// 			};
// 			multipart_parser_settings_.on_body_end = [](multipart_parser*)
// 			{
// 				std::cout << "*****************on_body_end********************************" << std::endl;
// 				return 0;
// 			};

			multipart_parser_ = multipart_parser_init(boundary.c_str(), &multipart_parser_settings_);
			multipart_parser_set_data(multipart_parser_, this);
		}

		return multipart_parser_execute(multipart_parser_, body().data(), body().size()) != 0;//== body().size();
	}

	bool request::parse_form_urlencoded()
	{
		std::string key, val;
		bool is_k = true;

		auto body_str = body();
		for (auto it = body_str.begin(); it != body_str.end(); ++it)
		{
			char c = *it;
			if (c == '&')
			{
				is_k = true;
				urlencoded_form_data_.emplace(key, val);
				key.clear();
			}
			else if (c == '=')
			{
				is_k = false;
				val.clear();
			}
			else
			{
				if (c == '%')
				{
					++it;
					char c1 = *it;
					++it;
					char c2 = *it;
					c = htoi(c1, c2);
				}

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
			urlencoded_form_data_.emplace(key, val);
			return true;
		}

		return false;
	}

	boost::string_ref request::get_header(const char* name, size_t size) const
    {
        auto it = std::find_if(headers_, headers_ + num_headers_, [this, name, size](struct phr_header const& hdr)
        {
            return iequal(hdr.name, hdr.name_len, name, size);
        });

        if (it == headers_ + num_headers_)
        {
            return{};
        }

        return boost::string_ref(it->value, it->value_len);
    }

    std::vector<boost::string_ref> request::get_headers(const char* name, size_t size) const
    {
        std::vector<boost::string_ref> headers;
        for (std::size_t i = 0; i < num_headers_; ++i)
        {
            if (iequal(headers_[i].name, headers_[i].name_len, name, size))
            {
                headers.emplace_back(boost::string_ref(headers_[i].value, headers_[i].value_len));
            }
        }

        return headers;
    }

    std::vector<request::header_t> request::get_headers() const
    {
        std::vector<header_t> headers;
        for (std::size_t i = 0; i < num_headers_; ++i)
        {
            headers.emplace_back(header_t
            {
                 boost::string_ref(headers_[i].name, headers_[i].name_len),
                 boost::string_ref(headers_[i].value, headers_[i].value_len)
            });
        }

        return headers;
    }

    bool request::has_header(const char* name, size_t size) const
    {
        auto it = std::find_if(headers_, headers_ + num_headers_, [name, size](struct phr_header const& hdr)
        {
            return iequal(hdr.name, hdr.name_len, name, size);
        });

        return it != headers_ + num_headers_;
    }

    std::size_t request::headers_num(const char* name, size_t size) const
    {
        std::size_t num = 0;
        for (std::size_t i = 0; i < num_headers_; ++i)
        {
            if (iequal(headers_[i].name, headers_[i].name_len, name, size))
            {
                ++num;
            }
        }

        return num;
    }

    boost::string_ref request::get_header_cs(std::string const& name) const
    {
        auto it = std::find_if(headers_, headers_ + num_headers_, [&name](struct phr_header const& hdr)
        {
            return boost::string_ref(hdr.name, hdr.name_len) == name;
        });

        if (it == headers_ + num_headers_)
        {
            return{};
        }

        return boost::string_ref(it->value, it->value_len);
    }

    std::vector<boost::string_ref> request::get_headers_cs(std::string const& name) const
    {
        std::vector<boost::string_ref> headers;
        for (std::size_t i = 0; i < num_headers_; ++i)
        {
            if (boost::string_ref(headers_[i].name, headers_[i].name_len) == name)
            {
                headers.emplace_back(boost::string_ref(headers_[i].value, headers_[i].value_len));
            }
        }

        return headers;
    }

    bool request::has_header_cs(std::string const& name) const
    {
        auto it = std::find_if(headers_, headers_ + num_headers_, [&name](struct phr_header const& hdr)
        {
            return boost::string_ref(hdr.name, hdr.name_len) == name;
        });

        return it != headers_ + num_headers_;
    }

    std::size_t request::headers_num_cs(std::string const& name) const
    {
        std::size_t num = 0;
        for (std::size_t i = 0; i < num_headers_; ++i)
        {
            if (boost::string_ref(headers_[i].name, headers_[i].name_len) == name)
            {
                ++num;
            }
        }

        return num;
    }

	template<typename T>
	void fix_offset(T*& ptr, ptrdiff_t offset)
	{
		ptr = reinterpret_cast<const char*>(ptr) + offset;
	}

	void request::increase_buffer(std::size_t size)
	{
		auto tmp = static_cast<char*>(std::realloc(buffer_.buffer, buffer_.max_size + size));

		ptrdiff_t offset = tmp - buffer_.buffer;
		fix_offset(method_, offset);
		fix_offset(url_, offset);
		for (auto it = headers_; it != headers_ + num_headers_; ++it)
		{
			fix_offset(it->name, offset);
			fix_offset(it->value, offset);
		}

		buffer_.buffer = tmp;
		buffer_.max_size += size;
	}

}