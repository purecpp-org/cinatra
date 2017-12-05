
#include <cinatra_http/response.h>
#include <cinatra_http/utils.h>
#include <cinatra_http/mime_types.h>

#include <boost/lexical_cast.hpp>

#include <string>

namespace cinatra
{
	namespace status_strings
	{
		const std::string switching_protocols =
			"HTTP/1.1 101 Switching Protocals\r\n";
		const std::string ok =
			"HTTP/1.1 200 OK\r\n";
		const std::string created =
			"HTTP/1.1 201 Created\r\n";
		const std::string accepted =
			"HTTP/1.1 202 Accepted\r\n";
		const std::string no_content =
			"HTTP/1.1 204 No Content\r\n";
		const std::string multiple_choices =
			"HTTP/1.1 300 Multiple Choices\r\n";
		const std::string moved_permanently =
			"HTTP/1.1 301 Moved Permanently\r\n";
		const std::string moved_temporarily =
			"HTTP/1.1 302 Moved Temporarily\r\n";
		const std::string not_modified =
			"HTTP/1.1 304 Not Modified\r\n";
		const std::string bad_request =
			"HTTP/1.1 400 Bad Request\r\n";
		const std::string unauthorized =
			"HTTP/1.1 401 Unauthorized\r\n";
		const std::string forbidden =
			"HTTP/1.1 403 Forbidden\r\n";
		const std::string not_found =
			"HTTP/1.1 404 Not Found\r\n";
		const std::string internal_server_error =
			"HTTP/1.1 500 Internal Server Error\r\n";
		const std::string not_implemented =
			"HTTP/1.1 501 Not Implemented\r\n";
		const std::string bad_gateway =
			"HTTP/1.1 502 Bad Gateway\r\n";
		const std::string service_unavailable =
			"HTTP/1.1 503 Service Unavailable\r\n";

		boost::asio::const_buffer to_buffer(response::status_type status)
		{
			switch (status)
			{
			case response::switching_protocols:
				return boost::asio::buffer(switching_protocols);
			case response::ok:
				return boost::asio::buffer(ok);
			case response::created:
				return boost::asio::buffer(created);
			case response::accepted:
				return boost::asio::buffer(accepted);
			case response::no_content:
				return boost::asio::buffer(no_content);
			case response::multiple_choices:
				return boost::asio::buffer(multiple_choices);
			case response::moved_permanently:
				return boost::asio::buffer(moved_permanently);
			case response::moved_temporarily:
				return boost::asio::buffer(moved_temporarily);
			case response::not_modified:
				return boost::asio::buffer(not_modified);
			case response::bad_request:
				return boost::asio::buffer(bad_request);
			case response::unauthorized:
				return boost::asio::buffer(unauthorized);
			case response::forbidden:
				return boost::asio::buffer(forbidden);
			case response::not_found:
				return boost::asio::buffer(not_found);
			case response::internal_server_error:
				return boost::asio::buffer(internal_server_error);
			case response::not_implemented:
				return boost::asio::buffer(not_implemented);
			case response::bad_gateway:
				return boost::asio::buffer(bad_gateway);
			case response::service_unavailable:
				return boost::asio::buffer(service_unavailable);
			default:
				return boost::asio::buffer(internal_server_error);
			}
		}

	} // namespace status_strings

	namespace misc_strings
	{

		const char name_value_separator[] = { ':', ' ' };
		const char crlf[] = { '\r', '\n' };
	} // namespace misc_strings

	bool response::to_buffers(std::vector<boost::asio::const_buffer>& buffers)
	{
		if (!header_buffer_wroted_)
		{
            add_header("Date", http_date(time(nullptr)));
			buffers.reserve(headers_.size() * 4 + 5);
			buffers.emplace_back(status_strings::to_buffer(status_));
			for (auto const& h : headers_)
			{
				buffers.emplace_back(boost::asio::buffer(h.name));
				buffers.emplace_back(boost::asio::buffer(misc_strings::name_value_separator));
				buffers.emplace_back(boost::asio::buffer(h.value));
				buffers.emplace_back(boost::asio::buffer(misc_strings::crlf));
			}
			buffers.push_back(boost::asio::buffer(misc_strings::crlf));
			header_buffer_wroted_ = true;
		}
		
		switch (body_type_)
		{
		case response::none:
			return true;
		case response::string_body:
			buffers.emplace_back(boost::asio::buffer(content_));
			return true;
		case response::file_body:
		{
			content_.resize(1024 * 1024);
			fs_->read(&content_[0], content_.size());
			buffers.emplace_back(boost::asio::buffer(content_.data(), static_cast<std::size_t>(fs_->gcount())));
			return fs_->eof();
		}
			break;
		case response::chunked_body:
			content_ = content_gen_();
			if (content_.empty())
			{
				static const std::string chunked_end = "0\r\n\r\n";
				buffers.emplace_back(boost::asio::buffer(chunked_end));
				return true;
			}

			{
				// chunked编码中分段的长度
				static const char hex_lookup[] = "0123456789abcdef";
				size_t content_len = content_.size();
				int hex_len = 0;
				if (content_len == 0)
				{
					hex_len = 1;
				}
				for (auto tmp = content_len; tmp != 0; tmp >>= 4)
				{
					++hex_len;
				}

				chunked_len_buf_[hex_len] = '\r';
				chunked_len_buf_[hex_len + 1] = '\n';

				auto tmp = hex_len;
				for (--tmp; tmp >= 0; content_len >>= 4, --tmp)
				{
					chunked_len_buf_[tmp] = hex_lookup[content_len & 0xf];
				}
				buffers.emplace_back(boost::asio::buffer(chunked_len_buf_, hex_len + 2));
			}
			buffers.emplace_back(boost::asio::buffer(content_));
			buffers.emplace_back(boost::asio::buffer(misc_strings::crlf));
			return false;
		default:
			assert(false);
			return true;
		}
	}

	response response::stock_reply(response::status_type status)
	{
		response rep;
		rep.set_status(status);
		rep.response_text(rep.to_string(status));
		rep.headers_.resize(2);
		rep.headers_[0].name = "Content-Length";
		rep.headers_[0].value = boost::lexical_cast<std::string>(rep.content_.size());
		rep.headers_[1].name = "Content-Type";
		rep.headers_[1].value = "text/html";
		return rep;
	}

	void response::reset()
	{
		status_ = response::ok;
		header_buffer_wroted_ = false;
		body_type_ = none;
		headers_.clear();
		content_.clear();
		fs_.reset();
		content_gen_ = {};
	}

	void response::set_status(status_type status)
	{
		status_ = status;
	}

	std::vector<cinatra::response::header_t>& response::headers()
	{
		return headers_;
	}

	std::vector<cinatra::response::header_t> const& response::headers() const
	{
		return headers_;
	}

	void response::add_header(std::string const& name, std::string const& value)
	{
		headers_.emplace_back(header_t{ name, value });
	}

	boost::string_ref response::get_header(const std::string& name)
	{
		return get_header(name.data(), name.size());
	}

	boost::string_ref response::get_header(const char* name, size_t size) const
	{
		auto it = std::find_if(headers_.cbegin(), headers_.cend(), [this, name, size](header_t const& hdr)
		{
			return iequal(hdr.name.data(), hdr.name.size(), name, size);
		});

		if (it == headers_.cend())
		{
			return{};
		}

		return it->value;
	}

	std::vector<boost::string_ref> response::get_headers(const std::string& name) const
	{
		return get_headers(name.data(), name.size());
	}

	std::vector<boost::string_ref> response::get_headers(const char* name, size_t size) const
	{
		std::vector<boost::string_ref> headers;
		for (auto const& it : headers_)
		{
			if (iequal(it.name.data(), it.name.size(), name, size))
			{
				headers.emplace_back(boost::string_ref(it.value.data(), it.value.size()));
			}
		}

		return headers;
	}



	bool response::has_header(const std::string& name) const
	{
		return has_header(name.data(), name.size());
	}

	bool response::has_header(const char* name, size_t size) const
	{
		auto it = std::find_if(headers_.cbegin(), headers_.cend(), [name, size](header_t const& hdr)
		{
			return iequal(hdr.name.data(), hdr.name.size(), name, size);
		});

		return it != headers_.cend();
	}

	std::size_t response::headers_num(const std::string& name) const
	{
		return headers_num(name.data(), name.size());
	}

	std::size_t response::headers_num(const char* name, size_t size) const
	{
		std::size_t num = 0;
		for (auto const& it : headers_)
		{
			if (iequal(it.name.data(), it.name.size(), name, size))
			{
				++num;
			}
		}

		return num;
	}

	std::size_t response::headers_num() const
	{
		return headers_.size();
	}

	boost::string_ref response::get_header_cs(std::string const& name) const
	{
		auto it = std::find_if(headers_.cbegin(), headers_.cend(), [&name](header_t const& hdr)
		{
			return hdr.name == name;
		});

		if (it == headers_.cend())
		{
			return{};
		}

		return boost::string_ref(it->value.data(), it->value.size());
	}

	std::vector<boost::string_ref> response::get_headers_cs(std::string const& name) const
	{
		std::vector<boost::string_ref> headers;
		for (auto const& it : headers_)
		{
			if (it.name == name)
			{
				headers.emplace_back(boost::string_ref(it.value.data(), it.value.size()));
			}
		}

		return headers;
	}

	bool response::has_header_cs(std::string const& name) const
	{
		auto it = std::find_if(headers_.cbegin(), headers_.cend(), [&name](header_t const& hdr)
		{
			return hdr.name == name;
		});

		return it != headers_.cend();
	}

	std::size_t response::headers_num_cs(std::string const& name) const
	{
		std::size_t num = 0;
		for (auto const& it : headers_)
		{
			if (it.name == name)
			{
				++num;
			}
		}

		return num;
	}

	void response::response_text(std::string body)
	{
		if (!has_header("content-length", 14))
		{
			add_header("Content-Length", boost::lexical_cast<std::string>(body.size()));
		}

		body_type_ = response::string_body;
		content_ = std::move(body);
	}

	bool response::response_file(boost::filesystem::path path)
	{
		//TODO: 支持range
		boost::system::error_code ec;
		auto size = boost::filesystem::file_size(path, ec);
		if (ec)
		{
			return false;
		}
		add_header("Content-Length", boost::lexical_cast<std::string>(size));

		auto last_time = boost::filesystem::last_write_time(path, ec);
		if (ec)
		{
			return false;
		}
		add_header("Last-Modified", http_date(last_time));

		fs_.reset(new std::ifstream());
		fs_->open(path.generic_string(), std::ios::binary | std::ios::in);
		if (!fs_->is_open())
		{
			return false;
		}

        add_header("Content-Type", mime_types::extension_to_type(path.extension().generic_string()));
		body_type_ = response::file_body;
		return true;
	}

	void response::response_by_generator(content_generator_t gen)
	{
		body_type_ = response::chunked_body;
		add_header("Transfer-Encoding", "chunked");
		content_gen_ = std::move(gen);
	}

}
