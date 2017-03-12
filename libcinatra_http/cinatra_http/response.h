
#pragma once

#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <vector>
#include <fstream>
#include <memory>

#include <cinatra_http/picohttpparser.h>

namespace cinatra
{
	using content_generator_t = std::function<std::string(void)>;

	class response
	{
	public:

		using handler_ec_t = std::function<void(boost::system::error_code const&)>;
		using handler_ec_size_t = std::function<void(boost::system::error_code const&, std::size_t)>;
		using handler_strref_intptr_t = std::function<void(boost::string_ref, intptr_t)>;

		using write_func1_t = std::function<void(const void*, std::size_t, handler_ec_size_t)>;
		using write_func2_t = std::function<void(std::vector<boost::asio::const_buffer> const&, handler_ec_size_t)>;
		using read_func_t = std::function<void(void*, std::size_t, handler_ec_size_t)>;
		using read_chunk_func_t = std::function<void(handler_strref_intptr_t)>;
		using shutdown_func_t = std::function<void(handler_ec_t)>;
		using close_func_t = std::function<void(void)>;
		using is_closed_func_t = std::function<bool(void)>;
		using end_func_t = std::function<void(void)>;

		class connection
		{
		public:
			connection() = default;
			//TODO:怎样简化???????
			connection(response& rep, write_func1_t write_func1, write_func2_t write_func2,
				read_func_t read_func, read_func_t read_some_func, read_chunk_func_t read_chunk_func,
				shutdown_func_t shutdown_func, close_func_t close_func, is_closed_func_t is_closed_func, end_func_t end_func)
				:rep_(rep), write_func1_(std::move(write_func1)), write_func2_(std::move(write_func2)),
				read_func_(std::move(read_func)), read_chunk_func_(std::move(read_chunk_func)), read_some_func_(std::move(read_some_func)),
				shutdown_func_(std::move(shutdown_func)), close_func_(std::move(close_func)), is_closed_func_(std::move(is_closed_func)), end_func_(std::move(end_func))
			{}

			void async_write(const void* data, std::size_t size, handler_ec_size_t handler) const
			{
				write_func1_(data, size, std::move(handler));
			}

			void async_write(std::vector<boost::asio::const_buffer> const& buffers, handler_ec_size_t handler) const
			{
				write_func2_(buffers, std::move(handler));
			}

			void async_read(void* data, std::size_t size, handler_ec_size_t handler) const
			{
				read_func_(data, size, std::move(handler));
			}

			void async_read_some(void* data, std::size_t size, handler_ec_size_t handler)
			{
				read_some_func_(data, size, handler);
			}

			void async_read_chunk(handler_strref_intptr_t handler)
			{
				read_chunk_func_(handler);
			}

			void shutdown(handler_ec_t handler)
			{
				shutdown_func_(handler);
			}

			void close()
			{
				close_func_();
			}

			bool is_closed()
			{
				return is_closed_func_();
			}

			response const& get_reply() const
			{
				return rep_;
			}
			response& get_reply()
			{
				return rep_;
			}
			// TODO: chunked write
			~connection()
			{
				end_func_();
			}
		private:
			response& rep_;
			write_func1_t write_func1_;
			write_func2_t write_func2_;
			read_func_t read_func_;
			read_func_t read_some_func_;
			read_chunk_func_t read_chunk_func_;
			shutdown_func_t shutdown_func_;
			close_func_t close_func_;
			is_closed_func_t is_closed_func_;
			end_func_t end_func_;
		};

		using get_connection_func_t = std::function<std::shared_ptr<connection>(bool)>;

		enum status_type
		{
			switching_protocols = 101,
			ok = 200,
			created = 201,
			accepted = 202,
			no_content = 204,
			multiple_choices = 300,
			moved_permanently = 301,
			moved_temporarily = 302,
			not_modified = 304,
			bad_request = 400,
			unauthorized = 401,
			forbidden = 403,
			not_found = 404,
			internal_server_error = 500,
			not_implemented = 501,
			bad_gateway = 502,
			service_unavailable = 503
		};

		struct header_t
		{
			std::string name;
			std::string value;
		};

		bool to_buffers(std::vector<boost::asio::const_buffer>& buffers);
		static response stock_reply(status_type status);
		void reset();

		status_type status()
		{
			return status_;
		}
		void set_status(status_type status);
		std::vector<header_t>& headers();
		std::vector<header_t> const& headers() const;
		void add_header(std::string const& name, std::string const& value);
		
		boost::string_ref get_header(const std::string& name);
		boost::string_ref get_header(const char* name, size_t size) const;

		std::vector<boost::string_ref> get_headers(const std::string& name) const;
		std::vector<boost::string_ref> get_headers(const char* name, size_t size) const;

		bool has_header(const std::string& name) const;
		bool has_header(const char* name, size_t size) const;

		std::size_t headers_num(const std::string& name) const;
		std::size_t headers_num(const char* name, size_t size) const;

		std::size_t headers_num() const;

		boost::string_ref get_header_cs(std::string const& name) const;

		std::vector<boost::string_ref> get_headers_cs(std::string const& name) const;

		bool has_header_cs(std::string const& name) const;

		std::size_t headers_num_cs(std::string const& name) const;

		void response_text(std::string body);
		bool response_file(boost::filesystem::path path);
		void response_by_generator(content_generator_t gen);

		bool is_delay() const
		{
			return delay_;
		}
		void set_delay(bool delay)
		{
			delay_ = delay;
		}

		void set_get_connection_func(get_connection_func_t func)
		{
			get_connection_func_ = std::move(func);
		}

		std::shared_ptr<connection> get_connection(bool kill_timer = false, bool delay = true)
		{
			set_delay(delay);
			return get_connection_func_(kill_timer);
		}

		bool header_buffer_wroted() const { return header_buffer_wroted_; }

		enum body_type_t
		{
			none,
			string_body,
			file_body,
			chunked_body
		};

		body_type_t body_type() { return body_type_; }

		bool is_complete()
		{
			return body_type_ != none;
		}
	private:
		std::vector<header_t> headers_;
		std::string content_;
		status_type status_ = ok;

		bool header_buffer_wroted_ = false;
		body_type_t body_type_ = none;
		
		std::shared_ptr<std::ifstream> fs_;
		char chunked_len_buf_[20];
		content_generator_t content_gen_;

		get_connection_func_t get_connection_func_;

		bool delay_ = false;
	};
}
